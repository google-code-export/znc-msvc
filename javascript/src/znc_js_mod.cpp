/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "main.h"
#include "Modules.h"
#include "znc_smjs.h"
#include "znc_js_mod.h"
#include "znc_js_mod_events.h"
#include "util.h"

int CJavaScriptMod::ms_uNumberOfInstances = 0;
JSRuntime* CJavaScriptMod::ms_jsRuntime = NULL;

/************************************************************************/
/* CONSTRUCTOR / INITIALIZERS                                           */
/************************************************************************/

MODCONSTRUCTOR(CJavaScriptMod::CJavaScriptMod)
{
	ms_uNumberOfInstances++;
}


bool CJavaScriptMod::OnLoad(const CString& sArgs, CString& sMessage)
{
	if(!ms_jsRuntime)
	{
#if JS_VERSION >= 180
		JS_SetCStringsAreUTF8();
#else
		if(!JS_CStringsAreUTF8())
		{
			sMessage = "SpiderMonkey 1.7 needs to be compiled with JS_C_STRINGS_ARE_UTF8.";
			return false;
		}
#endif

		// 8 MB RAM ought to be enough for anybody!
		ms_jsRuntime = JS_NewRuntime(8L * 1024L * 1024L);
		// (it's not actually a hard limit, just the number of bytes
		// after which the GC will kick in, yo)
	}

	if(!ms_jsRuntime)
	{
		sMessage = "Initializing the JavaScript runtime FAILED!";
		return false;
	}

	/* warning: when a script is loaded from here, error messages to PutModule
		will most probably get lost during ZNC startup */

	LoadFromDisk();

	return true;
}


/************************************************************************/
/* User command handling                                                */
/************************************************************************/

void CJavaScriptMod::OnModCommand(const CString& sCommand)
{
	const CString sCmd = sCommand.Token(0);

	if(sCmd.Equals("LISTMODS") || sCmd.Equals("LISTSCRIPTS"))
	{
		CTable tMods;
		tMods.AddColumn("Name");
		tMods.AddColumn("Arguments");

		for(set<CZNCScript*>::const_iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
		{
			tMods.AddRow();
			tMods.SetCell("Name", (*it)->GetName());
			tMods.SetCell("Arguments", (*it)->GetArguments());
		}

		PutModule(tMods);
	}
	else
	{
		PutModule("Command not understood. Please note that you can't execute any JavaScript commands from IRC.");
	}
}


/************************************************************************/
/* CONFIG UTILS                                                         */
/************************************************************************/

void CJavaScriptMod::SaveToDisk()
{
	CString sModules;

	for(set<CZNCScript*>::const_iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		sModules += (*it)->GetName();
		sModules += ":";
		sModules += (*it)->GetArguments();
		sModules += "\n";
	}

	SetNV("activeModules", sModules);

	// save other stuff...
}


void CJavaScriptMod::LoadFromDisk()
{
	CString sModules = GetNV("activeModules");
	VCString vsModules;

	sModules.Split("\n", vsModules, false);

	for(VCString::const_iterator it = vsModules.begin(); it != vsModules.end(); it++)
	{
		const CString sMod = (*it).Token(0, false, ":");
		const CString sArgs = (*it).Token(1, true, ":");
		CString sError;

		if(!LoadModule(sMod, sArgs, sError))
		{
			PutStatus("Unable to load [" + sMod + ".js] [" + sError + "].");
		}
	}

	// load other stuff...
}


/************************************************************************/
/* SCRIPT LOADING BUSINESS                                              */
/************************************************************************/

bool CJavaScriptMod::LoadModule(const CString& sName, const CString& sArgs, CString& srErrorMessage)
{
	if(sName.empty())
	{
		srErrorMessage = "Usage: LoadMod <module> [args]";
		return false;
	}

	if(sName.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz_-.") != CString::npos)
	{
		srErrorMessage = "Module names can only contain 0-9, a-z, _ and -.";
		return false;
	}

	for(set<CZNCScript*>::const_iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		if((*it)->GetName() == sName)
		{
			srErrorMessage = "A module of this name has already been loaded.";
			return false;
		}
	}

	CString sModPath, sTmp;

	if (!CModules::FindModPath(sName + ".js", sModPath, sTmp))
	{
		srErrorMessage = "No such module";
	}
	else
	{
		CZNCScript *pScript = new CZNCScript(this, sName, sModPath);

		if(pScript->LoadScript(srErrorMessage))
		{
			m_scripts.insert(pScript);

			return true;
		}
		else 
		{
			delete pScript;
		}
	}

	return false;
}


bool CJavaScriptMod::UnLoadModule(const CString& sName, CString& srErrorMessage)
{
	for(set<CZNCScript*>::const_iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		if((*it)->GetName() == sName)
		{
			m_scripts.erase(it);
			return true;
		}
	}

	srErrorMessage = "A module of this name is not loaded.";
	return false;
}


/************************************************************************/
/* CLEANUP / DESTRUCTOR                                                 */
/************************************************************************/

CJavaScriptMod::~CJavaScriptMod()
{
	ms_uNumberOfInstances--;

	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		delete *it;
	}

	if(ms_jsRuntime)
	{
		JS_DestroyRuntime(ms_jsRuntime);
		ms_jsRuntime = NULL;
	}

	if(ms_uNumberOfInstances == 0)
	{
		JS_ShutDown();
	}
}


MODULEDEFS(CJavaScriptMod, "Makes ZNC scriptable with JavaScript!")
