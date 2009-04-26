/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "znc.h"

// Forward Declaration
class CShellMod;

class CShellSock : public CExecSock {
public:
	CShellSock(CShellMod* pShellMod, CClient* pClient, const CString& sExec) : CExecSock() {
		EnableReadLine();
		m_pParent = pShellMod;
		m_pClient = pClient;

		if (Execute(sExec) == -1) {
			CString s = "Failed to execute: ";
			s += strerror(errno);
			ReadLine(s);
			return;
		}

		// Get rid of that write fd, we aren't going to use it
		// (And clients expecting input will fail this way).
		close(GetWSock());
		SetWSock(open("/dev/null", O_WRONLY));
	}
	// These next two function's bodies are at the bottom of the file since they reference CShellMod
	virtual void ReadLine(const CString& sData);
	virtual void Disconnected();

	CShellMod*	m_pParent;

private:
	CClient*	m_pClient;
};

class CShellMod : public CModule {
public:
	MODCONSTRUCTOR(CShellMod) {
		m_sPath = CZNC::Get().GetHomePath();
	}

	virtual ~CShellMod() {
		vector<Csock*> vSocks = m_pManager->FindSocksByName("SHELL");

		for (unsigned int a = 0; a < vSocks.size(); a++) {
			m_pManager->DelSockByAddr(vSocks[a]);
		}
	}

	virtual bool OnLoad(const CString& sArgs, CString& sMessage)
	{
#ifndef MOD_SHELL_ALLOW_EVERYONE
		if (!m_pUser->IsAdmin()) {
			sMessage = "You must be admin to use the shell module";
			return false;
		}
#endif

		return true;
	}

	virtual void OnModCommand(const CString& sLine) {
		CString sCommand = sLine.Token(0);
		if (sCommand.Equals("cd")) {
			CString sArg = sLine.Token(1, true);
			CString sPath = CDir::ChangeDir(m_sPath, (sArg.empty() ? CString(CZNC::Get().GetHomePath()) : sArg), CZNC::Get().GetHomePath());
			CFile Dir(sPath);

			if (Dir.IsDir()) {
				m_sPath = sPath;
			} else if (Dir.Exists()) {
				PutShell("cd: not a directory [" + sPath + "]");
			} else {
				PutShell("cd: no such directory [" + sPath + "]");
			}

			PutShell("znc$");
		} else if (sCommand.Equals("SEND")) {
			CString sToNick = sLine.Token(1);
			CString sFile = sLine.Token(2);

			if ((sToNick.empty()) || (sFile.empty())) {
				PutShell("usage: Send <nick> <file>");
			} else {
				sFile = CDir::ChangeDir(m_sPath, sFile, CZNC::Get().GetHomePath());

				if (!CFile::Exists(sFile)) {
					PutShell("get: no such file [" + sFile + "]");
				} else if (!CFile::IsReg(sFile)) {
					PutShell("get: not a file [" + sFile + "]");
				} else {
					m_pUser->SendFile(sToNick, sFile, GetModName());
				}
			}
		} else if (sCommand.Equals("GET")) {
			CString sFile = sLine.Token(1);

			if (sFile.empty()) {
				PutShell("usage: Get <file>");
			} else {
				sFile = CDir::ChangeDir(m_sPath, sFile, CZNC::Get().GetHomePath());

				if (!CFile::Exists(sFile)) {
					PutShell("get: no such file [" + sFile + "]");
				} else if (!CFile::IsReg(sFile)) {
					PutShell("get: not a file [" + sFile + "]");
				} else {
					m_pUser->SendFile(m_pUser->GetCurNick(), sFile, GetModName());
				}
			}
		} else {
			RunCommand(sLine);
		}
	}

	virtual EModRet OnStatusCommand(CString& sCommand) {
		if (sCommand.Equals("SHELL")) {
			PutShell("-- ZNC Shell Service --");
			return HALT;
		}

		return CONTINUE;
	}

	virtual EModRet OnDCCUserSend(const CNick& RemoteNick, unsigned long uLongIP, unsigned short uPort, const CString& sFile, unsigned long uFileSize) {
		if (RemoteNick.GetNick().Equals(GetModNick())) {
			CString sLocalFile = CDir::ChangeDir(m_sPath, sFile, CZNC::Get().GetHomePath());

			m_pUser->GetFile(m_pUser->GetCurNick(), CUtils::GetIP(uLongIP), uPort, sLocalFile, uFileSize, GetModName());

			return HALT;
		}

		return CONTINUE;
	}

	void PutShell(const CString& sLine) {
		CString sPath = m_sPath;

		CString::size_type a = sPath.find(' ');
		while (a != CString::npos) {
			sPath.replace(a, 1, "_");
			a = sPath.find(' ');
		}

		PutModule(sLine, "shell", sPath);
	}

	void RunCommand(const CString& sCommand) {
		m_pManager->AddSock((Csock*) new CShellSock(this, m_pClient, "cd " + m_sPath + " && " + sCommand), "SHELL");
	}
private:
	CString	m_sPath;
};

void CShellSock::ReadLine(const CString& sData) {
	CString sLine = sData;

	sLine.TrimRight("\r\n");
	sLine.Replace("\t", "    ");

	m_pParent->SetClient(m_pClient);
	m_pParent->PutShell(sLine);
	m_pParent->SetClient(NULL);
}

void CShellSock::Disconnected() {
	// If there is some incomplete line in the buffer, read it
	// (e.g. echo echo -n "hi" triggered this)
	CString &sBuffer = GetInternalReadBuffer();
	if (!sBuffer.empty())
		ReadLine(sBuffer);

	m_pParent->SetClient(m_pClient);
	m_pParent->PutShell("znc$");
	m_pParent->SetClient(NULL);
}

MODULEDEFS(CShellMod, "Gives shell access")

