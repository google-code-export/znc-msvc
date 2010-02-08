/*
 * Copyright (C) 2010 flakes @ EFNet
 * See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "Modules.h"
#include "User.h"

class CFixRelayBot : public CModule
{
private:
	CString m_mask, m_look;

public:
	MODCONSTRUCTOR(CFixRelayBot) {}

	bool OnLoad(const CString& sArgs, CString& sErrorMsg)
	{
		if(sArgs.empty())
		{
			sErrorMsg = "Missing bot host mask argument (e.g. XYZ!bot@host.tld)!";
			return false;
		}

		m_mask = sArgs;
		m_look = ":" + m_mask + " PRIVMSG ";

		return true;
	}

	EModRet OnRaw(CString& sLine)
	{
		if(sLine.Left(m_look.size()).Equals(m_look))
		{
			CString sChan = sLine.Token(2);
			CString sMsg = sLine.Token(3, true);
			sMsg.LeftChomp();

			if(sMsg.empty() || sMsg[0] != '<')
				return CONTINUE;

			CString::size_type uPos = sMsg.find('>');
			if(uPos == CString::npos)
				return CONTINUE;

			CString sTempNick = sMsg.substr(1, uPos - 1);
			CString sNick;
			for(size_t p = 0; p < sTempNick.size(); p++)
			{
				if(isalnum(sTempNick[p]) || std::string("[\\]^_-{|}").find(sTempNick[p]) != CString::npos)
				{
					sNick += sTempNick[p];
				}
			}

			sNick += "\\S";

			m_pUser->PutUser(":" + sNick + "!SkypeUser@irc.znc.in PRIVMSG " + sChan + " :" + sMsg.substr(uPos + 2), NULL);
			return HALT;
		}
		return CONTINUE;
	}
};

MODULEDEFS(CFixRelayBot, "Makes relayed bot messages look like real messages")
