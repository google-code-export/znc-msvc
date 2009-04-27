/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "Modules.h"
#include "znc.h"
#include "Server.h"

#ifndef _WIN32
#include <syslog.h>
#else
#define LOG_INFO 1
#define LOG_NOTICE 2
#define LOG_WARNING 3
#endif

#include <fstream>
#include <ctime>

using namespace std;

class CAdminLogMod : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CAdminLogMod) {}

	virtual ~CAdminLogMod() {
		Log("Logging ended.");
	}

	virtual bool OnLoad(const CString & sArgs, CString & sMessage) {
		sLogFile = CZNC::Get().GetZNCPath() + "/znc.log";
		Log("Logging started. ZNC PID[" + CString(getpid()) + "] "
#ifndef _WIN32
			"UID/GID[" + CString(getuid()) + ":" + CString(getgid()) + "]");
#else
			);
#endif
		return true;
	}

	virtual void OnIRCConnected() {
		Log(m_pUser->GetUserName() + " connected to IRC: " + m_pUser->GetCurrentServer()->GetName());
	}

	virtual void OnIRCDisconnected() {
		if (m_pUser->GetCurrentServer() == NULL)
			Log(m_pUser->GetUserName() + " disconnected from IRC[Active server removed]");
		else
			Log(m_pUser->GetUserName() + " disconnected from IRC: " + m_pUser->GetCurrentServer()->GetName());
	}

	virtual EModRet OnRaw(CString& sLine) {
		if (strncasecmp(sLine.c_str(), "ERROR ", 6) == 0) {
			//ERROR :Closing Link: nick[24.24.24.24] (Excess Flood)
			//ERROR :Closing Link: nick[24.24.24.24] Killer (Local kill by Killer (reason))
			CString sError(sLine.substr(7));
			if (sError.Left(1) == ":")
				sError.LeftChomp();
			Log(m_pUser->GetUserName() + " disconnected from IRC: " +
			    m_pUser->GetCurrentServer()->GetName() + " [" + sError + "]",LOG_NOTICE);
		}
		return CONTINUE;
        }

	virtual void OnClientLogin() {
		Log(m_pUser->GetUserName() + " attached to ZNC from " + m_pClient->GetRemoteIP());
	}

	virtual void OnClientDisconnect() {
		Log(m_pUser->GetUserName() + " detached out of ZNC from " + m_pClient->GetRemoteIP());
	}

	virtual void OnFailedLogin(const CString& sUsername, const CString& sRemoteIP) {
		Log(sUsername + " failed to login from " + sRemoteIP,LOG_WARNING);
	}

	void Log(CString sLine, int iPrio = LOG_INFO) {
		CString sTarget = GetNV("target");
#ifndef _WIN32
		if (sTarget == "syslog" || sTarget == "both")
			syslog(iPrio,"%s",sLine.c_str());
		if (sTarget == "" || sTarget == "file" || sTarget == "both") {
#else
		if(sTarget == "" || sTarget == "file") {
#endif
			time_t curtime;
			tm* timeinfo;
			char t[23];

			time(&curtime);
			timeinfo = localtime(&curtime);
			int i = (int)strftime(t,23,"[%Y-%m-%d %H:%M:%S] ",timeinfo);

			ofstream ofLog;
			ofLog.open(sLogFile.c_str(), ofstream::app);
			if (ofLog.good()) {
				ofLog.write(t, i);
				ofLog.write(sLine.c_str(), sLine.length());
				ofLog << endl;
			}
			ofLog.close();
		}
	}

	virtual void OnModCommand(const CString& sCommand) {
		if (!GetUser()->IsAdmin()) {
			PutModule("Access denied");
			return;
		}

		CString sTarget = GetNV("target");

		if (strcasecmp(sCommand.Token(0).c_str(), "target") == 0) {
			if (strcasecmp(sCommand.Token(1,true).c_str(), "file") == 0) {
				SetNV("target", "file", true);
				PutModule("[Status] Logging enabled to file: " + sLogFile);
#ifndef _WIN32
			} else if (strcasecmp(sCommand.Token(1,true).c_str(), "syslog") == 0) {
				SetNV("target", "syslog", true);
				PutModule("[Status] Logging enabled to syslog");
			} else if (strcasecmp(sCommand.Token(1,true).c_str(), "both") == 0) {
				SetNV("target", "both", true);
				PutModule("[Status] Logging enabled to syslog and file: " + sLogFile);
			} else {
				PutModule("Unknown logging facility: [" + sCommand.Token(1,true) + "] " +
					  "valid options are \"file\", \"syslog\" or \"both\"");
			}
		} else {
			if (sTarget == "both")
				PutModule("[Status] Logging enabled to syslog and file: " + sLogFile);
			else
				PutModule("[Status] Logging is enabled to " +
					   (sTarget=="syslog"?"syslog":"file: " + sLogFile));
#else
			} else {
				PutModule("Unknown logging facility: [" + sCommand.Token(1,true) + "] " +
					"the only supported option is \"file\"");
			}
		} else {
			PutModule("[Status] Logging is enabled to file: " + sLogFile);
#endif
			PutModule("TARGET <file|syslog|both> - Change logging facility");
		}
	}
private:
	CString	sLogFile;
};

#ifndef _WIN32
GLOBALMODULEDEFS(CAdminLogMod, "Log ZNC events to file and/or syslog.")
#else
GLOBALMODULEDEFS(CAdminLogMod, "Log ZNC events to a file.")
#endif
