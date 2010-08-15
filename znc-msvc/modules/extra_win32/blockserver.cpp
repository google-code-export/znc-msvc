/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Modules.h"
#include "User.h"

class CBlockServerMod : public CModule {
public:
	MODCONSTRUCTOR(CBlockServerMod) {}

	virtual EModRet OnStatusCommand(CString& sLine) {
		CString sCommand = sLine.Token(0);
	
		if (sCommand.Equals("ADDSERVER") || sCommand.Equals("REMSERVER") || sCommand.Equals("DELSERVER")) {
			PutStatus("Access denied");
			return HALTCORE;
		}
		return CONTINUE;
	}

	virtual EModRet OnUserRaw(CString& sLine) {
		if (!sLine.Token(0).Equals("znc"))
			return CONTINUE;

		CString sTarget = sLine.Token(1);
		CString sModCommand;

		if (sTarget.TrimPrefix(m_pUser->GetStatusPrefix())) {
			sModCommand = sLine.Token(2, true);
		} else {
			sTarget  = "status";
			sModCommand = sLine.Token(1, true);
		}

		if (sTarget == "status")
			return OnStatusCommand(sModCommand);
		return CONTINUE;
	}
};

MODULEDEFS(CBlockServerMod, "Block *status' server interface")
