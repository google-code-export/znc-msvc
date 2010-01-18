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

	return true;
}


void CJavaScriptMod::OnModCommand(const CString& sCommand)
{
	CString sError;
	CZNCScript *scr = new CZNCScript(this, "print", "Y:/Dev/js-1.8.0/js/src/Y2.js");
	if(scr->LoadScript(sError)) m_scripts.insert(scr); else delete scr;
}


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
