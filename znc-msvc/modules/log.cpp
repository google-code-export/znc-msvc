/*
 * CNU's Logging Module
 * Copyright (C) 2006-2007, CNU <bshalm@broadpark.no> (http://cnu.dieplz.net/znc)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "stdafx.hpp"
#include "main.h"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include "User.h"
#include "Nick.h"
#include "Modules.h"
#include "Chan.h"
#include "Server.h"

class CLogMod: public CModule {
public:
	MODCONSTRUCTOR(CLogMod) {}

	void PutLog(const CString& sLine, const CString& sWindow = "Status");
	CString GetServer();

	virtual void OnIRCConnected();
	virtual void OnIRCDisconnected();
	virtual EModRet OnBroadcast(CString& sMessage);

	virtual void OnRawMode(const CNick& OpNick, CChan& Channel, const CString& sModes, const CString& sArgs);
	virtual void OnKick(const CNick& OpNick, const CString& sKickedNick, CChan& Channel, const CString& sMessage);
	virtual void OnQuit(const CNick& Nick, const CString& sMessage, const vector<CChan*>& vChans);
	virtual void OnJoin(const CNick& Nick, CChan& Channel);
	virtual void OnPart(const CNick& Nick, CChan& Channel);
	virtual void OnNick(const CNick& OldNick, const CString& sNewNick, const vector<CChan*>& vChans);
	virtual EModRet OnTopic(CNick& Nick, CChan& Channel, CString& sTopic);

	/* notices */
	virtual EModRet OnUserNotice(CString& sTarget, CString& sMessage);
	virtual EModRet OnPrivNotice(CNick& Nick, CString& sMessage);
	virtual EModRet OnChanNotice(CNick& Nick, CChan& Channel, CString& sMessage);

	/* actions */
	virtual EModRet OnUserAction(CString& sTarget, CString& sMessage);
	virtual EModRet OnPrivAction(CNick& Nick, CString& sMessage);
	virtual EModRet OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage);

	/* msgs */
	virtual EModRet OnUserMsg(CString& sTarget, CString& sMessage);
	virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage);
	virtual EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage);
};

void CLogMod::PutLog(const CString& sLine, const CString& sWindow /*= "Status"*/)
{
	std::ofstream ofLog;
	std::stringstream ssPath;
	time_t curtime;
	tm* timeinfo;

	time(&curtime);
	// Don't forget the user's timezone offset (which is in hours and we want seconds)
	curtime += (time_t) (m_pUser->GetTimezoneOffset() * 60 * 60);
	timeinfo = localtime(&curtime);

	/* Generate file name: ~/.znc/users/<user>/moddata/log/WINDOW_YYYYMMDD.log  */
	ssPath.fill('0');
	ssPath << GetSavePath() << "/" << sWindow << "_"
	   << std::setw(4) << (timeinfo->tm_year + 1900)
	   << std::setw(2) << (timeinfo->tm_mon + 1)
	   << std::setw(2) << (timeinfo->tm_mday)
	   << ".log";

	ofLog.open(ssPath.str().c_str(), std::ofstream::app);

	if (ofLog.good())
	{
		/* Write line: [HH:MM:SS] MSG */
		ofLog.fill('0');
		ofLog << "[" << std::setw(2) << timeinfo->tm_hour
			  << ":" << std::setw(2) << timeinfo->tm_min
			  << ":" << std::setw(2) << timeinfo->tm_sec
			  << "] " << sLine << endl;
	}
}

CString CLogMod::GetServer()
{
	CServer* pServer = m_pUser->GetCurrentServer();

	if (!pServer)
		return "(no server)";

	//! @todo pServer->IsSSL()
	return pServer->GetName() + ":" + CString(pServer->GetPort());
}

void CLogMod::OnIRCConnected()
{
	PutLog("Connected to IRC (" + GetServer() + ")");
}

void CLogMod::OnIRCDisconnected()
{
	PutLog("Disconnected from IRC (" + GetServer() + ")");
}

CModule::EModRet CLogMod::OnBroadcast(CString& sMessage)
{
	PutLog("Broadcast: " + sMessage);
	return CONTINUE;
}

