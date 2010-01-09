/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "Modules.h"

class CStartDisconnectedMod : public CModule {
public:
	MODCONSTRUCTOR(CStartDisconnectedMod) {}
	~CStartDisconnectedMod() {}

	virtual bool OnLoad(const CString& sArgsi, CString& sMessage) {
		m_pUser->SetIRCConnectEnabled(false);
		return true;
	}
};

MODULEDEFS(CStartDisconnectedMod, "Prevents a user account from connecting on start-up.")
