/*
* Copyright (C) 2010 Ingmar Runge
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#include "main.h"
#include "znc_script.h"
#include "znc_script_funcs.h"
#include "znc_js_mod.h"
#include "util.h"

#include "znc_js_mod_events.inc"

using namespace std;

/************************************************************************/
/* CHANNEL OBJECT CLASS                                                 */
/************************************************************************/

static JSBool znc_chan_construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *szChanName = new char[1024]; // 1023 chars ought to be enough for any channel name.
	if(!szChanName)
	{
		JS_ReportOutOfMemory(cx);
		return JS_FALSE;
	}
	*szChanName = 0;
	JS_SetPrivate(cx, obj, szChanName);
	return JS_TRUE;
}


static void znc_chan_finalize(JSContext *cx, JSObject *obj)
{
	char* szChanName = reinterpret_cast<char*>(JS_GetPrivate(cx, obj));
	delete[] szChanName;
}


static JSClass s_chan_class = {
	"znc_chan_class", JSCLASS_HAS_PRIVATE | JSCLASS_CONSTRUCT_PROTOTYPE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, znc_chan_finalize,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec s_chan_functions[] = {
	JS_FS("GetName",		ZNCJSFUNC_NAME(Chan_GetName),		0, 0, 0),
	JS_FS_END
};

static JSPropertySpec s_chan_properties[] = {
	{0}
};


#define _GET_CHAN \
	GET_SCRIPT(pScript); \
	const char* szChanName = reinterpret_cast<char*>( \
		JS_GetInstancePrivate(pScript->GetContext(), obj, &s_chan_class, NULL)); \
	CChan* pChan = pScript->GetUser()->FindChan(szChanName); \
	if(!pChan) \
	{ \
		const CString sTmpError = "Channel " + CString(szChanName) + " has been removed " \
			"and can no longer be accessed from the script."; \
		JS_ReportError(cx, sTmpError.c_str()); \
		return JS_FALSE; \
	}


_ZNCJSFUNC(Chan_GetName)
{
	_GET_CHAN;

	*rval = STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, pChan->GetName()));

	return JS_TRUE;
}


JSObject* CZNCScript::MakeChanObject(const CChan& Chan) const
{
	JSObject* joNew = JS_InitClass(m_jsContext, m_jsGlobalObj, NULL,
		&s_chan_class, znc_chan_construct, 0, s_chan_properties,
		s_chan_functions, NULL, NULL);

	char *szChanName = reinterpret_cast<char*>(
		JS_GetInstancePrivate(m_jsContext, joNew, &s_chan_class, NULL));

	strncpy(szChanName, Chan.GetName().c_str(), 1023);
	szChanName[1023] = 0;

	return joNew;
}
