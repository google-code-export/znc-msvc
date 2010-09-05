/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc.h"
#include "Chan.h"
#include "IRCSock.h"
#include "Server.h"
#include "User.h"
#include "Listener.h"
#include <pwd.h>
#include <list>
#ifdef HAVE_ARES
#include "ares.h"
#endif

namespace
{ // private namespace for local things
	struct CGlobalModuleConfigLine
	{
		CString  m_sName;
		CString  m_sValue;
		CUser   *m_pUser;
		CChan   *m_pChan;
	};
}

CZNC::CZNC() {
	m_pModules = new CGlobalModules();
	m_pISpoofLockFile = NULL;
	m_uiConnectDelay = 5;
	m_uiAnonIPLimit = 10;
	SetISpoofFormat(""); // Set ISpoofFormat to default
	m_uBytesRead = 0;
	m_uBytesWritten = 0;
	m_uiMaxBufferSize = 500;
	m_pConnectUserTimer = NULL;
	m_eConfigState = ECONFIG_NOTHING;
	m_TimeStarted = time(NULL);
	m_sConnectThrottle.SetTTL(30000);
}

CZNC::~CZNC() {
	if (m_pISpoofLockFile)
		ReleaseISpoof();

	m_pModules->UnloadAll();

	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->GetModules().UnloadAll();
	}

	for (size_t b = 0; b < m_vpListeners.size(); b++) {
		delete m_vpListeners[b];
	}

	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->SetBeingDeleted(true);
	}

	m_pConnectUserTimer = NULL;
	// This deletes m_pConnectUserTimer
	m_Manager.Cleanup();
	DeleteUsers();

	delete m_pModules;

	ShutdownCsocket();
	DeletePidFile();
}

CString CZNC::GetVersion() {
	char szBuf[128];

	snprintf(szBuf, sizeof(szBuf), "%1.3f"VERSION_EXTRA, VERSION);
	// If snprintf overflows (which I doubt), we want to be on the safe side
	szBuf[sizeof(szBuf) - 1] = '\0';

	return szBuf;
}

CString CZNC::GetTag(bool bIncludeVersion) {
	if (!bIncludeVersion) {
		return "ZNC - http://znc.sourceforge.net";
	}

	char szBuf[128];
	snprintf(szBuf, sizeof(szBuf), "ZNC %1.3f"VERSION_EXTRA" - http://znc.sourceforge.net", VERSION);
	// If snprintf overflows (which I doubt), we want to be on the safe side
	szBuf[sizeof(szBuf) - 1] = '\0';

	return szBuf;
}

CString CZNC::GetUptime() const {
	time_t now = time(NULL);
	return CString::ToTimeStr(now - TimeStarted());
}

bool CZNC::OnBoot() {
	ALLMODULECALL(OnBoot(), return false);

	return true;
}

bool CZNC::ConnectUser(CUser *pUser) {
	CString sSockName = "IRC::" + pUser->GetUserName();
	// Don't use pUser->GetIRCSock(), as that only returns something if the
	// CIRCSock is already connected, not when it's still connecting!
	CIRCSock* pIRCSock = (CIRCSock*) m_Manager.FindSockByName(sSockName);

	if (m_pISpoofLockFile != NULL) {
		return false;
	}

	if (!pUser->GetIRCConnectEnabled())
		return false;

	if (pIRCSock || !pUser->HasServers())
		return false;

	if (pUser->ConnectPaused())
		return false;

	CServer* pServer = pUser->GetNextServer();

	if (!pServer)
		return false;

	if (m_sConnectThrottle.GetItem(pServer->GetName()))
		return false;

	if (!WriteISpoof(pUser)) {
		DEBUG("ISpoof could not be written");
		pUser->PutStatus("ISpoof could not be written, retrying...");
		return true;
	}

	m_sConnectThrottle.AddItem(pServer->GetName());

	DEBUG("User [" << pUser->GetUserName() << "] is connecting to [" << pServer->GetString(false) << "] ...");
	pUser->PutStatus("Attempting to connect to [" + pServer->GetString(false) + "] ...");

	pIRCSock = new CIRCSock(pUser);
	pIRCSock->SetPass(pServer->GetPass());

	bool bSSL = false;
#ifdef HAVE_LIBSSL
	if (pServer->IsSSL()) {
		bSSL = true;
	}
#endif

	MODULECALL(OnIRCConnecting(pIRCSock), pUser, NULL,
		DEBUG("Some module aborted the connection attempt");
		pUser->PutStatus("Some module aborted the connection attempt");
		ReleaseISpoof();
		delete pIRCSock;
		return false;
	);

	if (!m_Manager.Connect(pServer->GetName(), pServer->GetPort(), sSockName, 120, bSSL, pUser->GetBindHost(), pIRCSock)) {
		ReleaseISpoof();
		pUser->PutStatus("Unable to connect. (Bad host?)");
	}

	return true;
}

bool CZNC::HandleUserDeletion()
{
	map<CString, CUser*>::iterator it;
	map<CString, CUser*>::iterator end;

	if (m_msDelUsers.empty())
		return false;

	end = m_msDelUsers.end();
	for (it = m_msDelUsers.begin(); it != end; ++it) {
		CUser* pUser = it->second;
		pUser->SetBeingDeleted(true);

		if (GetModules().OnDeleteUser(*pUser)) {
			pUser->SetBeingDeleted(false);
			continue;
		}
		m_msUsers.erase(pUser->GetUserName());

		// Don't use pUser->GetIRCSock(), as that only returns something if the
		// CIRCSock is already connected, not when it's still connecting!
		CIRCSock* pIRCSock = (CIRCSock*) m_Manager.FindSockByName("IRC::" + pUser->GetUserName());

		if (pIRCSock) {
			m_Manager.DelSockByAddr(pIRCSock);
		}

		pUser->DelClients();
		pUser->DelModules();
		CWebSock::FinishUserSessions(*pUser);
		AddBytesRead(pUser->BytesRead());
		AddBytesWritten(pUser->BytesWritten());
		delete pUser;
	}

	m_msDelUsers.clear();

	return true;
}

#ifdef _WIN32
void CZNC::Loop(bool* bLoop) {
	while (*bLoop) {
		LoopDoMaintenance();

		// have to use Loop instead of DynamicSelectLoop because the latter waits for too long
		m_Manager.Loop();
	}
	Broadcast("ZNC has been requested to shut down!");
	if(!CZNC::Get().WriteConfig())
	{
		CUtils::PrintError("Writing config file to disk failed! We are forced to continue shutting down anyway.");
	}
}
#else
void CZNC::Loop() {
	while (true) {
		LoopDoMaintenance();

		// Csocket wants micro seconds
		// 500 msec to 600 sec
		m_Manager.DynamicSelectLoop(500 * 1000, 600 * 1000 * 1000);
	}
}
#endif

void CZNC::LoopDoMaintenance()
{
	CString sError;

	switch (GetConfigState()) {
		case ECONFIG_NEED_REHASH:
			SetConfigState(ECONFIG_NOTHING);

			if (RehashConfig(sError)) {
				Broadcast("Rehashing succeeded", true);
			} else {
				Broadcast("Rehashing failed: " + sError, true);
				Broadcast("ZNC is in some possibly inconsistent state!", true);
			}
			break;
		case ECONFIG_NEED_WRITE:
			SetConfigState(ECONFIG_NOTHING);

			if (WriteConfig()) {
				Broadcast("Writing the config suceeded", true);
			} else {
				Broadcast("Writing the config file failed", true);
			}
			break;
		case ECONFIG_NOTHING:
			break;
	}

	// Check for users that need to be deleted
	if (HandleUserDeletion()) {
		// Also remove those user(s) from the config file
		WriteConfig();
	}
}


bool CZNC::WriteISpoof(CUser* pUser) {
	if (m_pISpoofLockFile != NULL)
		return false;

	if (!m_sISpoofFile.empty()) {
		m_pISpoofLockFile = new CFile;
		if (!m_pISpoofLockFile->TryExLock(m_sISpoofFile, O_RDWR | O_CREAT)) {
			delete m_pISpoofLockFile;
			m_pISpoofLockFile = NULL;
			return false;
		}

		char buf[1024];
		memset((char*) buf, 0, 1024);
		m_pISpoofLockFile->Read(buf, 1023);
		m_sOrigISpoof = buf;

		if (!m_pISpoofLockFile->Seek(0) || !m_pISpoofLockFile->Truncate()) {
			delete m_pISpoofLockFile;
			m_pISpoofLockFile = NULL;
			return false;
		}

		CString sData = pUser->ExpandString(m_sISpoofFormat);

		// If the format doesn't contain anything expandable, we'll
		// assume this is an "old"-style format string.
		if (sData == m_sISpoofFormat) {
			sData.Replace("%", pUser->GetIdent());
		}
		m_pISpoofLockFile->Write(sData + "\n");
	}
	return true;
}

