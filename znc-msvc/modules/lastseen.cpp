/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Chan.h"
#include "znc.h"

using std::map;
using std::pair;
using std::multimap;

class CLastSeenMod : public CGlobalModule {
private:
	time_t GetTime(const CUser *pUser) {
		return GetNV(pUser->GetUserName()).ToULong();
	}

	void SetTime(const CUser *pUser) {
		SetNV(pUser->GetUserName(), CString(time(NULL)));
	}

	const CString FormatLastSeen(const CUser *pUser, const char* sDefault = "") {
		time_t last = GetTime(pUser);
		if (last < 1) {
			return sDefault;
		} else {
			char buf[1024];
			strftime(buf, sizeof(buf) - 1, "%c", localtime(&last));
			return buf;
		}
	}

	typedef multimap<time_t, CUser*> MTimeMulti;
	typedef map<CString, CUser*> MUsers;
public:
	GLOBALMODCONSTRUCTOR(CLastSeenMod) {
	}

	virtual ~CLastSeenMod() {}

	// IRC stuff:

	virtual void OnModCommand(const CString& sLine) {
		const CString sCommand = sLine.Token(0).AsLower();

		if (!GetUser()->IsAdmin()) {
			PutModule("Access denied");
			return;
		}

		if (sCommand == "show") {
			const MUsers& mUsers = CZNC::Get().GetUserMap();
			MUsers::const_iterator it;
			CTable Table;

			Table.AddColumn("User");
			Table.AddColumn("Last Seen");

			for (it = mUsers.begin(); it != mUsers.end(); ++it) {
				Table.AddRow();
				Table.SetCell("User", it->first);
				Table.SetCell("Last Seen", FormatLastSeen(it->second));
			}

			PutModule(Table);
		} else {
			PutModule("This module only supports 'show'");
		}
	}

	// Event stuff:

	virtual void OnClientLogin() {
		SetTime(GetUser());
	}

	virtual void OnClientDisconnect() {
		SetTime(GetUser());
	}

	virtual EModRet OnDeleteUser(CUser& User) {
		DelNV(User.GetUserName());
		return CONTINUE;
	}

	// Web stuff:

	virtual bool WebRequiresLogin() { return true; }
	virtual bool WebRequiresAdmin() { return true; }
	virtual CString GetWebMenuTitle() { return "Last Seen"; }

	virtual bool OnWebRequest(CWebSock& WebSock, const CString& sPageName, CTemplate& Tmpl) {
		if (sPageName.empty() || sPageName == "index") {
			CModules& GModules = CZNC::Get().GetModules();
			Tmpl["WebAdminLoaded"] = CString(GModules.FindModule("webadmin") != NULL);

			MTimeMulti mmSorted;
			const MUsers& mUsers = CZNC::Get().GetUserMap();

			for (MUsers::const_iterator uit = mUsers.begin(); uit != mUsers.end(); ++uit) {
				mmSorted.insert(pair<time_t, CUser*>(GetTime(uit->second), uit->second));
			}

			for (MTimeMulti::const_iterator it = mmSorted.begin(); it != mmSorted.end(); ++it) {
				CUser *pUser = it->second;
				CTemplate& Row = Tmpl.AddRow("UserLoop");

				Row["Username"] = pUser->GetUserName();
				Row["IsSelf"] = CString(pUser == WebSock.GetSession()->GetUser());
				Row["LastSeen"] = FormatLastSeen(pUser, "never");

				Row["Info"] = CString(pUser->GetClients().size()) +
					" client" + CString(pUser->GetClients().size() == 1 ? "" : "s");
				if(!pUser->GetCurrentServer()) {
					Row["Info"] += ", not connected to IRC";
				} else {
					size_t uChans = 0;
					const vector<CChan*>& vChans = pUser->GetChans();
					for (unsigned int a = 0; a < vChans.size(); ++a) {
						if (vChans[a]->IsOn()) ++uChans;
					}
					size_t n = uChans;
					Row["Info"] += ", joined to " + CString(uChans);
					if(uChans != vChans.size()) {
						Row["Info"] += " out of " + CString(vChans.size()) + " configured";
						n = vChans.size();
					}
					Row["Info"] += " channel" + CString(n == 1 ? "" : "s");
				}
			}

			return true;
		}

		return false;
	}

	virtual bool OnEmbeddedWebRequest(CWebSock& WebSock, const CString& sPageName, CTemplate& Tmpl) {
		if (sPageName == "webadmin/user" && WebSock.GetSession()->IsAdmin()) {
			CUser* pUser = CZNC::Get().FindUser(Tmpl["Username"]);
			if (pUser) {
				Tmpl["LastSeen"] = FormatLastSeen(pUser);
			}
			return true;
		}

		return false;
	}

};

GLOBALMODULEDEFS(CLastSeenMod, "Collects data about when a user last logged in")
