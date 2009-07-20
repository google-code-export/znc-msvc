/*
 * Copyright (C) 2009 flakes @ EFNet
 * Version 0.5 (2009-07-19 10:00 AM)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "znc.h"
#include "User.h"
#include "Chan.h"
#include "Nick.h"
#include "Modules.h"

class CProwlMod : public CModule {
protected:
	CString m_host;
	CString m_apiKey;
	int m_priority;
	int m_idleAfterMinutes;
	VCString m_highlights;

	time_t m_lastActivity;

public:
	MODCONSTRUCTOR(CProwlMod) {

		/*** mini config start ***/

		m_host = "prowl.weks.net";
		m_apiKey = "";
		m_priority = 1;
		m_idleAfterMinutes = 5;

		m_highlights.push_back("2nd nick");
		m_highlights.push_back("3rd nick");

		/*** mini config end ***/

		m_lastActivity = time(NULL);
	}

protected:
	static CString URLEscape(const CString& sStr) {
		return sStr.Escape_n(CString::EURL);
	}

	CString MakeRequest(const CString& sDescription) {
		CString s;

		s += "GET /publicapi/add";
		s += "?apikey=" + URLEscape(m_apiKey);
		s += "&priority=" + CString(m_priority);
		s += "&application=ZNC";
		s += "&event=" + URLEscape(m_pUser->GetCurNick());
		s += "&description=" + URLEscape(sDescription);

		s += " HTTP/1.0\r\n";
		s += "Connection: close\r\n";
		s += "Host: " + m_host + "\r\n";
		s += "User-Agent: " + CZNC::GetTag() + "\r\n";
		s += "\r\n";

		return s;
	}

	void SendNotification(const CString& sMessage) {
		CSocket *p = new CSocket(this);
		p->Connect(m_host, 443, true); // connect to host at port 443 using SSL
		p->Write(MakeRequest(sMessage));
		p->Close(Csock::CLT_AFTERWRITE); // discard the response...
		AddSocket(p);
	}

	void CheckHighlight(const CString& sNick, const CString& sChannel, const CString& sMessage)
	{
		if(m_pUser) {
			if(m_lastActivity < time(NULL) - m_idleAfterMinutes * 60 || !m_pUser->IsUserAttached()) {
				const CString sLcMessage = sMessage.AsLower();
				bool bFound = (sLcMessage.find(m_pUser->GetCurNick().AsLower()) != CString::npos);

				for(VCString::iterator it = m_highlights.begin(); !bFound && it != m_highlights.end(); it++) {
					bFound = (sLcMessage.find((*it).AsLower()) != CString::npos);
				}

				if(bFound) {
					SendNotification("<" + sNick + "> on " + sChannel + ": " + sMessage);
				}
			}
		}
	}

public:
	EModRet OnPrivMsg(CNick& Nick, CString& sMessage) {
		CheckHighlight(Nick.GetNick(), "(priv)", sMessage);
		return CONTINUE;
	}

	EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) {
		CheckHighlight(Nick.GetNick(), Channel.GetName(), sMessage);
		return CONTINUE;
	}

	EModRet OnPrivAction(CNick& Nick, CString& sMessage) {
		CheckHighlight(Nick.GetNick(), "(priv)", sMessage);
		return CONTINUE;
	}

	EModRet OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage) {
		CheckHighlight(Nick.GetNick(), Channel.GetName(), sMessage);
		return CONTINUE;
	}

	EModRet OnUserAction(CString& sTarget, CString& sMessage) { m_lastActivity = time(NULL); return CONTINUE; }
	EModRet OnUserMsg(CString& sTarget, CString& sMessage) { m_lastActivity = time(NULL); return CONTINUE; }
	EModRet OnUserNotice(CString& sTarget, CString& sMessage) { m_lastActivity = time(NULL); return CONTINUE; }
	EModRet OnUserJoin(CString& sChannel, CString& sKey) { m_lastActivity = time(NULL); return CONTINUE; }
	EModRet OnUserPart(CString& sChannel, CString& sMessage) { m_lastActivity = time(NULL); return CONTINUE; }
};

MODULEDEFS(CProwlMod, "Sends nick highlights to prowl.")
