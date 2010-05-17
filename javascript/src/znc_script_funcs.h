/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_JS_SCRIPT_FUNCS_H
#define _ZNC_JS_SCRIPT_FUNCS_H

#include "jsapi.h"

#define _ZNCJSFUNC_H(FUNC_NAME) static JSBool Script_##FUNC_NAME(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#define ZNCJSFUNC_NAME(FUNC_NAME) CZNCScriptFuncs::Script_##FUNC_NAME

class CZNCScriptFuncs
{
public:
	/* ZNC */
	_ZNCJSFUNC_H(PutModule);
	_ZNCJSFUNC_H(PutModNotice);
	_ZNCJSFUNC_H(PutIRC);
	_ZNCJSFUNC_H(PutUser);
	_ZNCJSFUNC_H(PutStatus);
	_ZNCJSFUNC_H(SendMessage);
	_ZNCJSFUNC_H(SendNotice);
	_ZNCJSFUNC_H(GetModName);
	_ZNCJSFUNC_H(GetModNick);
	_ZNCJSFUNC_H(GetStatusPrefix);
	_ZNCJSFUNC_H(GetTag);
	_ZNCJSFUNC_H(GetVersion);
	_ZNCJSFUNC_H(GetUptime);
	_ZNCJSFUNC_H(TimeStarted);

	_ZNCJSFUNC_H(AddEventHandler);
	_ZNCJSFUNC_H(RemoveEventHandler);
	_ZNCJSFUNC_H(SetInterval);
	_ZNCJSFUNC_H(SetTimeout);
	_ZNCJSFUNC_H(ClearIntervalOrTimeout);
	_ZNCJSFUNC_H(GetUser);
	_ZNCJSFUNC_H(StoreString);
	_ZNCJSFUNC_H(RetrieveString);

	/* (global) */
	_ZNCJSFUNC_H(MD5);
#ifdef HAVE_LIBSSL
	_ZNCJSFUNC_H(SHA1);
#endif
	_ZNCJSFUNC_H(SHA256);
	_ZNCJSFUNC_H(WildCmp);

	/* Nick */
	_ZNCJSFUNC_H(Nick_GetNick);
	_ZNCJSFUNC_H(Nick_GetIdent);
	_ZNCJSFUNC_H(Nick_GetHost);
	_ZNCJSFUNC_H(Nick_GetHostMask);
	_ZNCJSFUNC_H(Nick_GetPermStr);
	_ZNCJSFUNC_H(Nick_HasPerm);

	/* Chan */
	_ZNCJSFUNC_H(Chan_GetName);

	/* User */
	_ZNCJSFUNC_H(User_GetName);
};

#define _ZNCJSFUNC(FUNC_NAME) JSBool CZNCScriptFuncs::Script_##FUNC_NAME(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#define GET_SCRIPT(VARNAME) CZNCScript* VARNAME = reinterpret_cast<CZNCScript*>(JS_GetContextPrivate(cx));

#endif /* !_ZNC_JS_SCRIPT_FUNCS_H */
