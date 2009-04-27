/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "IRCSock.h"
#include "User.h"

#define REQUIRESSL

class CCertCheckerMod : public CModule {
public:
	MODCONSTRUCTOR(CCertCheckerMod) {
	}

	virtual ~CCertCheckerMod() {
	}

	EModRet OnIRCRegistration(CString& sPass, CString& sNick, CString& sIdent, CString& sRealName) {
		// Meh, I can't use this one because the ssl handshake isn't
		// finished yet
		return CONTINUE;
	}

	virtual void OnIRCConnected() {
		CIRCSock *pIRC = GetUser()->GetIRCSock();
		CString sIP = pIRC->GetRemoteIP();
		CString sPubKey = pIRC->GetPeerPubKey();
		CString sSavedPubKey = GetNV(sIP);

		if (!pIRC->GetSSL())
			return;

		DEBUG("certchecker: [" << sIP << "]: [" << sPubKey << "][" + sSavedPubKey + "]");

		if (sSavedPubKey.empty()) {
			SetNV(sIP, sPubKey);
			PutModule("Connecting to new server, ssl pubkey has been saved");
		} else if (sSavedPubKey != sPubKey) {
			PutModule("SSL CERTIFICATE'S PUBLIC KEY DOES NOT MATCH");
			PutModule("You attempted to connect to [" + pIRC->GetHostName()
					+ "] which resolved to [" + sIP + "]");
			PutModule("The server's public key is:");
			PutModule(sPubKey);
			PutModule("Last time we connected, the public key was:");
			PutModule(sSavedPubKey);

			pIRC->Close();
		}
	}

	virtual void OnModCommand(const CString& sCommand) {
		CString sCmd = sCommand.Token(0);

		if (sCmd.Equals("list")) {
			CTable Table;

			Table.AddColumn("IP");
			Table.AddColumn("PubKey");

			for (MCString::iterator it = BeginNV(); it != EndNV(); it++) {
				Table.AddRow();
				Table.SetCell("IP", it->first);
				Table.SetCell("PubKey", it->second);
			}
			PutModule(Table);
		} else if (sCmd.Equals("del")) {
			if (DelNV(sCommand.Token(1)))
				PutModule("Done");
			else
				PutModule("IP not found");
		} else {
			PutModule("Commands: list, del [ip]");
		}
	}
};

MODULEDEFS(CCertCheckerMod, "Check if a ssl server's certificate changed")
