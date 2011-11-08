/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Chan.h"
#include "User.h"
#include "Modules.h"

class CPartOnNoClientsMod : public CModule {
public:
	MODCONSTRUCTOR(CPartOnNoClientsMod) {}
	~CPartOnNoClientsMod() {}
	
	void OnClientDisconnect() {
		if(!m_pUser->IsUserAttached()) {
			const vector<CChan*>& vChans = m_pUser->GetChans();
			
			while(!vChans.empty())
			{
				CChan *pChan = *(vChans.begin());
				CString sChan = pChan->GetName();
				m_pUser->DelChan(sChan);
				PutIRC("PART " + sChan);
			}
		}
	}
};

MODULEDEFS(CPartOnNoClientsMod, "Parts all channels when the last client disconnects.")
