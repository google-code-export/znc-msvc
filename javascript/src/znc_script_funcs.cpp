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

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s/s", &szMessage, &szModuleName))
		return JS_FALSE;

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutModule(szModuleName, szMessage)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(PutModNotice)
{
	GET_SCRIPT(pScript);
	char* szMessage = NULL;
	const char* szModuleName = pScript->GetName().c_str();

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s/s", &szMessage, &szModuleName))
		return JS_FALSE;

	const CString sModuleName(szModuleName);

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutUser(":" + pScript->GetUser()->GetStatusPrefix() + sModuleName + "!" +
			sModuleName + "@znc.in NOTICE " + pScript->GetUser()->GetCurNick() + " :" + CString(szMessage))
		));

	return JS_TRUE;
}


_ZNCJSFUNC(PutUser)
{
	GET_SCRIPT(pScript);
	char* szLine = NULL;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szLine))
		return JS_FALSE;

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutUser(szLine)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(PutIRC)
{
	GET_SCRIPT(pScript);
	char* szLine = NULL;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szLine))
		return JS_FALSE;

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutIRC(szLine)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(PutStatus)
{
	GET_SCRIPT(pScript);
	char* szLine = NULL;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szLine))
		return JS_FALSE;

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->GetUser()->PutStatus(szLine)
		));

	return JS_TRUE;
}


static JSBool _SendMsgOrNotice(const CString& sType, JSContext *cx, uintN argc, jsval *vp)
{
	char* szTo = NULL;
	char* szMsg = NULL;
	JSBool bAutoSplit = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "ss/b", &szTo, &szMsg, &bAutoSplit))
		return JS_FALSE;

	GET_SCRIPT(pScript);
	CUser* pUser = pScript->GetUser();
	const CString sTo(szTo), sMsg(szMsg);

	bool bToChan = pUser->IsChan(sTo);
	bool bSent = false;
	// split + ellipses messages exceeding 400 bytes:
	// we need to take into account possible UTF-8 multibyte characters.
	// this can be slowish, so we skip it if possible:

	if(!bAutoSplit || sMsg.size() <= 400) // unit: bytes, not characters!
	{
		bSent = pUser->PutIRC(sType + " " + sTo + " :" + sMsg);

		if(bSent && bToChan)
		{
			pUser->PutUser(":" + pUser->GetIRCNick().GetHostMask() + " " + sType + " " + sTo + " :" + sMsg);
		}
	}
	else
	{
		char *szMsgCopy = strdup(sMsg.c_str());
		char *p = szMsgCopy;
		size_t uPos = 0;

		do 
		{
			// IRC of course allows lines up to 512 bytes, but that includes new lines
			// and PRIVMSG plus the target name, so we assume about 400 bytes as max "payload" length...

			int uBytes = 0;
			while(*p && uBytes < 400)
			{
				char *old_p = p;
				// we assume that SpiderMonkey returns a valid UTF-8 string...
				p = g_utf8_find_next_char(p, NULL);
				uBytes += (p - old_p);
				if(uBytes > 6) { *old_p = 0; p = old_p; break; } // ...but one can never be too sure.
			}

			CString sLine = sMsg.substr(uPos, uBytes);

			if(*p) sLine += "..."; // will be continued...
			if(uPos > 0) sLine = "..." + sLine; // ...continuation

			uPos += uBytes;

			bSent = pUser->PutIRC(sType + " " + sTo + " :" + sLine);

			if(bSent && bToChan)
			{
				pUser->PutUser(":" + pUser->GetIRCNick().GetHostMask() + " " + sType + " " + sTo + " :" + sLine);
			}

		} while(bSent && *p);

		free(szMsgCopy);
	}

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(bSent));

	return JS_TRUE;
}

_ZNCJSFUNC(SendMessage)
{
	return _SendMsgOrNotice("PRIVMSG", cx, argc, vp);
}

_ZNCJSFUNC(SendNotice)
{
	return _SendMsgOrNotice("NOTICE", cx, argc, vp);
}


_ZNCJSFUNC(AddEventHandler)
{
	char* szEvent = NULL;
	JSObject* joCallbackDummy;
	jsval *argv = JS_ARGV(cx, vp);

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
		if(JS_AddValueRoot(cx, jvStored))
		{
			JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(JS_TRUE));

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
	jsval *argv = JS_ARGV(cx, vp);

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

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(bRemoved));

	return JS_TRUE;
}


