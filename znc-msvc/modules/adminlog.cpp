/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "User.h"
#include "Client.h"
#include "Server.h"

#ifndef _WIN32
#include <syslog.h>
#else
#define LOG_INFO 0
#define LOG_NOTICE 1
#define LOG_WARNING 2
#endif

class CAdminLogMod : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CAdminLogMod) {
#ifndef _WIN32
		openlog("znc", LOG_PID, LOG_DAEMON);
#endif
	}

	virtual ~CAdminLogMod() {
		Log("Logging ended.");
#ifndef _WIN32
		closelog();
#endif
	}

	virtual bool OnLoad(const CString & sArgs, CString & sMessage) {
#ifndef _WIN32
		CString sTarget = GetNV("target");
		if (sTarget.Equals("syslog"))
			m_eLogMode = LOG_TO_SYSLOG;
		else if (sTarget.Equals("both"))
			m_eLogMode = LOG_TO_BOTH;
		else if (sTarget.Equals("file"))
			m_eLogMode = LOG_TO_FILE;
		else
#endif
			m_eLogMode = LOG_TO_FILE;

		m_sLogFile = GetSavePath() + "/znc.log";

#ifndef _WIN32
		Log("Logging started. ZNC PID[" + CString(getpid()) + "] UID/GID[" + CString(getuid()) + ":" + CString(getgid()) + "]");
#else
		Log("Logging started. ZNC PID[" + CString(_getpid()) + "]");
#endif
		return true;
	}

	virtual void OnIRCConnected() {
		Log("[" + m_pUser->GetUserName() + "] connected to IRC: " + m_pUser->GetCurrentServer()->GetName());
	}

	virtual void OnIRCDisconnected() {
		Log("[" + m_pUser->GetUserName() + "] disconnected from IRC");
	}

	virtual EModRet OnRaw(CString& sLine) {
		if (sLine.Equals("ERROR ", false, 6)) {
			//ERROR :Closing Link: nick[24.24.24.24] (Excess Flood)
			//ERROR :Closing Link: nick[24.24.24.24] Killer (Local kill by Killer (reason))
			CString sError(sLine.substr(6));
			if (sError.Left(1) == ":")
				sError.LeftChomp();
			Log("[" + m_pUser->GetUserName() + "] disconnected from IRC: " +
			    m_pUser->GetCurrentServer()->GetName() + " [" + sError + "]", LOG_NOTICE);
		}
		return CONTINUE;
        }

	virtual void OnClientLogin() {
		Log("[" + m_pUser->GetUserName() + "] connected to ZNC from " + m_pClient->GetRemoteIP());
	}

	virtual void OnClientDisconnect() {
		Log("[" + m_pUser->GetUserName() + "] disconnected from ZNC from " + m_pClient->GetRemoteIP());
	}

	virtual void OnFailedLogin(const CString& sUsername, const CString& sRemoteIP) {
		Log("[" + sUsername + "] failed to login from " + sRemoteIP, LOG_WARNING);
	}

	void Log(CString sLine, int iPrio = LOG_INFO) {
#ifndef _WIN32
		if (m_eLogMode & LOG_TO_SYSLOG)
			syslog(iPrio, "%s", sLine.c_str());
#endif
		if (m_eLogMode & LOG_TO_FILE) {
			time_t curtime;
			tm* timeinfo;
			char buf[23];

			time(&curtime);
			timeinfo = localtime(&curtime);
			strftime(buf,sizeof(buf),"[%Y-%m-%d %H:%M:%S] ",timeinfo);

			CFile LogFile(m_sLogFile);

			if (LogFile.Open(O_WRONLY | O_APPEND | O_CREAT))
				LogFile.Write(buf + sLine + "\n");
			else
				DEBUG("Failed to write to [" << m_sLogFile  << "]: " << strerror(errno));
		}
	}

	virtual void OnModCommand(const CString& sCommand) {
		if (!GetUser()->IsAdmin()) {
			PutModule("Access denied");
			return;
		}

		CString sCmd = sCommand.Token(0);

		if (sCmd.Equals("target")) {
			CString sArg = sCommand.Token(1, true);
			CString sTarget;
			CString sMessage;
			LogMode mode;

			if (sArg.Equals("file")) {
				sTarget = "file";
				sMessage = "Now only logging to file";
				mode = LOG_TO_FILE;
			}
#ifndef _WIN32
			else if (sArg.Equals("syslog")) {
				sTarget = "syslog";
				sMessage = "Now only logging to syslog";
				mode = LOG_TO_SYSLOG;
			} else if (sArg.Equals("both")) {
				sTarget = "both";
				sMessage = "Now logging to file and syslog";
				mode = LOG_TO_BOTH;
			} 
#endif
			else {
				PutModule("Unknown target");
				return;
			}

			Log(sMessage);
			SetNV("target", sTarget);
			m_eLogMode = mode;
			PutModule(sMessage);
		} else if (sCmd.Equals("show")) {
			CString sTarget;

#ifndef _WIN32
			switch (m_eLogMode)
			{
			case LOG_TO_FILE:
				sTarget = "file";
				break;
			case LOG_TO_SYSLOG:
				sTarget = "syslog";
				break;
			case LOG_TO_BOTH:
				sTarget = "both, file and syslog";
				break;
			}
#else
			sTarget = "file";
#endif

			PutModule("Logging is enabled for " + sTarget);
			if (m_eLogMode != LOG_TO_SYSLOG)
				PutModule("Log file will be written to [" + m_sLogFile + "]");
		} else
#ifndef _WIN32
			PutModule("Commands: show, target <file|syslog|both>");
#else
			PutModule("Commands: Only logging to a file is supported on Win32.");
#endif
	}
private:
	enum LogMode {
		LOG_TO_FILE   = 1 << 0,
		LOG_TO_SYSLOG = 1 << 1,
		LOG_TO_BOTH   = LOG_TO_FILE | LOG_TO_SYSLOG
	};
	LogMode m_eLogMode;
	CString m_sLogFile;
};

#ifndef _WIN32
GLOBALMODULEDEFS(CAdminLogMod, "Log ZNC events to file and/or syslog.")
#else
GLOBALMODULEDEFS(CAdminLogMod, "Log ZNC events to a file.")
#endif