void CZNC::ReleaseISpoof() {
	if (m_pISpoofLockFile == NULL)
		return;

	if (!m_sISpoofFile.empty()) {
		if (m_pISpoofLockFile->Seek(0) && m_pISpoofLockFile->Truncate()) {
			m_pISpoofLockFile->Write(m_sOrigISpoof);
		}

		m_sOrigISpoof = "";
	}

	delete m_pISpoofLockFile;
	m_pISpoofLockFile = NULL;
}

CFile* CZNC::InitPidFile() {
	if (!m_sPidFile.empty()) {
		CString sFile;

		// absolute path or relative to the data dir?
		if (m_sPidFile[0] != '/')
			sFile = GetZNCPath() + "/" + m_sPidFile;
		else
			sFile = m_sPidFile;

		return new CFile(sFile);
	}

	return NULL;
}

bool CZNC::WritePidFile(int iPid) {
	CFile* File = InitPidFile();
	if (File == NULL)
		return false;

	CUtils::PrintAction("Writing pid file [" + File->GetLongName() + "]");

	bool bRet = false;
	if (File->Open(O_WRONLY | O_TRUNC | O_CREAT)) {
		File->Write(CString(iPid) + "\n");
		File->Close();
		bRet = true;
	}

	delete File;
	CUtils::PrintStatus(bRet);
	return bRet;
}

bool CZNC::DeletePidFile() {
	CFile* File = InitPidFile();
	if (File == NULL)
		return false;

	CUtils::PrintAction("Deleting pid file [" + File->GetLongName() + "]");

	bool bRet = false;
	if (File->Delete())
		bRet = true;

	delete File;
	CUtils::PrintStatus(bRet);
	return bRet;
}

bool CZNC::WritePemFile() {
#ifndef HAVE_LIBSSL
	CUtils::PrintError("ZNC was not compiled with ssl support.");
	return false;
#else
	CString sPemFile = GetPemLocation();
	const char* pHostName = getenv("HOSTNAME");
	CString sHost;

	if (pHostName) {
		sHost = pHostName;
	}

	if (CFile::Exists(sPemFile)) {
		CUtils::PrintError("Pem file [" + sPemFile + "] already exists");
		return false;
	}

	while (!CUtils::GetInput("hostname of your shell", sHost, sHost, "including the '.com' portion")) ;

	CUtils::PrintAction("Writing Pem file [" + sPemFile + "]");
	FILE *f = fopen(sPemFile.c_str(), "w");

	if (!f) {
		CUtils::PrintStatus(false, "Unable to open");
		return false;
	}

	CUtils::GenerateCert(f, sHost);
	fclose(f);

	CUtils::PrintStatus(true);
	return true;
#endif
}

void CZNC::DeleteUsers() {
	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		a->second->SetBeingDeleted(true);
		delete a->second;
	}

	m_msUsers.clear();
	DisableConnectUser();
}

Csock* CZNC::FindSockByName(const CString& sSockName) {
	return m_Manager.FindSockByName(sSockName);
}

bool CZNC::IsHostAllowed(const CString& sHostMask) const {
	for (map<CString,CUser*>::const_iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		if (a->second->IsHostAllowed(sHostMask)) {
			return true;
		}
	}

	return false;
}

bool CZNC::AllowConnectionFrom(const CString& sIP) const {
	if (m_uiAnonIPLimit == 0)
		return true;
	if (GetManager().GetAnonConnectionCount(sIP) >= m_uiAnonIPLimit)
		return false;
	return true;
}

void CZNC::InitDirs(const CString& sArgvPath, const CString& sDataDir) {
#ifndef _WIN32
	char *home;

	// If the bin was not ran from the current directory, we need to add that dir onto our cwd
	CString::size_type uPos = sArgvPath.rfind('/');
	if (uPos == CString::npos)
		m_sCurPath = "./";
	else
		m_sCurPath = CDir::ChangeDir("./", sArgvPath.Left(uPos), "");

	// Try to set the user's home dir, default to binpath on failure
	home = getenv("HOME");

	m_sHomePath.clear();
	if (home) {
		m_sHomePath = home;
	}

	if (m_sHomePath.empty()) {
		struct passwd* pUserInfo = getpwuid(getuid());

		if (pUserInfo) {
			m_sHomePath = pUserInfo->pw_dir;
		}
	}

	if (m_sHomePath.empty()) {
		m_sHomePath = m_sCurPath;
	}

	if (sDataDir.empty()) {
		m_sZNCPath = m_sHomePath + "/.znc";
	} else {
		m_sZNCPath = sDataDir;
	}
#else
	char strPath[MAX_PATH + 1] = {0};

	m_sCurPath = CDir::ChangeDir("./", "", "");

	// fallback home path = current dir.
	m_sHomePath = m_sCurPath;

	// preferred home path = CSIDL_PERSONAL (= My Documents)
	if(SUCCEEDED(SHGetFolderPath(0, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, strPath)) && PathIsDirectory(strPath))
	{
		m_sHomePath = CDir::ChangeDir(strPath, "", "");
	}

	if(!sDataDir.empty())
	{
		// a custom datadir overrides everything else.
		m_sZNCPath = CDir::ChangeDir("./", sDataDir, "");
	}
	else
	{
		// .znc dir in current folder overrides default
		// to maintain backwards compatibility.
		m_sZNCPath = CDir::ChangeDir(m_sCurPath, ".znc", "");

		if(!PathIsDirectory(m_sZNCPath.c_str()))
		{
			*strPath = 0;

			// default config location = Application Data\ZNC.
			if(SUCCEEDED(SHGetFolderPath(0, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath)) && PathIsDirectory(strPath))
			{
				m_sZNCPath = CDir::ChangeDir(strPath, "ZNC", "");
			}
		}
	}
#endif

	m_sSSLCertFile = m_sZNCPath + "/znc.pem";
}

CString CZNC::GetConfPath(bool bAllowMkDir) const {
	CString sConfPath = m_sZNCPath + "/configs";
	if (bAllowMkDir && !CFile::Exists(sConfPath)) {
		CDir::MakeDir(sConfPath);
	}

	return sConfPath;
}

CString CZNC::GetUserPath() const {
	CString sUserPath = m_sZNCPath + "/users";
	if (!CFile::Exists(sUserPath)) {
		CDir::MakeDir(sUserPath);
	}

	return sUserPath;
}

CString CZNC::GetModPath() const {
	CString sModPath = m_sZNCPath + "/modules";

	if (!CFile::Exists(sModPath)) {
		CDir::MakeDir(sModPath);
	}

	return sModPath;
}


CString CZNC::ExpandConfigPath(const CString& sConfigFile, bool bAllowMkDir) {
	CString sRetPath;

	if (sConfigFile.empty()) {
		sRetPath = GetConfPath(bAllowMkDir) + "/znc.conf";
	} else {
		if (sConfigFile.Left(2) == "./" || sConfigFile.Left(3) == "../") {
			sRetPath = GetCurPath() + "/" + sConfigFile;
#ifdef WIN32
		} else if(PathIsFileSpec(sConfigFile.c_str())) {
#else
		} else if (sConfigFile.Left(1) != "/") {
#endif
			sRetPath = GetConfPath(bAllowMkDir) + "/" + sConfigFile;
		} else {
			sRetPath = sConfigFile;
		}
	}

	return sRetPath;
}

