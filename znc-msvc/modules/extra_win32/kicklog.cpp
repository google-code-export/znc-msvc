/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Modules.h"
#include "User.h"
#include "Chan.h"

class CKickLogMod : public CModule {
public:
	MODCONSTRUCTOR(CKickLogMod) {}

	virtual ~CKickLogMod() {}

	virtual void OnKick(const CNick& OpNick, const CString& sKickedNick, CChan& Channel, const CString& sMessage) {
		if (m_pUser->IsUserAttached())
			return;
		if (sKickedNick != GetUser()->GetNick())
			return;

		CString sMsg = "Kicked by [" + OpNick.GetNickMask() + "] in [";
		sMsg += Channel.GetName() + "]: " + sMessage;

		sMsg = GetUser()->AddTimestamp(sMsg);
		DEBUG(sMsg);

		m_vSavedKicks.push_back(sMsg);
	}

	virtual void OnClientLogin() {
		VCString::const_iterator it;
		for (it = m_vSavedKicks.begin(); it != m_vSavedKicks.end(); it++) {
			PutModule(*it);
		}
		m_vSavedKicks.clear();
	}

	VCString m_vSavedKicks;
};

MODULEDEFS(CKickLogMod, "Log kick messages when you are kicked")
