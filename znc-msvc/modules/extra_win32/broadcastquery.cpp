/*
 * Copyright (C) 2010  Brian Campbell "BrianC"
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "User.h"
#include "Modules.h"

class CBroadcastQuery : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CBroadcastQuery) {}

	virtual EModRet OnBroadcast(CString &sMessage) {
		CString sModName = "broadcast";

		m_pUser->PutModule(sModName, sMessage);

		return HALTCORE;
	}
};

GLOBALMODULEDEFS(CBroadcastQuery, "Sends all broadcasts as queries, instead of notices")
