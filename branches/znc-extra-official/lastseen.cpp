/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"
#include "znc.h"

using std::map;

class CLastSeenMod : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CLastSeenMod)
	{
	}

	virtual ~CLastSeenMod() {}

	virtual bool OnLoad(const CString& sArgs, CString& sErrorMsg)
	{
		const map<CString, CUser*>& mUsers = CZNC::Get().GetUserMap();
		map<CString, CUser*>::const_iterator it;

		for (it = mUsers.begin(); it != mUsers.end(); it++) {
			if (it->second->GetClients().size())
				m_mLastSeen[it->first] = time(NULL);
			else
				m_mLastSeen[it->first] = 0;
		}

		return true;
	}

	virtual void OnModCommand(const CString& sLine)
	{
		const CString sCommand = sLine.Token(0).AsLower();

		if (!GetUser()->IsAdmin()) {
			PutModule("Access denied");
			return;
		}

		if (sCommand == "show") {
			char buf[1024];
			map<CString, time_t>::iterator it;
			CTable Table;

			Table.AddColumn("User");
			Table.AddColumn("Last Seen");

			for (it = m_mLastSeen.begin(); it != m_mLastSeen.end(); it++) {
				Table.AddRow();
				Table.SetCell("User", it->first);

				if (!it->second)
					Table.SetCell("Last Seen", "never");
				else {
					strftime(buf, sizeof(buf), "%c", localtime(&it->second));
					Table.SetCell("Last Seen", buf);
				}
			}

			PutModule(Table);
		} else {
			PutModule("This module only supports 'show'");
		}
	}

	virtual void OnClientLogin()
	{
		const CString& sName = GetUser()->GetUserName();

		m_mLastSeen[sName] = time(NULL);
	}

	virtual void OnClientDisconnect()
	{
		const CString& sName = GetUser()->GetUserName();

		m_mLastSeen[sName] = time(NULL);
	}

protected:

private:
	map<CString, time_t>	m_mLastSeen;
};

GLOBALMODULEDEFS(CLastSeenMod, "Collects data about when a user last logged in")
