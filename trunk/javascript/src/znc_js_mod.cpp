/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "main.h"
#include "Modules.h"
#include "jsapi.h"
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
		JS_SetCStringsAreUTF8();

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
/* SCRIPT BUSINESS                                                      */
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
