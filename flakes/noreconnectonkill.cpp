/*
 * Copyright (C) flakes
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "Modules.h"

class CNoReconnectOnKillMod : public CModule {
public:
	MODCONSTRUCTOR(CNoReconnectOnKillMod) {}

	EModRet OnRaw(CString& sLine) {
		CString sCmd = sLine.Token(0).AsUpper();

		if(sCmd == "ERROR") {
			CString sRest = sLine.Token(1, true).AsLower();

			if(sRest.find("lined") != CString::npos || sRest.find("kill") != CString::npos) {
				if(m_pUser) {
					m_pUser->SetIRCConnectEnabled(false);
				}
			}
		}
		return CONTINUE;
	}
};

MODULEDEFS(CNoReconnectOnKillMod, "Disables automatic reconnect if you have been killed or *lined from the server.")