bool CZNC::WriteConfig() {
	if (GetConfigFile().empty()) {
		return false;
	}

	// Close the old handle to the config file, we are replacing that file.
	m_LockFile.Close();

	// We first write to a temporary file and then move it to the right place
	m_LockFile.SetFileName(GetConfigFile() + "~");

	if (!m_LockFile.Open(O_WRONLY | O_CREAT | O_TRUNC, 0600)) {
		return false;
	}

	// We have to "transfer" our lock on the config to the new file.
	// The old file (= inode) is going away and thus a lock on it would be
	// useless. These lock should always succeed (races, anyone?).
	if (!m_LockFile.TryExLock()) {
		return false;
	}

	GLOBALMODULECALL(OnWriteConfig(m_LockFile), NULL, NULL, return false);

	m_LockFile.Write("AnonIPLimit  = " + CString(m_uiAnonIPLimit) + "\n");
	m_LockFile.Write("MaxBufferSize= " + CString(m_uiMaxBufferSize) + "\n");
	m_LockFile.Write("SSLCertFile  = " + CString(m_sSSLCertFile) + "\n");

	for (size_t l = 0; l < m_vpListeners.size(); l++) {
		CListener* pListener = m_vpListeners[l];
		CString sHostPortion = pListener->GetBindHost();

		if (!sHostPortion.empty()) {
			sHostPortion = sHostPortion.FirstLine() + " ";
		}

		CString sAcceptProtocol;
		if(pListener->GetAcceptType() == CListener::ACCEPT_IRC)
			sAcceptProtocol = "irc_only ";
		else if(pListener->GetAcceptType() == CListener::ACCEPT_HTTP)
			sAcceptProtocol = "web_only ";

		CString s6;
		switch (pListener->GetAddrType()) {
			case ADDR_IPV4ONLY:
				s6 = "4";
				break;
			case ADDR_IPV6ONLY:
				s6 = "6";
				break;
			case ADDR_ALL:
				s6 = " ";
				break;
		}

		m_LockFile.Write("Listener" + s6 + "    = " + sAcceptProtocol + sHostPortion +
			CString((pListener->IsSSL()) ? "+" : "") + CString(pListener->GetPort()) + "\n");
	}

	m_LockFile.Write("ConnectDelay = " + CString(m_uiConnectDelay) + "\n");
	m_LockFile.Write("ServerThrottle = " + CString(m_sConnectThrottle.GetTTL()/1000) + "\n");

	if (!m_sISpoofFile.empty()) {
		m_LockFile.Write("ISpoofFile   = " + m_sISpoofFile.FirstLine() + "\n");
		if (!m_sISpoofFormat.empty()) {
			m_LockFile.Write("ISpoofFormat = " + m_sISpoofFormat.FirstLine() + "\n");
		}
	}

	if (!m_sPidFile.empty()) {
		m_LockFile.Write("PidFile      = " + m_sPidFile.FirstLine() + "\n");
	}

	if (!m_sSkinName.empty()) {
		m_LockFile.Write("Skin         = " + m_sSkinName.FirstLine() + "\n");
	}

	if (!m_sStatusPrefix.empty()) {
		m_LockFile.Write("StatusPrefix = " + m_sStatusPrefix.FirstLine() + "\n");
	}

	for (unsigned int m = 0; m < m_vsMotd.size(); m++) {
		m_LockFile.Write("Motd         = " + m_vsMotd[m].FirstLine() + "\n");
	}

	for (unsigned int v = 0; v < m_vsBindHosts.size(); v++) {
		m_LockFile.Write("BindHost     = " + m_vsBindHosts[v].FirstLine() + "\n");
	}

	CGlobalModules& Mods = GetModules();

	for (unsigned int a = 0; a < Mods.size(); a++) {
		CString sName = Mods[a]->GetModName();
		CString sArgs = Mods[a]->GetArgs();

		if (!sArgs.empty()) {
			sArgs = " " + sArgs.FirstLine();
		}

		m_LockFile.Write("LoadModule   = " + sName.FirstLine() + sArgs + "\n");
	}

	for (map<CString,CUser*>::iterator it = m_msUsers.begin(); it != m_msUsers.end(); ++it) {
		CString sErr;

		if (!it->second->IsValid(sErr)) {
			DEBUG("** Error writing config for user [" << it->first << "] [" << sErr << "]");
			continue;
		}

		m_LockFile.Write("\n");

		if (!it->second->WriteConfig(m_LockFile)) {
			DEBUG("** Error writing config for user [" << it->first << "]");
		}
	}

	// If Sync() fails... well, let's hope nothing important breaks..
	m_LockFile.Sync();

	// We wrote to a temporary name, move it to the right place
#ifdef _WIN32
	// on win32, we need to close the ~ temp file first.
	m_LockFile.Close();

	bool bResult = m_LockFile.Move(GetConfigFile(), true);

	// re-open config to keep it from being deleted.
	// aka restore state before entering this function.
	m_LockFile.Open(GetConfigFile());

	return bResult;
#else
	if (!m_LockFile.Move(GetConfigFile(), true))
		return false;

	// Everything went fine, just need to update the saved path.
	m_LockFile.SetFileName(GetConfigFile());

	return true;
#endif
}

