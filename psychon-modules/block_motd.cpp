/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "User.h"

class CBlockMotd : public CModule
{
public:
	MODCONSTRUCTOR(CBlockMotd)
	{
	}

	virtual ~CBlockMotd()
	{
	}

	virtual EModRet OnRaw(CString &sLine)
	{
		const CString sCmd = sLine.Token(1);

		if (sCmd == "375" /* begin of MOTD */
				|| sCmd == "372" /* MOTD */
				|| sCmd == "376" /* End of MOTD */)
			return HALT;
		return CONTINUE;
	}
};

MODULEDEFS(CBlockMotd, "Block the MOTD from IRC")
