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
/* NICK OBJECT CLASS                                                    */
/************************************************************************/

static JSClass s_nick_class;


static JSBool znc_nick_construct(JSContext *cx, uintN argc, jsval *vp)
{
	if(JSObject *obj = JS_NewObject(cx, &CZNCScriptFuncs::s_nick_class, NULL, NULL))
	{
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
		return JS_TRUE;
	}

	return JS_FALSE;
}


static void znc_nick_finalize(JSContext *cx, JSObject *obj)
{
	CNick* pNick = reinterpret_cast<CNick*>(JS_GetInstancePrivate(cx, obj, &CZNCScriptFuncs::s_nick_class, NULL));
	delete pNick;
}


static JSFunctionSpec s_nick_functions[] = {
	JS_FS("GetNick",		ZNCJSFUNC_NAME(Nick_GetNick),		0, 0),
	JS_FS("GetIdent",		ZNCJSFUNC_NAME(Nick_GetIdent),		0, 0),
	JS_FS("GetHost",		ZNCJSFUNC_NAME(Nick_GetHost),		0, 0),
	JS_FS("GetHostMask",	ZNCJSFUNC_NAME(Nick_GetHostMask),	0, 0),
	JS_FS("GetPermStr",		ZNCJSFUNC_NAME(Nick_GetPermStr),	0, 0),
	JS_FS("HasPerm",		ZNCJSFUNC_NAME(Nick_HasPerm),		1, 0),
	JS_FS_END
};


enum NickProperties
{
	PROP_NICK, PROP_IDENT, PROP_HOST
};

static JSPropertySpec s_nick_properties[] = {
	{"nick",	PROP_NICK,	JSPROP_PERMANENT | JSPROP_ENUMERATE},
	{"ident",	PROP_IDENT,	JSPROP_PERMANENT | JSPROP_ENUMERATE},
	{"host",	PROP_HOST,	JSPROP_PERMANENT | JSPROP_ENUMERATE},
	{0}
};


#define _GET_NICK GET_SCRIPT(pScript); \
	CNick* pCNick = reinterpret_cast<CNick*>( \
	JS_GetInstancePrivate(pScript->GetContext(), JSVAL_TO_OBJECT(JS_THIS(cx, vp)), &s_nick_class, NULL))


#define _NICK_STR_METHOD(METHOD) \
	_ZNCJSFUNC(Nick_##METHOD) \
{	_GET_NICK; \
	jsval jvStr = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), pCNick->METHOD())); \
	JS_SET_RVAL(cx, vp, jvStr); \
	return JS_TRUE; \
	}

_NICK_STR_METHOD(GetNick)
_NICK_STR_METHOD(GetIdent)
_NICK_STR_METHOD(GetHost)
_NICK_STR_METHOD(GetHostMask)
_NICK_STR_METHOD(GetPermStr)


_ZNCJSFUNC(Nick_HasPerm) \
{
	_GET_NICK;
	unsigned char *ch = NULL;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &ch))
		return JS_FALSE;

	if(!ch || !*ch)
	{
		JS_ReportError(cx, "Invalid mode character to Nick.HasPerm!");
		return JS_FALSE;
	}

	JS_SET_RVAL(cx, vp,
		BOOLEAN_TO_JSVAL(pCNick->HasPerm(ch[0]))
		);

	return JS_TRUE;
}

static JSBool znc_nick_get_prop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	jsval jvId;
	if(JS_IdToValue(cx, id, &jvId) && JSVAL_IS_INT(jvId))
	{
		_GET_NICK;
		CString sStr;

		switch(JSVAL_TO_INT(jvId))
		{
		case PROP_NICK: sStr = pCNick->GetNick(); break;
		case PROP_IDENT: sStr = pCNick->GetIdent(); break;
		case PROP_HOST: sStr = pCNick->GetHost(); break;
		}

		jsval jvStr = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sStr));
		*vp = jvStr;
	}

	return JS_TRUE;
}


static JSBool znc_nick_set_prop(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	jsval jvId;
	if(JS_IdToValue(cx, id, &jvId) && JSVAL_IS_INT(jvId))
	{
		_GET_NICK;
		CString sStr;

		if(!JSVAL_IS_STRING(*vp))
		{
			return JS_FALSE;
		}

		sStr = CUtil::WideToUtf8(JSVAL_TO_STRING(*vp));

		switch(JSVAL_TO_INT(jvId))
		{
		case PROP_NICK: pCNick->SetNick(sStr); break;
		case PROP_IDENT: pCNick->SetIdent(sStr); break;
		case PROP_HOST: pCNick->SetHost(sStr); break;
		default:
			return JS_FALSE;
		}
	}

	return JS_TRUE;
}


JSClass CZNCScriptFuncs::s_nick_class = {
	"znc_nick_class", JSCLASS_HAS_PRIVATE | JSCLASS_CONSTRUCT_PROTOTYPE,
	JS_PropertyStub, JS_PropertyStub, znc_nick_get_prop, znc_nick_set_prop,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, znc_nick_finalize,
	JSCLASS_NO_OPTIONAL_MEMBERS
};


JSObject* CZNCScript::MakeNickObject(const CNick* pNick) const
{
	JSObject* joNew = JS_InitClass(m_jsContext, m_jsGlobalObj, NULL,
		&CZNCScriptFuncs::s_nick_class, znc_nick_construct, 0, s_nick_properties,
		s_nick_functions, NULL, NULL);

	if(pNick)
	{
		CNick* pNewNick = new CNick();
		pNewNick->Clone(*pNick);

		// :TODO: consider moving this to the constructor
		JS_SetPrivate(m_jsContext, joNew, pNewNick);
	}

	return joNew;
}