bool CZNC::WriteNewConfig(const CString& sConfigFile) {
	CString sAnswer, sUser;
	VCString vsLines;

	m_sConfigFile = ExpandConfigPath(sConfigFile);
	CUtils::PrintMessage("Building new config");

	CUtils::PrintMessage("");
	CUtils::PrintMessage("First lets start with some global settings...");
	CUtils::PrintMessage("");

	// Listen
	unsigned int uListenPort = 0;
	while (!CUtils::GetNumInput("What port would you like ZNC to listen on?", uListenPort, 1, 65535)) ;

	CString sSSL;
#ifdef HAVE_LIBSSL
	if (CUtils::GetBoolInput("Would you like ZNC to listen using SSL?", false)) {
		sSSL = "+";

		CString sPemFile = GetPemLocation();
		if (!CFile::Exists(sPemFile)) {
			CUtils::PrintError("Unable to locate pem file: [" + sPemFile + "]");
			if (CUtils::GetBoolInput("Would you like to create a new pem file now?",
						true)) {
				WritePemFile();
			}
		}
	}
#endif

	CString s6 = "4";
#ifdef HAVE_IPV6
	if (CUtils::GetBoolInput("Would you like ZNC to listen using ipv6?", false)) {
		s6 = " ";
	}
#endif

	CString sListenHost;
	CUtils::GetInput("Listen Host", sListenHost, "", "Blank for all ips");

	if (!sListenHost.empty()) {
		sListenHost += " ";
	}

	vsLines.push_back("Listener" + s6 + "  = " + sListenHost + sSSL + CString(uListenPort));
	// !Listen

	set<CModInfo> ssGlobalMods;
	GetModules().GetAvailableMods(ssGlobalMods, true);
	size_t uNrOtherGlobalMods = FilterUncommonModules(ssGlobalMods);

	if (!ssGlobalMods.empty()) {
		CUtils::PrintMessage("");
		CUtils::PrintMessage("-- Global Modules --");
		CUtils::PrintMessage("");

		if (CUtils::GetBoolInput("Do you want to load any global modules?")) {
			CTable Table;
			Table.AddColumn("Name");
			Table.AddColumn("Description");
			set<CModInfo>::iterator it;

			for (it = ssGlobalMods.begin(); it != ssGlobalMods.end(); ++it) {
				const CModInfo& Info = *it;
				Table.AddRow();
				Table.SetCell("Name", Info.GetName());
				Table.SetCell("Description", Info.GetDescription().Ellipsize(128));
			}

			unsigned int uTableIdx = 0; CString sLine;
			while (Table.GetLine(uTableIdx++, sLine)) {
				CUtils::PrintMessage(sLine);
			}

			if (uNrOtherGlobalMods > 0) {
				CUtils::PrintMessage("And " + CString(uNrOtherGlobalMods) + " other (uncommon) modules. You can enable those later.");
			}

			CUtils::PrintMessage("");

			for (it = ssGlobalMods.begin(); it != ssGlobalMods.end(); ++it) {
				const CModInfo& Info = *it;
				CString sName = Info.GetName();

				if (CUtils::StdoutIsTTY()) {
					if (CUtils::GetBoolInput("Load global module <\033[1m" + sName + "\033[22m>?", false))
						vsLines.push_back("LoadModule = " + sName);
				} else {
					if (CUtils::GetBoolInput("Load global module <" + sName + ">?", false))
						vsLines.push_back("LoadModule = " + sName);
				}
			}
		}
	}

	// User
	CUtils::PrintMessage("");
	CUtils::PrintMessage("Now we need to setup a user...");
	CUtils::PrintMessage("ZNC needs one user per IRC network.");
	CUtils::PrintMessage("");

	bool bFirstUser = true;

	do {
		vsLines.push_back("");
		CString sNick;
		do {
			CUtils::GetInput("Username", sUser, "", "AlphaNumeric");
		} while (!CUser::IsValidUserName(sUser));

		vsLines.push_back("<User " + sUser + ">");
		CString sSalt;
		sAnswer = CUtils::GetSaltedHashPass(sSalt);
		vsLines.push_back("\tPass       = " + CUtils::sDefaultHash + "#" + sAnswer + "#" + sSalt + "#");

		if (CUtils::GetBoolInput("Would you like this user to be an admin?", bFirstUser)) {
			vsLines.push_back("\tAdmin      = true");
		} else {
			vsLines.push_back("\tAdmin      = false");
		}

		CUtils::GetInput("Nick", sNick, CUser::MakeCleanUserName(sUser));
		vsLines.push_back("\tNick       = " + sNick);
		CUtils::GetInput("Alt Nick", sAnswer, sNick + "_");
		if (!sAnswer.empty()) {
			vsLines.push_back("\tAltNick    = " + sAnswer);
		}
		CUtils::GetInput("Ident", sAnswer, sNick);
		vsLines.push_back("\tIdent      = " + sAnswer);
		CUtils::GetInput("Real Name", sAnswer, "Got ZNC?");
		vsLines.push_back("\tRealName   = " + sAnswer);
		CUtils::GetInput("Bind Host", sAnswer, "", "optional");
		if (!sAnswer.empty()) {
			vsLines.push_back("\tBindHost   = " + sAnswer);
		}
		// todo: Possibly add motd

		unsigned int uBufferCount = 0;

		CUtils::GetNumInput("Number of lines to buffer per channel", uBufferCount, 0, ~0, 50);
		if (uBufferCount) {
			vsLines.push_back("\tBuffer     = " + CString(uBufferCount));
		}
		if (CUtils::GetBoolInput("Would you like to keep buffers after replay?", false)) {
			vsLines.push_back("\tKeepBuffer = true");
		} else {
			vsLines.push_back("\tKeepBuffer = false");
		}

		CUtils::GetInput("Default channel modes", sAnswer, "+stn");
		if (!sAnswer.empty()) {
			vsLines.push_back("\tChanModes  = " + sAnswer);
		}

		set<CModInfo> ssUserMods;
		GetModules().GetAvailableMods(ssUserMods);
		size_t uNrOtherUserMods = FilterUncommonModules(ssUserMods);

		if (ssUserMods.size()) {
			vsLines.push_back("");
			CUtils::PrintMessage("");
			CUtils::PrintMessage("-- User Modules --");
			CUtils::PrintMessage("");

			if (CUtils::GetBoolInput("Do you want to automatically load any user modules for this user?")) {
				CTable Table;
				Table.AddColumn("Name");
				Table.AddColumn("Description");
				set<CModInfo>::iterator it;

				for (it = ssUserMods.begin(); it != ssUserMods.end(); ++it) {
					const CModInfo& Info = *it;
					Table.AddRow();
					Table.SetCell("Name", Info.GetName());
					Table.SetCell("Description", Info.GetDescription().Ellipsize(128));
				}

				unsigned int uTableIdx = 0; CString sLine;
				while (Table.GetLine(uTableIdx++, sLine)) {
					CUtils::PrintMessage(sLine);
				}

				if (uNrOtherUserMods > 0) {
					CUtils::PrintMessage("And " + CString(uNrOtherUserMods) + " other (uncommon) modules. You can enable those later.");
				}

				CUtils::PrintMessage("");

				for (it = ssUserMods.begin(); it != ssUserMods.end(); ++it) {
					const CModInfo& Info = *it;
					CString sName = Info.GetName();

					if (CUtils::StdoutIsTTY()) {
						if (CUtils::GetBoolInput("Load module <\033[1m" + sName + "\033[22m>?", false))
							vsLines.push_back("\tLoadModule = " + sName);
					} else {
						if (CUtils::GetBoolInput("Load module <" + sName + ">?", false))
							vsLines.push_back("\tLoadModule = " + sName);
					}
				}
			}
		}

		vsLines.push_back("");
		CUtils::PrintMessage("");
		CUtils::PrintMessage("-- IRC Servers --");
		CUtils::PrintMessage("Only add servers from the same IRC network.");
		CUtils::PrintMessage("If a server somehow doesn't work, another server will be used.");
		CUtils::PrintMessage("");

		do {
			CString sHost, sPass;
			bool bSSL = false;
			unsigned int uServerPort = 0;

			while (!CUtils::GetInput("IRC server", sHost, "", "host only") || !CServer::IsValidHostName(sHost)) ;
			while (!CUtils::GetNumInput("[" + sHost + "] Port", uServerPort, 1, 65535, 6667)) ;
			CUtils::GetInput("[" + sHost + "] Password (probably empty)", sPass);

#ifdef HAVE_LIBSSL
			bSSL = CUtils::GetBoolInput("Does this server use SSL?", false);
#endif

			vsLines.push_back("\tServer     = " + sHost + ((bSSL) ? " +" : " ") + CString(uServerPort) + " " + sPass);

			CUtils::PrintMessage("");
		} while (CUtils::GetBoolInput("Would you like to add another server for this IRC network?", false));

		vsLines.push_back("");
		CUtils::PrintMessage("");
		CUtils::PrintMessage("-- Channels --");
		CUtils::PrintMessage("");

		CString sArg = "a";
		CString sPost = " for ZNC to automatically join?";
		bool bDefault = true;

		while (CUtils::GetBoolInput("Would you like to add " + sArg + " channel" + sPost, bDefault)) {
			while (!CUtils::GetInput("Channel name", sAnswer)) ;
			vsLines.push_back("\t<Chan " + sAnswer + ">");
			vsLines.push_back("\t</Chan>");
			sArg = "another";
			sPost = "?";
			bDefault = false;
		}

		vsLines.push_back("</User>");

		CUtils::PrintMessage("");
		bFirstUser = false;
	} while (CUtils::GetBoolInput("Would you like to setup another user (e.g. for connecting to another network)?", false));
	// !User

	CFile File;
	bool bFileOK, bFileOpen = false;
	do {
		CUtils::PrintAction("Writing config [" + m_sConfigFile + "]");

		bFileOK = true;
		if (CFile::Exists(m_sConfigFile)) {
			if (!m_LockFile.TryExLock(m_sConfigFile)) {
				CUtils::PrintStatus(false, "ZNC is currently running on this config.");
				bFileOK = false;
			} else {
				m_LockFile.Close();
				CUtils::PrintStatus(false, "This config already exists.");
				if (CUtils::GetBoolInput("Would you like to overwrite it?", false))
					CUtils::PrintAction("Overwriting config [" + m_sConfigFile + "]");
				else
					bFileOK = false;
			}
		}

		if (bFileOK) {
			File.SetFileName(m_sConfigFile);
			if (File.Open(O_WRONLY | O_CREAT | O_TRUNC, 0600)) {
				bFileOpen = true;
			} else {
				CUtils::PrintStatus(false, "Unable to open file");
				bFileOK = false;
			}
		}
		if (!bFileOK) {
			CUtils::GetInput("Please specify an alternate location (or \"stdout\" for displaying the config)", m_sConfigFile, m_sConfigFile);
			if (m_sConfigFile.Equals("stdout"))
				bFileOK = true;
			else
				m_sConfigFile = ExpandConfigPath(m_sConfigFile);
		}
	} while (!bFileOK);

	if (!bFileOpen) {
		CUtils::PrintMessage("");
		CUtils::PrintMessage("Printing the new config to stdout:");
		CUtils::PrintMessage("");
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
	}

	for (unsigned int a = 0; a < vsLines.size(); a++) {
		if (bFileOpen) {
			File.Write(vsLines[a] + "\n");
		} else {
			cout << vsLines[a] << endl;
		}
	}

	if (bFileOpen) {
		File.Close();
		CUtils::PrintStatus(true);
	} else {
		cout << endl << "----------------------------------------------------------------------------" << endl << endl;
	}

	CUtils::PrintMessage("");
	CUtils::PrintMessage("To connect to this znc you need to connect to it as your irc server", true);
	CUtils::PrintMessage("using the port that you supplied.  You have to supply your login info", true);
	CUtils::PrintMessage("as the irc server password like so... user:pass.", true);
	CUtils::PrintMessage("");
	CUtils::PrintMessage("Try something like this in your IRC client...", true);
	CUtils::PrintMessage("/server <znc_server_ip> " + CString(uListenPort) + " " + sUser + ":<pass>", true);
	CUtils::PrintMessage("");

	m_LockFile.UnLock();
	return bFileOpen && CUtils::GetBoolInput("Launch znc now?", true);
}

size_t CZNC::FilterUncommonModules(set<CModInfo>& ssModules) {
	const char* ns[] = { "webadmin", "admin",
		"chansaver", "keepnick", "simple_away", "partyline",
		"kickrejoin", "nickserv", "perform" };
	const set<CString> ssNames(ns, ns + sizeof(ns) / sizeof(ns[0]));

	size_t uNrRemoved = 0;
	for(set<CModInfo>::iterator it = ssModules.begin(); it != ssModules.end(); ) {
		if(ssNames.count(it->GetName()) > 0) {
			it++;
		} else {
			set<CModInfo>::iterator it2 = it++;
			ssModules.erase(it2);
			uNrRemoved++;
		}
	}

	return uNrRemoved;
}

bool CZNC::ParseConfig(const CString& sConfig)
{
	CString s;

	m_sConfigFile = ExpandConfigPath(sConfig, false);

	return DoRehash(s);
}

bool CZNC::RehashConfig(CString& sError)
{
	ALLMODULECALL(OnPreRehash(), );

	// This clears m_msDelUsers
	HandleUserDeletion();

	// Mark all users as going-to-be deleted
	m_msDelUsers = m_msUsers;
	m_msUsers.clear();

	if (DoRehash(sError)) {
		ALLMODULECALL(OnPostRehash(), );

		return true;
	}

	// Rehashing failed, try to recover
	CString s;
	while (!m_msDelUsers.empty()) {
		AddUser(m_msDelUsers.begin()->second, s);
		m_msDelUsers.erase(m_msDelUsers.begin());
	}

	return false;
}

