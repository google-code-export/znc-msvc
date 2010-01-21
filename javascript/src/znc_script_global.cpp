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
/* GLOBAL CLASS                                                         */
/************************************************************************/

static JSClass s_global_class = {
	"znc_global_class", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec s_global_functions[] = {
	JS_FS("MD5",			ZNCJSFUNC_NAME(MD5),					1, 0, 0),
#ifdef HAVE_LIBSSL
	JS_FS("SHA1",			ZNCJSFUNC_NAME(SHA1),					1, 0, 0),
#endif
	JS_FS("SHA256",			ZNCJSFUNC_NAME(SHA256),					1, 0, 0),
	JS_FS("WildCmp",		ZNCJSFUNC_NAME(WildCmp),				2, 0, 0),
	JS_FS_END
};


/************************************************************************/
/* ZNC CLASS                                                            */
/************************************************************************/

JSBool znc_class_get_prop(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

static JSClass s_znc_class = {
	"znc_class", 0,
	JS_PropertyStub, JS_PropertyStub, znc_class_get_prop, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec s_znc_functions[] = {
	JS_FS("PutModule",			ZNCJSFUNC_NAME(PutModule),			2, 0, 0),
	JS_FS("PutIRC",				ZNCJSFUNC_NAME(PutIRC),				1, 0, 0),
	JS_FS("PutUser",			ZNCJSFUNC_NAME(PutUser),			1, 0, 0),
	JS_FS("SendMessage",		ZNCJSFUNC_NAME(SendMessage),		2, 0, 0),
	JS_FS("SendNotice",			ZNCJSFUNC_NAME(SendNotice),			2, 0, 0),

	JS_FS("GetUser",			ZNCJSFUNC_NAME(GetUser),			0, 0, 0),
	JS_FS("StoreString",		ZNCJSFUNC_NAME(StoreString),		2, 0, 0),
	JS_FS("RetrieveString",		ZNCJSFUNC_NAME(RetrieveString),		1, 0, 0),

	JS_FS("AddEventHandler",	ZNCJSFUNC_NAME(AddEventHandler),	2, 0, 0),
	JS_FS("RemoveEventHandler",	ZNCJSFUNC_NAME(RemoveEventHandler), 2, 0, 0),

	JS_FS("GetTag",				ZNCJSFUNC_NAME(GetTag),				1, 0, 0),
	JS_FS("GetVersion",			ZNCJSFUNC_NAME(GetVersion),			0, 0, 0),
	JS_FS("GetUptime",			ZNCJSFUNC_NAME(GetUptime),			0, 0, 0),
	JS_FS("TimeStarted",		ZNCJSFUNC_NAME(TimeStarted),		0, 0, 0),

	JS_FS("SetInterval",		ZNCJSFUNC_NAME(SetInterval),		2, 0, 0),
	JS_FS("SetTimeout",			ZNCJSFUNC_NAME(SetTimeout),			2, 0, 0),
	JS_FS("ClearInterval",	ZNCJSFUNC_NAME(ClearIntervalOrTimeout),	1, 0, 0),
	JS_FS("ClearTimeout",	ZNCJSFUNC_NAME(ClearIntervalOrTimeout),	1, 0, 0),

	JS_FS_END
};

enum ZNCProperties
{
	PROP_VERSION_MAJOR, PROP_VERSION_MINOR,
	PROP_CONST_CONTINUE, PROP_CONST_HALT, PROP_CONST_HALTCORE, PROP_CONST_HALTMODS
};

static JSPropertySpec s_znc_properties[] = {
	{"VERSION_MAJOR",	PROP_VERSION_MAJOR,		JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE},
	{"VERSION_MINOR",	PROP_VERSION_MINOR,		JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE},

	{"CONTINUE",		PROP_CONST_CONTINUE,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"HALT",			PROP_CONST_HALT,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"HALTCORE",		PROP_CONST_HALTCORE,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"HALTMODS",		PROP_CONST_HALTMODS,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};


static JSBool znc_class_get_prop(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	if(JSVAL_IS_INT(id))
	{
		switch(JSVAL_TO_INT(id))
		{
		case PROP_VERSION_MAJOR: *vp = INT_TO_JSVAL(VERSION_MAJOR); break;
		case PROP_VERSION_MINOR: *vp = INT_TO_JSVAL(VERSION_MINOR); break;
		case PROP_CONST_CONTINUE: *vp = INT_TO_JSVAL(CModule::CONTINUE); break;
		case PROP_CONST_HALT: *vp = INT_TO_JSVAL(CModule::HALT); break;
		case PROP_CONST_HALTCORE: *vp = INT_TO_JSVAL(CModule::HALTCORE); break;
		case PROP_CONST_HALTMODS: *vp = INT_TO_JSVAL(CModule::HALTMODS); break;
		}
	}

	return JS_TRUE;
}


bool CZNCScript::SetUpGlobalClasses(CString& srErrorMessage)
{
	m_jsGlobalObj = JS_NewObject(m_jsContext, &s_global_class, NULL, NULL);
	if(!m_jsGlobalObj)
	{
		srErrorMessage = "Creating the global object failed!";
		return false;
	}

	if(!JS_InitStandardClasses(m_jsContext, m_jsGlobalObj))
	{
		srErrorMessage = "Adding the standard classes to the global object failed!";
		return false;
	}

	JS_DefineFunctions(m_jsContext, m_jsGlobalObj, s_global_functions);

	JSObject* jsoZNC = JS_NewObject(m_jsContext, &s_znc_class, NULL, NULL);
	JS_DefineFunctions(m_jsContext, jsoZNC, s_znc_functions);
	JS_DefineProperties(m_jsContext, jsoZNC, s_znc_properties);
	jsval jvZNCNamedObj = OBJECT_TO_JSVAL(jsoZNC);

	JS_DefineProperty(m_jsContext, m_jsGlobalObj, "ZNC", jvZNCNamedObj,
		NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT);

	return true;
}