void CLogMod::OnRawMode(const CNick& OpNick, CChan& Channel, const CString& sModes, const CString& sArgs)
{
	PutLog("*** " + OpNick.GetNick() + " sets mode: " + sModes + " " + sArgs, Channel.GetName());
}

void CLogMod::OnKick(const CNick& OpNick, const CString& sKickedNick, CChan& Channel, const CString& sMessage)
{
	PutLog("*** " + sKickedNick + " was kicked by " + OpNick.GetNick() + " (" + sMessage + ")", Channel.GetName());
}

void CLogMod::OnQuit(const CNick& Nick, const CString& sMessage, const vector<CChan*>& vChans)
{
	for (std::vector<CChan*>::const_iterator pChan = vChans.begin(); pChan != vChans.end(); ++pChan)
		PutLog("*** Quits: " + Nick.GetNick() + " (" + Nick.GetIdent() + "@" + Nick.GetHost() + ") (" + sMessage + ")", (*pChan)->GetName());
}

void CLogMod::OnJoin(const CNick& Nick, CChan& Channel)
{
	PutLog("*** Joins: " + Nick.GetNick() + " (" + Nick.GetIdent() + "@" + Nick.GetHost() + ")", Channel.GetName());
}

void CLogMod::OnPart(const CNick& Nick, CChan& Channel)
{
	PutLog("*** Parts: " + Nick.GetNick() + " (" + Nick.GetIdent() + "@" + Nick.GetHost() + ")", Channel.GetName());
}

void CLogMod::OnNick(const CNick& OldNick, const CString& sNewNick, const vector<CChan*>& vChans)
{
	for (std::vector<CChan*>::const_iterator pChan = vChans.begin(); pChan != vChans.end(); ++pChan)
		PutLog("*** " + OldNick.GetNick() + " is now known as " + sNewNick, (*pChan)->GetName());
}

CModule::EModRet CLogMod::OnTopic(CNick& Nick, CChan& Channel, CString& sTopic)
{
	PutLog("*** " + Nick.GetNick() + " changes topic to '" + sTopic + "'", Channel.GetName());
	return CONTINUE;
}

/* notices */
CModule::EModRet CLogMod::OnUserNotice(CString& sTarget, CString& sMessage)
{
	PutLog("-" + GetUser()->GetCurNick() + "- " + sMessage, sTarget);
	return CONTINUE;
}

CModule::EModRet CLogMod::OnPrivNotice(CNick& Nick, CString& sMessage)
{
	PutLog("-" + Nick.GetNick() + "- " + sMessage, Nick.GetNick());
	return CONTINUE;
}

CModule::EModRet CLogMod::OnChanNotice(CNick& Nick, CChan& Channel, CString& sMessage)
{
	PutLog("-" + Nick.GetNick() + "- " + sMessage, Channel.GetName());
	return CONTINUE;
}

/* actions */
CModule::EModRet CLogMod::OnUserAction(CString& sTarget, CString& sMessage)
{
	PutLog("* " + GetUser()->GetCurNick() + " " + sMessage, sTarget);
	return CONTINUE;
}

CModule::EModRet CLogMod::OnPrivAction(CNick& Nick, CString& sMessage)
{
	PutLog("* " + Nick.GetNick() + " " + sMessage, Nick.GetNick());
	return CONTINUE;
}

CModule::EModRet CLogMod::OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage)
{
	PutLog("* " + Nick.GetNick() + " " + sMessage, Channel.GetName());
	return CONTINUE;
}

/* msgs */
CModule::EModRet CLogMod::OnUserMsg(CString& sTarget, CString& sMessage)
{
	PutLog("<" + GetUser()->GetCurNick() + "> " + sMessage, sTarget);
	return CONTINUE;
}

CModule::EModRet CLogMod::OnPrivMsg(CNick& Nick, CString& sMessage)
{
	PutLog("<" + Nick.GetNick() + "> " + sMessage, Nick.GetNick());
	return CONTINUE;
}

CModule::EModRet CLogMod::OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage)
{
	PutLog("<" + Nick.GetNick() + "> " + sMessage, Channel.GetName());
	return CONTINUE;
}

MODULEDEFS(CLogMod, "CNU's IRC Logging Module")
