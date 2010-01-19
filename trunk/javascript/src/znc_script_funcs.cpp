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
#ifdef HAVE_LIBSSL
#include <openssl/sha.h>
#endif

/* please note that a lot of code here would fuck up pretty
	bad if JS_CStringsAreUTF8 was false. */


_ZNCJSFUNC(PutModule)
{
	GET_SCRIPT(pScript);
	char* szMessage = NULL;
	const char* szModuleName = pScript->GetName().c_str();

	if(!JS_ConvertArguments(cx, argc, argv, "s/s", &szMessage, &szModuleName))
		return JS_FALSE;

	*rval = BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutModule(szModuleName, szMessage)
	);

	return JS_TRUE;
}


_ZNCJSFUNC(PutUser)
{
	GET_SCRIPT(pScript);
	char* szLine = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "s", &szLine))
		return JS_FALSE;

	*rval = BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutUser(szLine)
	);

	return JS_TRUE;
}


_ZNCJSFUNC(PutIRC)
{
	GET_SCRIPT(pScript);
	char* szLine = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "s", &szLine))
		return JS_FALSE;

	*rval = BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutIRC(szLine)
	);

	return JS_TRUE;
}


_ZNCJSFUNC(SendMessage)
{
	char* szTo = NULL;
	char* szMsg = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "ss", &szTo, &szMsg))
		return JS_FALSE;

	GET_SCRIPT(pScript);
	const CString sTo(szTo), sMsg(szMsg);
	CUser* pUser = pScript->GetUser();

	bool bSent = pUser->PutIRC("PRIVMSG " + sTo + " :" + sMsg);

	if(bSent && pUser->IsChan(sTo))
	{
		pUser->PutUser(":" + pUser->GetIRCNick().GetHostMask() + " PRIVMSG " + sTo + " :" + sMsg);
	}

	*rval = BOOLEAN_TO_JSVAL(bSent);

	return JS_TRUE;
}


_ZNCJSFUNC(SendNotice)
{
	char* szTo = NULL;
	char* szMsg = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "ss", &szTo, &szMsg))
		return JS_FALSE;

	GET_SCRIPT(pScript);
	const CString sTo(szTo), sMsg(szMsg);
	CUser* pUser = pScript->GetUser();

	bool bSent = pUser->PutIRC("NOTICE " + sTo + " :" + sMsg);

	if(bSent && pUser->IsChan(sTo))
	{
		pUser->PutUser(":" + pUser->GetIRCNick().GetHostMask() + " NOTICE " + sTo + " :" + sMsg);
	}

	*rval = BOOLEAN_TO_JSVAL(bSent);

	return JS_TRUE;
}


_ZNCJSFUNC(AddEventHandler)
{
	char* szEvent = NULL;
	JSObject* joCallbackDummy;

	if(!JS_ConvertArguments(cx, argc, argv, "so", &szEvent, &joCallbackDummy))
		return JS_FALSE;

	if(!JSVAL_IS_OBJECT(argv[1]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(argv[1])))
	{
		JS_ReportError(cx, "The second argument to "
			"AddEventHandler has to be a function or function object.");

		return JS_FALSE;
	}

	GET_SCRIPT(pScript);

	jsval* jvStored = pScript->StoreEventHandler(szEvent, argv[1]);

	if(jvStored != NULL)
	{
		if(JS_AddRoot(cx, jvStored))
		{
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

			return JS_TRUE;
		}
		else
		{
			JS_ReportError(cx, "Unable to store event handler (JS_AddRoot failed)!");
		}
	}
	else
	{
		JS_ReportError(cx, "Unable to store event handler. "
			"Please make sure you passed in a valid event name.");
	}

	return JS_FALSE;
}


_ZNCJSFUNC(RemoveEventHandler)
{
	char* szEvent = NULL;
	JSObject* joCallbackDummy;

	if(!JS_ConvertArguments(cx, argc, argv, "so", &szEvent, &joCallbackDummy))
		return JS_FALSE;

	if(!JSVAL_IS_OBJECT(argv[1]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(argv[1])))
	{
		JS_ReportError(cx, "The second argument to "
			"RemoveEventHandler has to be a function or function object.");

		return JS_FALSE;
	}

	GET_SCRIPT(pScript);

	bool bRemoved = pScript->RemoveEventHandler(szEvent, argv[1]);

	*rval = BOOLEAN_TO_JSVAL(bRemoved);

	return JS_TRUE;
}


_ZNCJSFUNC(GetTag)
{
	JSBool bIncVer = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, argv, "/b", &bIncVer))
		return JS_FALSE;

	GET_SCRIPT(pScript);
	const CString sTag = pScript->GetZNC()->GetTag(bIncVer != JS_FALSE);
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sTag.c_str()));

	return JS_TRUE;
}