bool CZNC::DoRehash(CString& sError)
{
	sError.clear();

	CUtils::PrintAction("Opening Config [" + m_sConfigFile + "]");

	if (!CFile::Exists(m_sConfigFile)) {
		sError = "No such file";
		CUtils::PrintStatus(false, sError);
		CUtils::PrintMessage("Restart znc with the --makeconf option if you wish to create this config.");
		return false;
	}

	if (!CFile::IsReg(m_sConfigFile)) {
		sError = "Not a file";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	// (re)open the config file
	if (m_LockFile.IsOpen())
		m_LockFile.Close();

	// need to open the config file Read/Write for fcntl()
	// exclusive locking to work properly!
	if (!m_LockFile.Open(m_sConfigFile, O_RDWR)) {
		sError = "Can not open config file";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	if (!m_LockFile.TryExLock()) {
		sError = "ZNC is already running on this config.";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	CFile &File = m_LockFile;

	// This fd is re-used for rehashing, so we must seek back to the beginning!
	if (!File.Seek(0)) {
		sError = "Could not seek to the beginning of the config.";
		CUtils::PrintStatus(false, sError);
		return false;
	}

	CUtils::PrintStatus(true);

	m_vsBindHosts.clear();
	m_vsMotd.clear();

	// Delete all listeners
	while (!m_vpListeners.empty()) {
		delete m_vpListeners[0];
		m_vpListeners.erase(m_vpListeners.begin());
	}

	CString sLine;
	bool bCommented = false;     // support for /**/ style comments
	CUser* pUser = NULL;         // Used to keep track of which user block we are in
	CUser* pRealUser = NULL;     // If we rehash a user, this is the real one
	CChan* pChan = NULL;         // Used to keep track of which chan block we are in
	unsigned int uLineNum = 0;
	MCString msModules;          // Modules are queued for later loading

	std::list<CGlobalModuleConfigLine> lGlobalModuleConfigLine;

	while (File.ReadLine(sLine)) {
		uLineNum++;

		// Remove all leading spaces and trailing line endings
		sLine.TrimLeft();
		sLine.TrimRight("\r\n");

		if ((sLine.empty()) || (sLine[0] == '#') || (sLine.Left(2) == "//")) {
			continue;
		}

		if (sLine.Left(2) == "/*") {
			if (sLine.Right(2) != "*/") {
				bCommented = true;
			}

			continue;
		}

		if (bCommented) {
			if (sLine.Right(2) == "*/") {
				bCommented = false;
			}

			continue;
		}

		if ((sLine.Left(1) == "<") && (sLine.Right(1) == ">")) {
			sLine.LeftChomp();
			sLine.RightChomp();
			sLine.Trim();

			CString sTag = sLine.substr(0, sLine.find_first_of(" \t\r\n"));
			CString sValue = (sTag.size() < sLine.size()) ? sLine.substr(sTag.size() +1) : "";

			sTag.Trim();
			sValue.Trim();

			if (sLine.Left(1) == "/") {
				sTag = sTag.substr(1);

				if (pUser) {
					if (pChan) {
						if (sTag.Equals("Chan")) {
							// Save the channel name, because AddChan
							// deletes the CChannel*, if adding fails
							sError = pChan->GetName();
							if (!pUser->AddChan(pChan)) {
								sError = "Channel [" + sError + "] defined more than once";
								CUtils::PrintError(sError);
								return false;
							}
							sError.clear();
							pChan = NULL;
							continue;
						}
					} else if (sTag.Equals("User")) {
						CString sErr;

						if (pRealUser) {
							if (!pRealUser->Clone(*pUser, sErr)
									|| !AddUser(pRealUser, sErr)) {
								sError = "Invalid user [" + pUser->GetUserName() + "] " + sErr;
								DEBUG("CUser::Clone() failed in rehash");
							}
							pUser->SetBeingDeleted(true);
							delete pUser;
							pUser = NULL;
						} else if (!AddUser(pUser, sErr)) {
							sError = "Invalid user [" + pUser->GetUserName() + "] " + sErr;
						}

						if (!sError.empty()) {
							CUtils::PrintError(sError);
							if (pUser) {
								pUser->SetBeingDeleted(true);
								delete pUser;
								pUser = NULL;
							}
							return false;
						}

						pUser = NULL;
						pRealUser = NULL;
						continue;
					}
				}
			} else if (sTag.Equals("User")) {
				if (pUser) {
					sError = "You may not nest <User> tags inside of other <User> tags.";
					CUtils::PrintError(sError);
					return false;
				}

				if (sValue.empty()) {
					sError = "You must supply a username in the <User> tag.";
					CUtils::PrintError(sError);
					return false;
				}

				if (m_msUsers.find(sValue) != m_msUsers.end()) {
					sError = "User [" + sValue + "] defined more than once.";
					CUtils::PrintError(sError);
					return false;
				}

				CUtils::PrintMessage("Loading user [" + sValue + "]");

				// Either create a CUser* or use an existing one
				map<CString, CUser*>::iterator it = m_msDelUsers.find(sValue);

				if (it != m_msDelUsers.end()) {
					pRealUser = it->second;
					m_msDelUsers.erase(it);
				} else
					pRealUser = NULL;

				pUser = new CUser(sValue);

				if (!m_sStatusPrefix.empty()) {
					if (!pUser->SetStatusPrefix(m_sStatusPrefix)) {
						sError = "Invalid StatusPrefix [" + m_sStatusPrefix + "] Must be 1-5 chars, no spaces.";
						CUtils::PrintError(sError);
						return false;
					}
				}

				continue;
			} else if (sTag.Equals("Chan")) {
				if (!pUser) {
					sError = "<Chan> tags must be nested inside of a <User> tag.";
					CUtils::PrintError(sError);
					return false;
				}

				if (pChan) {
					sError = "You may not nest <Chan> tags inside of other <Chan> tags.";
					CUtils::PrintError(sError);
					return false;
				}

				pChan = new CChan(sValue, pUser, true);
				continue;
			}
		}

		// If we have a regular line, figure out where it goes
		CString sName = sLine.Token(0, false, "=");
		CString sValue = sLine.Token(1, true, "=");

		// Only remove the first space, people might want
		// leading spaces (e.g. in the MOTD).
		if (sValue.Left(1) == " ")
			sValue.LeftChomp();

		// We don't have any names with spaces, trim all
		// leading/trailing spaces.
		sName.Trim();

		if ((!sName.empty()) && (!sValue.empty())) {
			if (pUser) {
				if (pChan) {
					if (sName.Equals("Buffer")) {
						pChan->SetBufferCount(sValue.ToUInt(), true);
						continue;
					} else if (sName.Equals("KeepBuffer")) {
						pChan->SetKeepBuffer(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("Detached")) {
						pChan->SetDetached(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("AutoCycle")) {
						if (sValue.Equals("true")) {
							CUtils::PrintError("WARNING: AutoCycle has been removed, instead try -> LoadModule = autocycle " + pChan->GetName());
						}
						continue;
					} else if (sName.Equals("Key")) {
						pChan->SetKey(sValue);
						continue;
					} else if (sName.Equals("Modes")) {
						pChan->SetDefaultModes(sValue);
						continue;
					}
				} else {
					if (sName.Equals("Buffer")) {
						pUser->SetBufferCount(sValue.ToUInt(), true);
						continue;
					} else if (sName.Equals("KeepBuffer")) {
						pUser->SetKeepBuffer(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("Nick")) {
						pUser->SetNick(sValue);
						continue;
					} else if (sName.Equals("CTCPReply")) {
						pUser->AddCTCPReply(sValue.Token(0), sValue.Token(1, true));
						continue;
					} else if (sName.Equals("QuitMsg")) {
						pUser->SetQuitMsg(sValue);
						continue;
					} else if (sName.Equals("AltNick")) {
						pUser->SetAltNick(sValue);
						continue;
					} else if (sName.Equals("AwaySuffix")) {
						CUtils::PrintMessage("WARNING: AwaySuffix has been depricated, instead try -> LoadModule = awaynick %nick%_" + sValue);
						continue;
					} else if (sName.Equals("AutoCycle")) {
						if (sValue.Equals("true")) {
							CUtils::PrintError("WARNING: AutoCycle has been removed, instead try -> LoadModule = autocycle");
						}
						continue;
					} else if (sName.Equals("Pass")) {
						// There are different formats for this available:
						// Pass = <plain text>
						// Pass = <md5 hash> -
						// Pass = plain#<plain text>
						// Pass = <hash name>#<hash>
						// Pass = <hash name>#<salted hash>#<salt>#
						// 'Salted hash' means hash of 'password' + 'salt'
						// Possible hashes are md5 and sha256
						if (sValue.Right(1) == "-") {
							sValue.RightChomp();
							sValue.Trim();
							pUser->SetPass(sValue, CUser::HASH_MD5);
						} else {
							CString sMethod = sValue.Token(0, false, "#");
							CString sPass = sValue.Token(1, true, "#");
							if (sMethod == "md5" || sMethod == "sha256") {
								CUser::eHashType type = CUser::HASH_MD5;
								if (sMethod == "sha256")
									type = CUser::HASH_SHA256;

								CString sSalt = sPass.Token(1, false, "#");
								sPass = sPass.Token(0, false, "#");
								pUser->SetPass(sPass, type, sSalt);
							} else if (sMethod == "plain") {
								pUser->SetPass(sPass, CUser::HASH_NONE);
							} else {
								pUser->SetPass(sValue, CUser::HASH_NONE);
							}
						}

						continue;
					} else if (sName.Equals("MultiClients")) {
						pUser->SetMultiClients(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("BounceDCCs")) {
						pUser->SetBounceDCCs(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("Ident")) {
						pUser->SetIdent(sValue);
						continue;
					} else if (sName.Equals("DenyLoadMod")) {
						pUser->SetDenyLoadMod(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("Admin")) {
						pUser->SetAdmin(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("DenySetBindHost") || sName.Equals("DenySetVHost")) {
						pUser->SetDenySetBindHost(sValue.Equals("true"));
						continue;
					} else if (sName.Equals("StatusPrefix")) {
						if (!pUser->SetStatusPrefix(sValue)) {
							sError = "Invalid StatusPrefix [" + sValue + "] Must be 1-5 chars, no spaces.";
							CUtils::PrintError(sError);
							return false;
						}
						continue;
					} else if (sName.Equals("DCCLookupMethod")) {
						pUser->SetUseClientIP(sValue.Equals("Client"));
						continue;
					} else if (sName.Equals("RealName")) {
						pUser->SetRealName(sValue);
						continue;
					} else if (sName.Equals("KeepNick")) {
						if (sValue.Equals("true")) {
							CUtils::PrintError("WARNING: KeepNick has been deprecated, instead try -> LoadModule = keepnick");
						}
						continue;
					} else if (sName.Equals("ChanModes")) {
						pUser->SetDefaultChanModes(sValue);
						continue;
					} else if (sName.Equals("BindHost") || sName.Equals("VHost")) {
						pUser->SetBindHost(sValue);
						continue;
					} else if (sName.Equals("DCCBindHost") || sName.Equals("DCCVHost")) {
						pUser->SetDCCBindHost(sValue);
						continue;
					} else if (sName.Equals("Allow")) {
						pUser->AddAllowedHost(sValue);
						continue;
					} else if (sName.Equals("Server")) {
						CUtils::PrintAction("Adding Server [" + sValue + "]");
						CUtils::PrintStatus(pUser->AddServer(sValue));
						continue;
					} else if (sName.Equals("Chan")) {
						pUser->AddChan(sValue, true);
						continue;
					} else if (sName.Equals("TimestampFormat")) {
						pUser->SetTimestampFormat(sValue);
						continue;
					} else if (sName.Equals("AppendTimestamp")) {
						pUser->SetTimestampAppend(sValue.ToBool());
						continue;
					} else if (sName.Equals("PrependTimestamp")) {
						pUser->SetTimestampPrepend(sValue.ToBool());
						continue;
					} else if (sName.Equals("IRCConnectEnabled")) {
						pUser->SetIRCConnectEnabled(sValue.ToBool());
						continue;
					} else if (sName.Equals("Timestamp")) {
						if (!sValue.Trim_n().Equals("true")) {
							if (sValue.Trim_n().Equals("append")) {
								pUser->SetTimestampAppend(true);
								pUser->SetTimestampPrepend(false);
							} else if (sValue.Trim_n().Equals("prepend")) {
								pUser->SetTimestampAppend(false);
								pUser->SetTimestampPrepend(true);
							} else if (sValue.Trim_n().Equals("false")) {
								pUser->SetTimestampAppend(false);
								pUser->SetTimestampPrepend(false);
							} else {
								pUser->SetTimestampFormat(sValue);
							}
						}
						continue;
					} else if (sName.Equals("TimezoneOffset")) {
						pUser->SetTimezoneOffset(sValue.ToDouble()); // there is no ToFloat()
						continue;
					} else if (sName.Equals("JoinTries")) {
						pUser->SetJoinTries(sValue.ToUInt());
						continue;
					} else if (sName.Equals("MaxJoins")) {
						pUser->SetMaxJoins(sValue.ToUInt());
						continue;
					} else if (sName.Equals("Skin")) {
						pUser->SetSkinName(sValue);
						continue;
					} else if (sName.Equals("LoadModule")) {
						CString sModName = sValue.Token(0);

						// XXX Legacy crap, added in znc 0.089
						if (sModName == "discon_kick") {
							CUtils::PrintMessage("NOTICE: [discon_kick] was renamed, loading [disconkick] instead");
							sModName = "disconkick";
						}

						CUtils::PrintAction("Loading Module [" + sModName + "]");
						CString sModRet;
						CString sArgs = sValue.Token(1, true);

						bool bModRet = pUser->GetModules().LoadModule(sModName, sArgs, pUser, sModRet);

						// If the module was loaded, sModRet contains
						// "Loaded Module [name] ..." and we strip away this beginning.
						if (bModRet)
							sModRet = sModRet.Token(1, true, sModName + "] ");

						CUtils::PrintStatus(bModRet, sModRet);
						if (!bModRet) {
							sError = sModRet;
							return false;
						}
						continue;
					}
				}
			} else {
				if (sName.Equals("Listen") || sName.Equals("Listen6") || sName.Equals("Listen4")
						|| sName.Equals("Listener") || sName.Equals("Listener6") || sName.Equals("Listener4")) {
					EAddrType eAddr = ADDR_ALL;
					if (sName.Equals("Listen4") || sName.Equals("Listen") || sName.Equals("Listener4")) {
						eAddr = ADDR_IPV4ONLY;
					}
					if (sName.Equals("Listener6")) {
						eAddr = ADDR_IPV6ONLY;
					}

					CListener::EAcceptType eAccept = CListener::ACCEPT_ALL;
					if (sValue.TrimPrefix("irc_only "))
						eAccept = CListener::ACCEPT_IRC;
					else if (sValue.TrimPrefix("web_only "))
						eAccept = CListener::ACCEPT_HTTP;

					bool bSSL = false;
					CString sPort;
					CString sBindHost;

					if (ADDR_IPV4ONLY == eAddr) {
						sValue.Replace(":", " ");
					}

					if (sValue.find(" ") != CString::npos) {
						sBindHost = sValue.Token(0, false, " ");
						sPort = sValue.Token(1, true, " ");
					} else {
						sPort = sValue;
					}

					if (sPort.Left(1) == "+") {
						sPort.LeftChomp();
						bSSL = true;
					}

					CString sHostComment;

					if (!sBindHost.empty()) {
						sHostComment = " on host [" + sBindHost + "]";
					}

					CString sIPV6Comment;

					switch (eAddr) {
						case ADDR_ALL:
							sIPV6Comment = "";
							break;
						case ADDR_IPV4ONLY:
							sIPV6Comment = " using ipv4";
							break;
						case ADDR_IPV6ONLY:
							sIPV6Comment = " using ipv6";
					}

					unsigned short uPort = sPort.ToUShort();
					CUtils::PrintAction("Binding to port [" + CString((bSSL) ? "+" : "") + CString(uPort) + "]" + sHostComment + sIPV6Comment);

#ifndef HAVE_IPV6
					if (ADDR_IPV6ONLY == eAddr) {
						sError = "IPV6 is not enabled";
						CUtils::PrintStatus(false, sError);
						return false;
					}
#endif

#ifndef HAVE_LIBSSL
					if (bSSL) {
						sError = "SSL is not enabled";
						CUtils::PrintStatus(false, sError);
						return false;
					}
#else
					CString sPemFile = GetPemLocation();

					if (bSSL && !CFile::Exists(sPemFile)) {
						sError = "Unable to locate pem file: [" + sPemFile + "]";
						CUtils::PrintStatus(false, sError);

						// If stdin is e.g. /dev/null and we call GetBoolInput(),
						// we are stuck in an endless loop!
						if (isatty(0) && CUtils::GetBoolInput("Would you like to create a new pem file?", true)) {
							sError.clear();
							WritePemFile();
						} else {
							return false;
						}

						CUtils::PrintAction("Binding to port [+" + CString(uPort) + "]" + sHostComment + sIPV6Comment);
					}
#endif
					if (!uPort) {
						sError = "Invalid port";
						CUtils::PrintStatus(false, sError);
						return false;
					}

					CListener* pListener = new CListener(uPort, sBindHost, bSSL, eAddr, eAccept);

					if (!pListener->Listen()) {
#ifdef _WIN32
						if(!CUtils::Win32StringError(::WSAGetLastError(), sError))
							sError = "unknown error, check the host name";
#else
						sError = (errno == 0 ? CString("unknown error, check the host name") : CString(strerror(errno)));
#endif
						sError = "Unable to bind [" + sError + "]";
						CUtils::PrintStatus(false, sError);
						delete pListener;
						return false;
					}

					m_vpListeners.push_back(pListener);
					CUtils::PrintStatus(true);

					continue;
				} else if (sName.Equals("LoadModule")) {
					CString sModName = sValue.Token(0);
					CString sArgs = sValue.Token(1, true);

					if (msModules.find(sModName) != msModules.end()) {
						sError = "Module [" + sModName +
							"] already loaded";
						CUtils::PrintError(sError);
						return false;
					}
					msModules[sModName] = sArgs;
					continue;
				} else if (sName.Equals("ISpoofFormat")) {
					m_sISpoofFormat = sValue;
					continue;
				} else if (sName.Equals("ISpoofFile")) {
					if (sValue.Left(2) == "~/") {
						sValue.LeftChomp(2);
						sValue = GetHomePath() + "/" + sValue;
					}
					m_sISpoofFile = sValue;
					continue;
				} else if (sName.Equals("MOTD")) {
					AddMotd(sValue);
					continue;
				} else if (sName.Equals("BindHost") || sName.Equals("VHost")) {
					AddBindHost(sValue);
					continue;
				} else if (sName.Equals("PidFile")) {
					m_sPidFile = sValue;
					continue;
				} else if (sName.Equals("Skin")) {
					SetSkinName(sValue);
					continue;
				} else if (sName.Equals("StatusPrefix")) {
					m_sStatusPrefix = sValue;
					continue;
				} else if (sName.Equals("ConnectDelay")) {
					m_uiConnectDelay = sValue.ToUInt();
					continue;
				} else if (sName.Equals("ServerThrottle")) {
					m_sConnectThrottle.SetTTL(sValue.ToUInt()*1000);
					continue;
				} else if (sName.Equals("AnonIPLimit")) {
					m_uiAnonIPLimit = sValue.ToUInt();
					continue;
				} else if (sName.Equals("MaxBufferSize")) {
					m_uiMaxBufferSize = sValue.ToUInt();
					continue;
				} else if (sName.Equals("SSLCertFile")) {
					m_sSSLCertFile = sValue;
					continue;
				}
			}

		}

		if (sName.Equals("GM:", false, 3))
		{ // GM: prefix is a pass through to config lines for global modules
			CGlobalModuleConfigLine cTmp;
			cTmp.m_sName = sName.substr(3, CString::npos);
			cTmp.m_sValue = sValue;
			cTmp.m_pChan = pChan;
			cTmp.m_pUser = pUser;
			lGlobalModuleConfigLine.push_back(cTmp);
		}
		else
		{
			sError = "Unhandled line " + CString(uLineNum) + " in config: [" + sLine + "]";
			CUtils::PrintError(sError);
			return false;
		}
	}

	// First step: Load and reload new modules or modules with new arguments
	for (MCString::iterator it = msModules.begin(); it != msModules.end(); ++it) {
		CString sModName = it->first;
		CString sArgs = it->second;
		CString sModRet;
		CModule *pOldMod;

		pOldMod = GetModules().FindModule(sModName);
		if (!pOldMod) {
			CUtils::PrintAction("Loading Global Module [" + sModName + "]");

			bool bModRet = GetModules().LoadModule(sModName, sArgs, NULL, sModRet);

			// If the module was loaded, sModRet contains
			// "Loaded Module [name] ..." and we strip away this beginning.
			if (bModRet)
				sModRet = sModRet.Token(1, true, sModName + "] ");

			CUtils::PrintStatus(bModRet, sModRet);
			if (!bModRet) {
				sError = sModRet;
				return false;
			}
		} else if (pOldMod->GetArgs() != sArgs) {
			CUtils::PrintAction("Reloading Global Module [" + sModName + "]");

			bool bModRet = GetModules().ReloadModule(sModName, sArgs, NULL, sModRet);

			// If the module was loaded, sModRet contains
			// "Loaded Module [name] ..." and we strip away this beginning.
			if (bModRet)
				sModRet = sModRet.Token(1, true, sModName + "] ");

			CUtils::PrintStatus(bModRet, sModRet);
			if (!bModRet) {
				sError = sModRet;
				return false;
			}
		} else
			CUtils::PrintMessage("Module [" + sModName + "] already loaded.");
	}

	// Second step: Unload modules which are no longer in the config
	set<CString> ssUnload;
	for (size_t i = 0; i < GetModules().size(); i++) {
		CModule *pCurMod = GetModules()[i];

		if (msModules.find(pCurMod->GetModName()) == msModules.end())
			ssUnload.insert(pCurMod->GetModName());
	}

	for (set<CString>::iterator it = ssUnload.begin(); it != ssUnload.end(); ++it) {
		if (GetModules().UnloadModule(*it))
			CUtils::PrintMessage("Unloaded Global Module [" + *it + "]");
		else
			CUtils::PrintMessage("Could not unload [" + *it + "]");
	}

	// last step, throw unhandled config items at global config
	for (std::list<CGlobalModuleConfigLine>::iterator it = lGlobalModuleConfigLine.begin(); it != lGlobalModuleConfigLine.end(); ++it)
	{
		if ((pChan && pChan == it->m_pChan) || (pUser && pUser == it->m_pUser))
			continue; // skip unclosed user or chan
		bool bHandled = false;
		if (it->m_pUser) {
			MODULECALL(OnConfigLine(it->m_sName, it->m_sValue, it->m_pUser, it->m_pChan), it->m_pUser, NULL, bHandled = true);
		} else {
			GLOBALMODULECALL(OnConfigLine(it->m_sName, it->m_sValue, it->m_pUser, it->m_pChan), it->m_pUser, NULL, bHandled = true);
		}
		if (!bHandled) {
			CUtils::PrintMessage("unhandled global module config line [GM:" + it->m_sName + "] = [" + it->m_sValue + "]");
		}
	}

	if (pChan) {
		sError = "Last <Chan> section not properly closed. File truncated?";
		CUtils::PrintError(sError);
		delete pChan;
		return false;
	}

	if (pUser) {
		sError = "Last <User> section not properly closed. File truncated?";
		CUtils::PrintError(sError);
		delete pUser;
		return false;
	}

	if (m_msUsers.empty()) {
		sError = "You must define at least one user in your config.";
		CUtils::PrintError(sError);
		return false;
	}

	if (m_vpListeners.empty()) {
		sError = "You must supply at least one Listen port in your config.";
		CUtils::PrintError(sError);
		return false;
	}

	// Make sure that users that want to connect do so and also make sure a
	// new ConnectDelay setting is applied.
	DisableConnectUser();
	EnableConnectUser();

	return true;
}

void CZNC::ClearBindHosts() {
	m_vsBindHosts.clear();
}

bool CZNC::AddBindHost(const CString& sHost) {
	if (sHost.empty()) {
		return false;
	}

	for (unsigned int a = 0; a < m_vsBindHosts.size(); a++) {
		if (m_vsBindHosts[a].Equals(sHost)) {
			return false;
		}
	}

	m_vsBindHosts.push_back(sHost);
	return true;
}

bool CZNC::RemBindHost(const CString& sHost) {
	VCString::iterator it;
	for (it = m_vsBindHosts.begin(); it != m_vsBindHosts.end(); ++it) {
		if (sHost.Equals(*it)) {
			m_vsBindHosts.erase(it);
			return true;
		}
	}

	return false;
}

void CZNC::Broadcast(const CString& sMessage, bool bAdminOnly,
		CUser* pSkipUser, CClient *pSkipClient) {
	for (map<CString,CUser*>::iterator a = m_msUsers.begin(); a != m_msUsers.end(); ++a) {
		if (bAdminOnly && !a->second->IsAdmin())
			continue;

		if (a->second != pSkipUser) {
			CString sMsg = sMessage;

			MODULECALL(OnBroadcast(sMsg), a->second, NULL, continue);
			a->second->PutStatusNotice("*** " + sMsg, NULL, pSkipClient);
		}
	}
}

CModule* CZNC::FindModule(const CString& sModName, const CString& sUsername) {
	if (sUsername.empty()) {
		return CZNC::Get().GetModules().FindModule(sModName);
	}

	CUser* pUser = FindUser(sUsername);

	return (!pUser) ? NULL : pUser->GetModules().FindModule(sModName);
}

CModule* CZNC::FindModule(const CString& sModName, CUser* pUser) {
	if (pUser) {
		return pUser->GetModules().FindModule(sModName);
	}

	return CZNC::Get().GetModules().FindModule(sModName);
}

CUser* CZNC::FindUser(const CString& sUsername) {
	map<CString,CUser*>::iterator it = m_msUsers.find(sUsername);

	if (it != m_msUsers.end()) {
		return it->second;
	}

	return NULL;
}

bool CZNC::DeleteUser(const CString& sUsername) {
	CUser* pUser = FindUser(sUsername);

	if (!pUser) {
		return false;
	}

	m_msDelUsers[pUser->GetUserName()] = pUser;
	return true;
}

bool CZNC::AddUser(CUser* pUser, CString& sErrorRet) {
	if (FindUser(pUser->GetUserName()) != NULL) {
		sErrorRet = "User already exists";
		DEBUG("User [" << pUser->GetUserName() << "] - already exists");
		return false;
	}
	if (!pUser->IsValid(sErrorRet)) {
		DEBUG("Invalid user [" << pUser->GetUserName() << "] - ["
				<< sErrorRet << "]");
		return false;
	}
	GLOBALMODULECALL(OnAddUser(*pUser, sErrorRet), pUser, NULL,
		DEBUG("AddUser [" << pUser->GetUserName() << "] aborted by a module ["
			<< sErrorRet << "]");
		return false;
	);
	m_msUsers[pUser->GetUserName()] = pUser;
	return true;
}

CListener* CZNC::FindListener(u_short uPort, const CString& sBindHost, EAddrType eAddr) {
	vector<CListener*>::iterator it;

	for (it = m_vpListeners.begin(); it < m_vpListeners.end(); ++it) {
		if ((*it)->GetPort() != uPort)
			continue;
		if ((*it)->GetBindHost() != sBindHost)
			continue;
		if ((*it)->GetAddrType() != eAddr)
			continue;
		return *it;
	}
	return NULL;
}

bool CZNC::AddListener(CListener* pListener) {
	if (!pListener->GetRealListener()) {
		// Listener doesnt actually listen
		delete pListener;
		return false;
	}

	// We don't check if there is an identical listener already listening
	// since one can't listen on e.g. the same port multiple times

	m_vpListeners.push_back(pListener);
	return true;
}

bool CZNC::DelListener(CListener* pListener) {
	vector<CListener*>::iterator it;

	for (it = m_vpListeners.begin(); it < m_vpListeners.end(); ++it) {
		if (*it == pListener) {
			m_vpListeners.erase(it);
			delete pListener;
			return true;
		}
	}

	return false;
}

static CZNC* g_pZNC = NULL;
CZNC& CZNC::Get() {
	if(!g_pZNC) {
		g_pZNC = new CZNC();
	}
	return *g_pZNC;
}
void CZNC::_Reset() {
	if(g_pZNC) delete g_pZNC;
	g_pZNC = NULL;
}

CZNC::TrafficStatsMap CZNC::GetTrafficStats(TrafficStatsPair &Users,
			TrafficStatsPair &ZNC, TrafficStatsPair &Total) {
	TrafficStatsMap ret;
	unsigned long long uiUsers_in, uiUsers_out, uiZNC_in, uiZNC_out;
	const map<CString, CUser*>& msUsers = CZNC::Get().GetUserMap();

	uiUsers_in = uiUsers_out = 0;
	uiZNC_in  = BytesRead();
	uiZNC_out = BytesWritten();

	for (map<CString, CUser*>::const_iterator it = msUsers.begin(); it != msUsers.end(); ++it) {
		ret[it->first] = TrafficStatsPair(it->second->BytesRead(), it->second->BytesWritten());
		uiUsers_in  += it->second->BytesRead();
		uiUsers_out += it->second->BytesWritten();
	}

	for (CSockManager::const_iterator it = m_Manager.begin(); it != m_Manager.end(); ++it) {
		CUser *pUser = NULL;
		if ((*it)->GetSockName().Left(5) == "IRC::") {
			pUser = ((CIRCSock *) *it)->GetUser();
		} else if ((*it)->GetSockName().Left(5) == "USR::") {
			pUser = ((CClient*) *it)->GetUser();
		}

		if (pUser) {
			ret[pUser->GetUserName()].first  += (*it)->GetBytesRead();
			ret[pUser->GetUserName()].second += (*it)->GetBytesWritten();
			uiUsers_in  += (*it)->GetBytesRead();
			uiUsers_out += (*it)->GetBytesWritten();
		} else {
			uiZNC_in  += (*it)->GetBytesRead();
			uiZNC_out += (*it)->GetBytesWritten();
		}
	}

	Users = TrafficStatsPair(uiUsers_in, uiUsers_out);
	ZNC   = TrafficStatsPair(uiZNC_in, uiZNC_out);
	Total = TrafficStatsPair(uiUsers_in + uiZNC_in, uiUsers_out + uiZNC_out);

	return ret;
}

void CZNC::AuthUser(CSmartPtr<CAuthBase> AuthClass) {
	// TODO unless the auth module calls it, CUser::IsHostAllowed() is not honoured
	GLOBALMODULECALL(OnLoginAttempt(AuthClass), NULL, NULL, return);

	CUser* pUser = FindUser(AuthClass->GetUsername());

	if (!pUser || !pUser->CheckPass(AuthClass->GetPassword())) {
		AuthClass->RefuseLogin("Invalid Password");
		return;
	}

	CString sHost = AuthClass->GetRemoteIP();

	if (!pUser->IsHostAllowed(sHost)) {
		AuthClass->RefuseLogin("Your host [" + sHost + "] is not allowed");
		return;
	}

	AuthClass->AcceptLogin(*pUser);
}

class CConnectUserTimer : public CCron {
public:
	CConnectUserTimer(int iSecs) : CCron() {
		SetName("Connect users");
		Start(iSecs);
		m_uiPosNextUser = 0;
		// Don't wait iSecs seconds for first timer run
		m_bRunOnNextCall = true;
	}
	virtual ~CConnectUserTimer() {
		// This is only needed when ZNC shuts down:
		// CZNC::~CZNC() sets its CConnectUserTimer pointer to NULL and
		// calls the manager's Cleanup() which destroys all sockets and
		// timers. If something calls CZNC::EnableConnectUser() here
		// (e.g. because a CIRCSock is destroyed), the socket manager
		// deletes that timer almost immediately, but CZNC now got a
		// dangling pointer to this timer which can crash later on.
		//
		// Unlikely but possible ;)
		CZNC::Get().LeakConnectUser(this);
	}

protected:
	virtual void RunJob() {
		size_t uiUserCount;
		bool bUsersLeft = false;
		const map<CString,CUser*>& mUsers = CZNC::Get().GetUserMap();
		map<CString,CUser*>::const_iterator it = mUsers.begin();

		uiUserCount = CZNC::Get().GetUserMap().size();

		if (m_uiPosNextUser >= uiUserCount) {
			m_uiPosNextUser = 0;
		}

		for (size_t i = 0; i < m_uiPosNextUser; i++) {
			it++;
		}

		// Try to connect each user, if this doesnt work, abort
		for (size_t i = 0; i < uiUserCount; i++) {
			if (it == mUsers.end())
				it = mUsers.begin();

			CUser* pUser = it->second;
			it++;
			m_uiPosNextUser = (m_uiPosNextUser + 1) % uiUserCount;

			// Is this user disconnected?
			if (pUser->GetIRCSock() != NULL)
				continue;

			// Does this user want to connect?
			if (!pUser->GetIRCConnectEnabled())
				continue;

			// Does this user have any servers?
			if (!pUser->HasServers())
				continue;

			// The timer runs until it once didn't find any users to connect
			bUsersLeft = true;

			DEBUG("Connecting user [" << pUser->GetUserName() << "]");

			if (CZNC::Get().ConnectUser(pUser))
				// User connecting, wait until next time timer fires
				return;
		}

		if (bUsersLeft == false) {
			DEBUG("ConnectUserTimer done");
			CZNC::Get().DisableConnectUser();
		}
	}

private:
	size_t m_uiPosNextUser;
};

void CZNC::EnableConnectUser() {
	if (m_pConnectUserTimer != NULL)
		return;

	m_pConnectUserTimer = new CConnectUserTimer(m_uiConnectDelay);
	GetManager().AddCron(m_pConnectUserTimer);
}

void CZNC::DisableConnectUser() {
	if (m_pConnectUserTimer == NULL)
		return;

	// This will kill the cron
	m_pConnectUserTimer->Stop();
	m_pConnectUserTimer = NULL;
}

// it is important that this method doesn't have its body in an .h file.
double CZNC::GetCoreVersion()
{
	return MODVERSION;
}

void CZNC::LeakConnectUser(CConnectUserTimer *pTimer) {
	if (m_pConnectUserTimer == pTimer)
		m_pConnectUserTimer = NULL;
}

bool CZNC::WaitForChildLock() {
	return m_LockFile.ExLock();
}
