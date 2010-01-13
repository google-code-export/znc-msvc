/*
 * Copyright (C) 2009 flakes @ EFNet
 * See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "znc.h"
#include "User.h"

class CFixLagChkMod : public CModule
{
private:
	typedef TCacheMap<const CString, CClient*> TWaitingMap;
	TWaitingMap m_waiting;
public:
	MODCONSTRUCTOR(CFixLagChkMod)
	{
		m_waiting.SetTTL(1000 * 120); // 2 minutes...
	}

	EModRet OnUserNotice(CString& sTarget, CString& sMessage)
	{
		if(m_pClient && sTarget.Equals(m_pUser->GetIRCNick().GetNick()) &&
			sMessage.Token(0).Equals("LAGCHK"))
		{
			m_waiting.AddItem(sMessage, m_pClient);
		}

		return CONTINUE;
	}

	EModRet OnPrivNotice(CNick& Nick, CString& sMessage)
	{
		if(Nick.GetNick().Equals(m_pUser->GetIRCNick().GetNick()) &&
			sMessage.Token(0).Equals("LAGCHK"))
		{
			CClient **client = m_waiting.GetItem(sMessage);

			if(client)
			{
				(*client)->PutClient(":" + m_pUser->GetIRCNick().GetHostMask() +
					" NOTICE " + m_pUser->GetIRCNick().GetNick() + " :" + sMessage);
			}

			return HALT;
		}

		return CONTINUE;
	}

	void OnClientDisconnect()
	{
		m_waiting.Clear();
	}
};

MODULEDEFS(CFixLagChkMod, "Fixes NoNameScript's and other clients' LAGCHK stuff by sending it to the correct client only.")
