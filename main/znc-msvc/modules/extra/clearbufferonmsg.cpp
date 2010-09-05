/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Chan.h"
#include "Modules.h"
#include "User.h"

class CClearBufferOnMsgMod : public CModule {
public:
	MODCONSTRUCTOR(CClearBufferOnMsgMod) {}

	void ClearAllBuffers() {
		const vector<CChan*>& vChans = GetUser()->GetChans();
		vector<CChan*>::const_iterator it;

		for (it = vChans.begin(); it != vChans.end(); ++it) {
			(*it)->ClearBuffer();
			// We force KeepBuffer on all channels since this module
			// doesnt make any sense without
			(*it)->SetKeepBuffer(true);
		}

	}

	virtual EModRet OnUserMsg(CString& sTarget, CString& sMessage) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserCTCP(CString& sTarget, CString& sMessage) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserAction(CString& sTarget, CString& sMessage) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserNotice(CString& sTarget, CString& sMessage) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserJoin(CString& sChannel, CString& sKey) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserPart(CString& sChannel, CString& sMessage) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual EModRet OnUserTopic(CString& sChannel, CString& sTopic) {
		ClearAllBuffers();
		return CONTINUE;
	}

	virtual void OnClientLogin() {
		ClearAllBuffers();
	}

	// No OnClientDisconnect(), disconnect could be coming from e.g. a timeout
};

MODULEDEFS(CClearBufferOnMsgMod, "Clear all channel buffers whenever the user does something")