_ZNCJSFUNC(GetVersion)
{
	GET_SCRIPT(pScript);
	const CString sVer = pScript->GetZNC()->GetVersion();
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sVer.c_str()));

	return JS_TRUE;
}


_ZNCJSFUNC(GetUptime)
{
	GET_SCRIPT(pScript);
	const CString sUpTm = pScript->GetZNC()->GetUptime();
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sUpTm.c_str()));

	return JS_TRUE;
}


_ZNCJSFUNC(TimeStarted)
{
	GET_SCRIPT(pScript);

	return JS_NewNumberValue(cx, (unsigned int)pScript->GetZNC()->TimeStarted(), rval);
}


static JSBool _SetIntervalOrTimeout(JSContext *cx, JSObject *obj,
	uintN argc, jsval *argv, jsval *rval, bool bRepeat)
{
	JSObject* joCallbackDummy;
	int32 iDelay;

	if(!JS_ConvertArguments(cx, argc, argv, "io", &iDelay, &joCallbackDummy))
		return JS_FALSE;

	if(!JSVAL_IS_OBJECT(argv[1]) || !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(argv[1])))
	{
		JS_ReportError(cx, "The second argument to SetTimeout/SetInterval has to be a function or function object.");
		return JS_FALSE;
	}

	if(iDelay < 1)
	{
		JS_ReportError(cx, "The delay has to be at least one second.");
		return JS_FALSE;
	}

	GET_SCRIPT(pScript);

	*rval = INT_TO_JSVAL(
		pScript->AddTimer(iDelay, bRepeat, argv[1])
	);

	return JS_TRUE;
}


_ZNCJSFUNC(SetInterval)
{
	return _SetIntervalOrTimeout(cx, obj, argc, argv, rval, true);
}


_ZNCJSFUNC(SetTimeout)
{
	return _SetIntervalOrTimeout(cx, obj, argc, argv, rval, false);
}


_ZNCJSFUNC(ClearIntervalOrTimeout)
{
	int32 iTimerId;

	if(!JS_ConvertArguments(cx, argc, argv, "i", &iTimerId))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	*rval = BOOLEAN_TO_JSVAL(
		pScript->RemoveTimer(iTimerId)
	);

	return JS_TRUE;
}


_ZNCJSFUNC(MD5)
{
	char* szData;

	if(!JS_ConvertArguments(cx, argc, argv, "s", &szData))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	const CString sHash = CString(szData).MD5();

	*rval = STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, sHash)
	);

	return JS_TRUE;
}


#ifdef HAVE_LIBSSL
_ZNCJSFUNC(SHA1)
{
	char *szData;

	if(JS_ConvertArguments(cx, argc, argv, "s", &szData))
	{
		char digest_hex[SHA_DIGEST_LENGTH * 2 + 1];
		unsigned char digest[SHA_DIGEST_LENGTH];
		memset(digest, 0, SHA_DIGEST_LENGTH);
		SHA_CTX ctx;

		SHA1_Init(&ctx);
		SHA1_Update(&ctx, szData, strlen(szData));
		SHA1_Final(digest, &ctx);

		sprintf(digest_hex,
			"%02x%02x%02x%02x%02x%02x%02x%02x"
			"%02x%02x%02x%02x%02x%02x%02x%02x"
			"%02x%02x%02x%02x",
			digest[ 0], digest[ 1], digest[ 2], digest[ 3], digest[ 4], digest[ 5], digest[ 6], digest[ 7],
			digest[ 8], digest[ 9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15],
			digest[16], digest[17], digest[18], digest[19]);

		*rval = STRING_TO_JSVAL(
			CUtil::MsgCpyToJSStr(cx, digest_hex)
		);

		return JS_TRUE;
	}

	return JS_FALSE;
}
#endif


_ZNCJSFUNC(SHA256)
{
	char* szData;

	if(!JS_ConvertArguments(cx, argc, argv, "s", &szData))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	const CString sHash = CString(szData).SHA256();

	*rval = STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, sHash)
		);

	return JS_TRUE;
}


_ZNCJSFUNC(WildCmp)
{
	char* szMask;
	char* szString;

	if(!JS_ConvertArguments(cx, argc, argv, "ss", &szMask, &szString))
		return JS_FALSE;

	*rval = BOOLEAN_TO_JSVAL(
		CString::WildCmp(szMask, szString)
	);

	return JS_TRUE;
}


_ZNCJSFUNC(GetUser)
{
	GET_SCRIPT(pScript);

	*rval = *pScript->GetJSUser();

	return JS_TRUE;
}


_ZNCJSFUNC(User_GetName)
{
	GET_SCRIPT(pScript);

	*rval = STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, pScript->GetUser()->GetUserName())
	);

	return JS_TRUE;
}
