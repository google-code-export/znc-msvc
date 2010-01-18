/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "main.h"
#include "util.h"

using namespace std;

#ifndef _WIN32
extern "C" {
#include "iconv_string.c"
}
#else
static CString WideToStr(const wchar_t *a_wideStr, unsigned int a_targetCodePage)
{
	int l_size = WideCharToMultiByte(a_targetCodePage, 0, a_wideStr, -1, NULL, NULL, NULL, NULL);

	if(l_size)
	{
		char *l_buf = new char[l_size];

		if(l_buf)
		{
			int l_chrs = WideCharToMultiByte(a_targetCodePage, 0, a_wideStr, -1, l_buf, l_size, NULL, NULL);
			CString l_result(l_buf);
			delete[] l_buf;
			return l_result;
		}
	}

	return "";
}


static wstring StrToWide(const CString& a_str, unsigned int a_originCodePage)
{
	int l_size = MultiByteToWideChar(a_originCodePage, 0, a_str.c_str(), -1, NULL, NULL);

	if(l_size)
	{
		wchar_t *l_buf = new wchar_t[l_size];

		if(l_buf)
		{
			int l_chrs = MultiByteToWideChar(a_originCodePage, 0, a_str.c_str(), -1, l_buf, l_size);
			wstring l_result(l_buf);
			delete[] l_buf;
			return l_result;
		}
	}

	return L"";
}
#endif


// please always keep in mind that sizeof(wchar_t) == sizeof(jschar) on
// Windows, but not in GCC!


#ifdef _WIN32

// :TODO: copy less stuff... :D

CString CUtil::WideToUtf8(const wstring& a_wideStr)
{
	return WideToStr(a_wideStr.c_str(), CP_UTF8);
}

CString CUtil::WideToUtf8(const jschar* jswszWideStr)
{
	return WideToStr((wchar_t*)jswszWideStr, CP_UTF8);
}

CString CUtil::WideToUtf8(const JSString* jsstrWideStr)
{
	size_t uBufLen = JS_GetStringLength(const_cast<JSString*>(jsstrWideStr)) + 1;
	wchar_t* wszBuf = new wchar_t[uBufLen];
	*(wchar_t*)(wszBuf + uBufLen - 1) = 0;
	wcscpy_s(wszBuf, uBufLen, (wchar_t*)JS_GetStringChars(const_cast<JSString*>(jsstrWideStr)));
	CString sResult = WideToStr(wszBuf, CP_UTF8);
	delete[] wszBuf;
	return sResult;
}

wstring CUtil::Utf8ToWide(const CString& a_utfStr)
{
	return StrToWide(a_utfStr, CP_UTF8);
}

jschar* CUtil::MsgCpyToWide(const CString& sMessage)
{
	size_t _dummy;
	return MsgCpyToWideEx(sMessage, _dummy);
}

jschar* CUtil::MsgCpyToWideEx(const CString& sMessage, size_t& ruNumChars)
{
	wstring wsMsg;

	if(g_utf8_validate(sMessage.c_str(), sMessage.size(), NULL))
	{
		wsMsg = Utf8ToWide(sMessage);
	}
	else
	{
		// ISO-8859-1/ANSI fallback...
		wsMsg = StrToWide(sMessage, CP_ACP);
	}
	// so we only support UTF-8 and ISO-8859-1 on Windows.

	ruNumChars = wsMsg.size(); // that easy! yay!

	jschar* jswszWideBuf = new jschar[wsMsg.size() + 1];
	wcscpy_s((wchar_t*)jswszWideBuf, ruNumChars + 1, wsMsg.c_str());

	return jswszWideBuf;
}

JSString* CUtil::MsgCpyToJSStr(JSContext* ctx, const CString& sString)
{
	const jschar* jswszBuf = CUtil::MsgCpyToWide(sString);
	JSString* jsMsg = JS_NewUCStringCopyZ(ctx, jswszBuf);
	delete[] jswszBuf;
	return jsMsg;
}

#else /* _WIN32 */

CString CUtil::WideToUtf8(const wstring& a_wideStr)
{
	char *szResult = NULL;
	if(iconv_string("UTF-8", "wchar_t", (char*)a_wideStr.c_str(),
		(char*)(a_wideStr.c_str() + a_wideStr.size() + 1), &szResult, NULL) >= 0)
	{
		CString sResult = szResult;
		free(szResult);
		return sResult;
	}
	return "";
}

CString CUtil::WideToUtf8(const jschar* jswszWideStr)
{
	char *szResult = NULL;
	if(iconv_string("UTF-8", "UTF-16", (char*)jswszWideStr,
		HOW THE FUCK DO WE GET THE LENGTH HERE, ADD OWN UTILITY SHIT
		(char*)(jswszWideStr + strlen(jswszWideStr) + 1), &szResult, NULL) >= 0)
	{
		CString sResult = szResult;
		free(szResult);
		return sResult;
	}
	return "";
}

CString CUtil::WideToUtf8(const JSString* jsstrWideStr)
{
	char *szResult = NULL;
	if(iconv_string("UTF-8", "UTF-16", (char*)JS_GetStringChars(const_cast<JSString*>(jsstrWideStr))),
		(char*)(JS_GetStringLength(const_cast<JSString*>(jsstrWideStr)) + 1), &szResult, NULL) >= 0)
	{
		CString sResult = szResult;
		free(szResult);
		return sResult;
	}
	return "";
}

#endif /* !_WIN32 */
