/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_H
#define _ZNC_H

#include "main.h"
#include "Client.h"
#include "FileUtils.h"
#include "Modules.h"
#include "Socket.h"
#include <map>

using std::map;

class CListener;
class CUser;
class CConnectUserTimer;

class ZNC_API CZNC {
public:
	CZNC();
	~CZNC();

	enum ConfigState {
		ECONFIG_NOTHING,
		ECONFIG_NEED_REHASH,
		ECONFIG_NEED_WRITE
	};

	void DeleteUsers();
	void LoopDoMaintenance();
#ifdef _WIN32
	void Loop(bool* bLoop);
#else
	void Loop();
#endif
	bool WriteISpoof(CUser* pUser);
	void ReleaseISpoof();
	bool WritePidFile(int iPid);
	bool DeletePidFile();
	bool WaitForChildLock();
	Csock* FindSockByName(const CString& sSockName);
	bool IsHostAllowed(const CString& sHostMask) const;
	// This returns false if there are too many anonymous connections from this ip
	bool AllowConnectionFrom(const CString& sIP) const;
	void InitDirs(const CString& sArgvPath, const CString& sDataDir);
	bool OnBoot();
	CString ExpandConfigPath(const CString& sConfigFile, bool bAllowMkDir = true);
	bool WriteNewConfig(const CString& sConfigFile);
	bool WriteConfig();
	bool ParseConfig(const CString& sConfig);
	bool RehashConfig(CString& sError);
	static CString GetVersion();
	static CString GetTag(bool bIncludeVersion = true);
	CString GetUptime() const;
	static double GetCoreVersion();
	void ClearBindHosts();
	bool AddBindHost(const CString& sHost);
	bool RemBindHost(const CString& sHost);
	void Broadcast(const CString& sMessage, bool bAdminOnly = false,
			CUser* pSkipUser = NULL, CClient* pSkipClient = NULL);
	void AddBytesRead(unsigned long long u) { m_uBytesRead += u; }
	void AddBytesWritten(unsigned long long u) { m_uBytesWritten += u; }
	unsigned long long BytesRead() const { return m_uBytesRead; }
	unsigned long long BytesWritten() const { return m_uBytesWritten; }

	// Traffic fun
	typedef std::pair<unsigned long long, unsigned long long> TrafficStatsPair;
	typedef std::map<CString, TrafficStatsPair> TrafficStatsMap;
	// Returns a map which maps user names to <traffic in, traffic out>
	// while also providing the traffic of all users together, traffic which
	// couldn't be accounted to any particular user and the total traffic
	// generated through ZNC.
	TrafficStatsMap GetTrafficStats(TrafficStatsPair &Users,
			TrafficStatsPair &ZNC, TrafficStatsPair &Total);

	// Authenticate a user.
	// The result is passed back via callbacks to CAuthBase.
	// CSmartPtr handles freeing this pointer!
	void AuthUser(CSmartPtr<CAuthBase> AuthClass);

	// Setters
	void SetConfigState(enum ConfigState e) { m_eConfigState = e; }
	void SetSkinName(const CString& s) { m_sSkinName = s; }
	void SetStatusPrefix(const CString& s) { m_sStatusPrefix = (s.empty()) ? "*" : s; }
	void SetISpoofFile(const CString& s) { m_sISpoofFile = s; }
	void SetISpoofFormat(const CString& s) { m_sISpoofFormat = (s.empty()) ? "global { reply \"%\" }" : s; }
	void SetMaxBufferSize(size_t i) { m_uiMaxBufferSize = i; }
	// !Setters

