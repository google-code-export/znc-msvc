/*
 * Copyright (C) 2007,  Uwe Grohnwaldt <lando@lando.cc> (http://lando.cc)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "User.h"
#include "Nick.h"
#include "Modules.h"
#include "Chan.h"
#include "Utils.h"
#include <pwd.h>
#include <map>
#include <vector>

class CQAuth : public CModule
{
public:
	MODCONSTRUCTOR(CQAuth)
	{
	}
	
	virtual ~CQAuth()
	{
	}

	virtual bool OnLoad(const CString& sArgs, CString& sMessage)
	{
		if (sArgs.empty())
			m_sAuth = GetNV("Auth");
		else
			m_sAuth = sArgs;
		
		return true;
	}

	virtual void OnModCommand( const CString& sCommand )
	{
		CString sCmdName = sCommand.Token(0).AsLower();
		if(sCmdName == "set") {
			CString sPass = sCommand.Token(1, true) + " ";
			m_sAuth = sPass;
			PutModule("Auth set");
		} else if(sCmdName == "auth") {
			if (m_sAuth.empty())
				PutModule("No auth set");
			else {
				PutIRC("PRIVMSG Q@CServe.quakenet.org :auth " + m_sAuth);
				PutModule("Authing!");
			}
		} else if(sCmdName == "show") {
			if (m_sAuth.empty())
				PutModule("No auth set");
			else
				PutModule("Current user and password: " +  m_sAuth);
		} else if(sCmdName == "save") {
			SetNV("Auth", m_sAuth);
			PutModule("Saved!");
		} else {
			PutModule("Commands: set <user password>, show, save");
		}
	}

	virtual void OnIRCConnected()
	{
		PutIRC("PRIVMSG Q@Cserve.quakenet.org :whoami");
	}
	
	void HandleMessage(CNick& Nick, const CString& sMessage)
	{
		if (!m_sAuth.empty()
				&& Nick.GetNick().CaseCmp("Q") == 0
				&& Nick.GetHost().CaseCmp("CServe.quakenet.org")  == 0
				&& sMessage.find("AUTH") != CString::npos
				&& sMessage.find("authenticate") != CString::npos) {
			PutIRC("PRIVMSG Q@CServe.quakenet.org :auth " + m_sAuth);
		}
	}
	
	virtual EModRet OnPrivMsg(CNick& Nick, CString& sMessage)
	{
		HandleMessage(Nick, sMessage);
		return CONTINUE;
	}

	virtual EModRet OnPrivNotice(CNick& Nick, CString& sMessage)
	{
		HandleMessage(Nick, sMessage);
 		return CONTINUE;
	}

private:
	CString	m_sAuth;
};

MODULEDEFS(CQAuth, "Auths you with Quakenet.")
