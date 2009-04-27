/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "IRCSock.h"
#include "znc.h"

#define MESSAGE "Your account has been disabled. Contact your administrator."

class CBlockUser : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CBlockUser) {}

	virtual ~CBlockUser() {}

	virtual bool OnLoad(const CString& sArgs, CString& sMessage) {
		CString sArg = sArgs;

		while (!sArg.empty()) {
			if (!Block(sArg.Token(0))) {
				sMessage = "Could not block [" + sArg.Token(0) + "]";
				return false;
			}
			sArg = sArg.Token(1, true);
		}

		return true;
	}

	virtual EModRet OnLoginAttempt(CSmartPtr<CAuthBase> Auth) {
		if (IsBlocked(Auth->GetUsername())) {
			Auth->RefuseLogin(MESSAGE);
			return HALT;
		}

		return CONTINUE;
	}

private:
	bool IsBlocked(const CString& sUser) {
		VCString::iterator it;
		for (it = m_blockedUsers.begin(); it != m_blockedUsers.end(); it++) {
			if (sUser.Equals(*it)) {
				return true;
			}
		}
		return false;
	}

	bool Block(const CString& sUser) {
		CUser *pUser = CZNC::Get().FindUser(sUser);

		if (!pUser)
			return false;

		// Disconnect all clients
		vector<CClient*>& vpClients = pUser->GetClients();
		vector<CClient*>::iterator it;
		for (it = vpClients.begin(); it != vpClients.end(); it++) {
			(*it)->PutStatusNotice(MESSAGE);
			(*it)->Close(Csock::CLT_AFTERWRITE);
		}

		// Disconnect from IRC...
		CIRCSock *pIRCSock = pUser->GetIRCSock();
		if (pIRCSock) {
			pIRCSock->Quit();
		}

		// ...and don't reconnect
		pUser->SetIRCConnectEnabled(false);

		m_blockedUsers.push_back(pUser->GetUserName());
		return true;
	}

	VCString	m_blockedUsers;
};

GLOBALMODULEDEFS(CBlockUser, "Block certain users from logging in")