_ZNCJSFUNC(GetModName)
{
	GET_SCRIPT(pScript);
	const CString sModName = pScript->GetName();

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sModName.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(GetModNick)
{
	GET_SCRIPT(pScript);
	const CString sModNick = pScript->GetUser()->GetStatusPrefix() + pScript->GetName();

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sModNick.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(GetStatusPrefix)
{
	GET_SCRIPT(pScript);
	const CString sPrefix = pScript->GetUser()->GetStatusPrefix();

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sPrefix.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(GetTag)
{
	JSBool bIncVer = JS_TRUE;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "/b", &bIncVer))
		return JS_FALSE;

	GET_SCRIPT(pScript);
	const CString sTag = pScript->GetZNC()->GetTag(bIncVer != JS_FALSE);

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sTag.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(GetVersion)
{
	GET_SCRIPT(pScript);
	const CString sVer = pScript->GetZNC()->GetVersion();

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sVer.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(GetUptime)
{
	GET_SCRIPT(pScript);
	const CString sUpTm = pScript->GetZNC()->GetUptime();

	JS_SET_RVAL(cx, vp,
		STRING_TO_JSVAL(JS_NewStringCopyZ(cx, sUpTm.c_str()))
		);

	return JS_TRUE;
}


_ZNCJSFUNC(TimeStarted)
{
	GET_SCRIPT(pScript);
	jsval rvalTmp;

	JSBool b = JS_NewNumberValue(cx, (unsigned int)pScript->GetZNC()->TimeStarted(), &rvalTmp);

	if(b) JS_SET_RVAL(cx, vp, rvalTmp);

	return b;
}


static JSBool _SetIntervalOrTimeout(JSContext *cx, uintN argc, jsval *vp, bool bRepeat)
{
	JSObject* joCallbackDummy;
	int32 iDelay;
	jsval *argv = JS_ARGV(cx, vp);

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

	JS_SET_RVAL(cx, vp, INT_TO_JSVAL(
		pScript->AddTimer(iDelay, bRepeat, argv[1])
	));

	return JS_TRUE;
}


_ZNCJSFUNC(SetInterval)
{
	return _SetIntervalOrTimeout(cx, argc, vp, true);
}


_ZNCJSFUNC(SetTimeout)
{
	return _SetIntervalOrTimeout(cx, argc, vp, false);
}


_ZNCJSFUNC(ClearIntervalOrTimeout)
{
	int32 iTimerId;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "i", &iTimerId))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		pScript->RemoveTimer(iTimerId)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(MD5)
{
	char* szData;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szData))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	const CString sHash = CString(szData).MD5();

	JS_SET_RVAL(cx, vp,  STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, sHash)
	));

	return JS_TRUE;
}


#ifdef HAVE_LIBSSL
_ZNCJSFUNC(SHA1)
{
	char *szData;

	if(JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szData))
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

		JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(
			CUtil::MsgCpyToJSStr(cx, digest_hex)
		));

		return JS_TRUE;
	}

	return JS_FALSE;
}
#endif


_ZNCJSFUNC(SHA256)
{
	char* szData;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szData))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	const CString sHash = CString(szData).SHA256();

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, sHash)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(WildCmp)
{
	char* szMask;
	char* szString;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "ss", &szMask, &szString))
		return JS_FALSE;

	JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(
		CString::WildCmp(szMask, szString)
	));

	return JS_TRUE;
}


_ZNCJSFUNC(GetUser)
{
	GET_SCRIPT(pScript);

	JS_SET_RVAL(cx, vp, 
		OBJECT_TO_JSVAL(pScript->GetJSUser()));

	return JS_TRUE;
}


_ZNCJSFUNC(User_GetName)
{
	GET_SCRIPT(pScript);

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, pScript->GetUser()->GetUserName())
	));

	return JS_TRUE;
}


_ZNCJSFUNC(StoreString)
{
	char* szName;
	char* szValue;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "ss", &szName, &szValue))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	JS_SET_RVAL(cx, vp,
		BOOLEAN_TO_JSVAL(pScript->SetNV(szName, szValue)));

	return JS_TRUE;
}


_ZNCJSFUNC(RetrieveString)
{
	char* szName;

	if(!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "s", &szName))
		return JS_FALSE;

	GET_SCRIPT(pScript);

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(
		CUtil::MsgCpyToJSStr(cx, pScript->GetNV(szName))
	));

	return JS_TRUE;
}

