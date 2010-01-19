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

using namespace std;


/************************************************************************/
/* ZNC USER SCRIPT CLASS                                                */
/************************************************************************/

static JSClass s_user_class = {
	"znc_user_class", 0,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec s_user_functions[] = {
	JS_FS("GetName",		ZNCJSFUNC_NAME(User_GetName),		0, 0, 0),
	JS_FS("GetUserName",	ZNCJSFUNC_NAME(User_GetName),		0, 0, 0),
	JS_FS_END
};

static JSPropertySpec s_user_properties[] = {
	{0}
};


bool CZNCScript::SetUpUserObject()
{
	JSObject* joUser = JS_NewObject(m_jsContext, &s_user_class, NULL, NULL);

	if(joUser)
	{
		JS_DefineProperties(m_jsContext, joUser, s_user_properties);
		JS_DefineFunctions(m_jsContext, joUser, s_user_functions);

		m_jvUserObj = OBJECT_TO_JSVAL(joUser);
		if(JS_AddRoot(m_jsContext, &m_jvUserObj))
		{
			return true;
		}
	}

	return false;
}
