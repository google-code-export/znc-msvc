/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "IRCSock.h"
#include "Modules.h"
#include "User.h"
#include "znc.h"

class CControlMod : public CModule {
public:
	MODCONSTRUCTOR(CControlMod) {}

	virtual void OnModCommand(const CString& sCommand) {
		if (!m_pUser->IsAdmin()) {
			PutModule("You need admin privs to use this");
			return;
		}
		if (!sCommand.Token(0).Equals("raw")) {
			PutModule("Usage: raw [username] [message]");
			return;
		}

		CString sUser = sCommand.Token(1);
		CString sRaw = sCommand.Token(2, true);
		CUser *pUser = CZNC::Get().FindUser(sUser);

		if (!pUser) {
			PutModule("No such User [" + sUser + "]");
			return;
		}

		CIRCSock *pIRC = pUser->GetIRCSock();

		if (!pIRC) {
			PutModule("User is not connected to IRC");
			return;
		}

		pIRC->PutIRC(sRaw);
		PutModule("Done");
	}
};

MODULEDEFS(CControlMod, "Send something to another user's irc server")