	// Getters
	enum ConfigState GetConfigState() const { return m_eConfigState; }
	CSockManager& GetManager() { return m_Manager; }
	const CSockManager& GetManager() const { return m_Manager; }
	CGlobalModules& GetModules() { return *m_pModules; }
	size_t FilterUncommonModules(set<CModInfo>& ssModules);
	CString GetSkinName() const { return m_sSkinName; }
	const CString& GetStatusPrefix() const { return m_sStatusPrefix; }
	const CString& GetCurPath() const { if (!CFile::Exists(m_sCurPath)) { CDir::MakeDir(m_sCurPath); } return m_sCurPath; }
	const CString& GetHomePath() const { if (!CFile::Exists(m_sHomePath)) { CDir::MakeDir(m_sHomePath); } return m_sHomePath; }
	const CString& GetZNCPath() const { if (!CFile::Exists(m_sZNCPath)) { CDir::MakeDir(m_sZNCPath); } return m_sZNCPath; }
	CString GetConfPath(bool bAllowMkDir = true) const;
	CString GetUserPath() const;
	CString GetModPath() const;
	CString GetPemLocation() const { return m_sSSLCertFile; }
	const CString& GetConfigFile() const { return m_sConfigFile; }
	bool WritePemFile();
	const CString& GetISpoofFile() const { return m_sISpoofFile; }
	const CString& GetISpoofFormat() const { return m_sISpoofFormat; }
	const VCString& GetBindHosts() const { return m_vsBindHosts; }
	const vector<CListener*>& GetListeners() const { return m_vpListeners; }
	time_t TimeStarted() const { return m_TimeStarted; }
	size_t GetMaxBufferSize() const { return m_uiMaxBufferSize; }
	// !Getters

	// Static allocator
	static CZNC& Get();
	static void _Reset();
	void HookOutput(outputHook fHook);
	CUser* FindUser(const CString& sUsername);
	CModule* FindModule(const CString& sModName, const CString& sUsername);
	CModule* FindModule(const CString& sModName, CUser* pUser);
	bool DeleteUser(const CString& sUsername);
	bool AddUser(CUser* pUser, CString& sErrorRet);
	const map<CString,CUser*> & GetUserMap() const { return(m_msUsers); }

	// Listener yummy
	CListener* FindListener(u_short uPort, const CString& BindHost, EAddrType eAddr);
	bool AddListener(CListener*);
	bool DelListener(CListener*);

	// Message of the Day
	void SetMotd(const CString& sMessage) { ClearMotd(); AddMotd(sMessage); }
	void AddMotd(const CString& sMessage) { if (!sMessage.empty()) { m_vsMotd.push_back(sMessage); } }
	void ClearMotd() { m_vsMotd.clear(); }
	const VCString& GetMotd() const { return m_vsMotd; }
	// !MOTD

	// Create a CIRCSocket. Return false if user cant connect
	bool ConnectUser(CUser *pUser);
	// This creates a CConnectUserTimer if we haven't got one yet
	void EnableConnectUser();
	void DisableConnectUser();

	// Never call this unless you are CConnectUserTimer::~CConnectUserTimer()
	void LeakConnectUser(CConnectUserTimer *pTimer);

private:
	CFile* InitPidFile();
	bool DoRehash(CString& sError);
	// Returns true if something was done
	bool HandleUserDeletion();

protected:
	time_t                 m_TimeStarted;

	enum ConfigState       m_eConfigState;
	vector<CListener*>     m_vpListeners;
	map<CString,CUser*>    m_msUsers;
	map<CString,CUser*>    m_msDelUsers;
	CSockManager           m_Manager;

	CString                m_sCurPath;
	CString                m_sHomePath;
	CString                m_sZNCPath;

	CString                m_sConfigFile;
	CString                m_sSkinName;
	CString                m_sStatusPrefix;
	CString                m_sISpoofFile;
	CString                m_sOrigISpoof;
	CString                m_sISpoofFormat;
	CString                m_sPidFile;
	CString                m_sSSLCertFile;
	VCString               m_vsBindHosts;
	VCString               m_vsMotd;
	CFile                  m_LockFile;
	CFile*                 m_pISpoofLockFile;
	unsigned int           m_uiConnectDelay;
	unsigned int           m_uiAnonIPLimit;
	size_t           m_uiMaxBufferSize;
	CGlobalModules*        m_pModules;
	unsigned long long     m_uBytesRead;
	unsigned long long     m_uBytesWritten;
	CConnectUserTimer     *m_pConnectUserTimer;
	TCacheMap<CString>     m_sConnectThrottle;
};

#endif // !_ZNC_H
