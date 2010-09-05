/*
 * Copyright (C) 2004-2007  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "main.h"
#include "User.h"
#include "Nick.h"
#include "Modules.h"
#include "Chan.h"
#include "Utils.h"
#include <pwd.h>
#include <map>
#include <vector>

class CTimedTimer : public CTimer
{
public:
	CTimedTimer( CModule* pModule, unsigned int uInterval, unsigned int uCycles, const CString& sLabel, const CString& sDescription ) 
		: CTimer( pModule, uInterval, uCycles, sLabel, sDescription) {}

	virtual ~CTimedTimer() {}

protected:
	virtual void RunJob();
};

class CTimedPerform : public CModule
{
public:
	MODCONSTRUCTOR(CTimedPerform)
	{
	}
	
	virtual ~CTimedPerform()
	{
	}

	virtual bool OnLoad(const CString& sArgs, CString& sErrorMsg)
	{
		m_timer = GetNV("Time").ToUInt();
		if (m_timer == 0 || m_timer > 300)
			m_timer = 60;

		GetNV("Perform").Split("\n", m_vPerform, false);
		
		return true;
	}

	virtual void OnModCommand( const CString& sCommand )
	{
		CString sCmdName = sCommand.Token(0).AsLower();
		if(sCmdName == "add")
		{
			CString sPerf = sCommand.Token(1, true);
			if(sPerf.Left(1) == "/")
				sPerf.LeftChomp();

			if(sPerf.Token(0).CaseCmp("MSG") == 0) {
				sPerf = "PRIVMSG " + sPerf.Token(1, true);
			}

			if((sPerf.Token(0).CaseCmp("PRIVMSG") == 0 ||
				sPerf.Token(0).CaseCmp("NOTICE") == 0) &&
				sPerf.Token(2).Left(1) != ":") {
				sPerf = sPerf.Token(0) + " " + sPerf.Token(1)
					+ " :" + sPerf.Token(2, true);
			}
			m_vPerform.push_back(sPerf);
			PutModule("Added!");
			Save();
		} else if(sCmdName == "del")
		{
			u_int iNum = atoi( sCommand.Token(1, true).c_str() );
			if ( iNum > m_vPerform.size() || iNum <= 0 )
			{
				PutModule( "Illegal # Requested");
				return;
			}
			else
			{
				m_vPerform.erase( m_vPerform.begin() + iNum - 1 );
				PutModule( "Command Erased.");
			}
			Save();
		} else if(sCmdName == "list")
		{
			int i = 1;
			CString sExpanded;
			for(VCString::iterator it = m_vPerform.begin(); it != m_vPerform.end(); it++, i++)
			{
				sExpanded = GetUser()->ExpandString( *it );
				if(sExpanded != *it)
					PutModule(CString( i ) + ": " + *it + " (" + sExpanded + ")");
				else
					PutModule(CString( i ) + ": " + *it);
			}
			PutModule(" -- End of List");
		} else if(sCmdName == "save")
		{
			if(Save())
				PutModule("Saved!");
			else
				PutModule("Error");
		} else if (sCmdName == "timer")
		{
			m_timer = sCommand.Token(1, true).ToUInt();
			SetNV("Time", CString(m_timer));
			PutModule("Timer set to " + CString(m_timer));
		}else
		{
			PutModule( "Commands: add <command>, del <nr>, list, save, timer <time>");
		}
	}

	virtual void OnIRCConnected()
	{
		AddTimer(new CTimedTimer(this, m_timer, 1, "TimedPerform", "This just does some perform goodies"));
	}

	void DoIt()
	{
		for( VCString::iterator it = m_vPerform.begin();
			it != m_vPerform.end();  it++)
		{
			PutIRC( GetUser()->ExpandString( *it ) );
		}
	}

private:
	bool Save()
	{
		CString sBuffer = "";
		
		for(VCString::iterator it = m_vPerform.begin(); it != m_vPerform.end(); it++)
		{
			sBuffer += *it + "\n";
		}
		SetNV("Perform", sBuffer);
		
		return true;
	}

	VCString	m_vPerform;
	unsigned int	m_timer;
};

void CTimedTimer::RunJob()
{
	CTimedPerform *p = (CTimedPerform *)m_pModule;
	p->DoIt();
}

MODULEDEFS(CTimedPerform, "Adds perform capabilities")
