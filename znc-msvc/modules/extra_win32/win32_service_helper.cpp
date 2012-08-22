/*
* Copyright (C) 2012 Ingmar Runge
* See the AUTHORS file for details.
*
* Helper module to communicate with ZNC GUI.
* http://code.google.com/p/znc-msvc/
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#include "stdafx.hpp"
#include "znc.h"
#include "User.h"
#include "Modules.h"


class CServiceHelperMod : public CGlobalModule
{
public:
	GLOBALMODCONSTRUCTOR(CServiceHelperMod)
	{
	}
	virtual ~CServiceHelperMod();

	/* ZNC events */
	EModRet OnStatusCommand(CString& sLine);
	void OnModCommand(const CString& sLine);

protected:
	/* local methods */

};


CModule::EModRet CServiceHelperMod::OnStatusCommand(CString& sLine)
{
	if(ZNCWin32ServiceMode() && m_pUser && m_pUser->IsAdmin())
	{
		const CString sCommand = sLine.Token(0);

		if(sCommand.Equals("RESTART"))
		{
			PutStatus("Error: for technical reasons, restarting ZNC when running as a service is not possible. Please restart ZNC via outside means.");
			return HALTCORE;
		}
	}

	return CONTINUE;
}


void CServiceHelperMod::OnModCommand(const CString& sLine)
{
	CString sCommand = sLine.Token(0);

	if(sCommand.Equals("HELP"))
	{
		PutModule("No commands available.");
		return;
	}
}


CServiceHelperMod::~CServiceHelperMod()
{
}


GLOBALMODULEDEFS(CServiceHelperMod, "Communication helper for the ZNC Windows GUI.")
