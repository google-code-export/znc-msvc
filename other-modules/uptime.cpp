/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "Modules.h"
#include "znc.h"

#define HOST "localhost"

class CUptimeTimer : public CTimer {
public:
	CUptimeTimer(CModule* pModule, unsigned int uInterval, unsigned int uCycles,
			const CString& sLabel, const CString& sDescription)
		: CTimer(pModule, uInterval, uCycles, sLabel, sDescription) {}
	virtual ~CUptimeTimer() {}

private:
protected:
	virtual void RunJob();
};

class CUptimeMod : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CUptimeMod) {}

	virtual bool OnLoad(const CString& sArgs, CString& sMessage) {
		AddTimer(new CUptimeTimer(this, 20/*60 * 30*/, 0, "Uptime",
					"Send uptime stats to a server"));
		return true;
	}

	unsigned int GetUptime() {
		time_t now = time(NULL);
		return now - CZNC::Get().TimeStarted();
	}

	CString MakeRequest() {
		CString s;
		CString sPost;

		sPost += "uptime=";
		sPost += CString(GetUptime());
		sPost += "&version=";
		sPost += CZNC::GetTag();
		sPost += "&args=";
		sPost += GetArgs();

		s += "POST / HTTP/1.0\r\n";
		s += "Connection: close\r\n";
		s += "Host: " HOST "\r\n";
		s += "User-Agent: " + CZNC::GetTag() + "\r\n";
		s += "Content-Length: ";
		s += CString(sPost.length());
		s += "\r\n";
		s += "\r\n";
		s += sPost;

		return s;
	}

	void SendUptime() {
		CSocket *p = new CSocket(this);
		p->Connect(HOST, 1235, false);
		p->Write(MakeRequest());
		p->Close(Csock::CLT_AFTERWRITE);
		AddSocket(p);
	}
};

void CUptimeTimer::RunJob() {
	CUptimeMod *p = (CUptimeMod *) m_pModule;
	p->SendUptime();
}

GLOBALMODULEDEFS(CUptimeMod, "Send uptime statistics to <url>")
