/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "ZNCString.h"
#include "znc_smjs.h"

class CUtil
{
public:
	static CString WideToUtf8(const std::wstring& wWideStr);
//	static CString WideToUtf8(const jschar* jswszWideStr);
	static CString WideToUtf8(const JSString* jsstrWideStr);
	static std::wstring Utf8ToWide(const CString& sUtfStr);
	static jschar* MsgCpyToWide(const CString& sMessage);
	static jschar* MsgCpyToWideEx(const CString& sMessage, size_t& ruNumChars);
	static JSString* MsgCpyToJSStr(JSContext* ctx, const CString& sString);
};

extern "C"
{
	int g_utf8_validate(const char *str, size_t max_len, const char **end);
	char *g_utf8_find_next_char(const char *p, const char *end = NULL);
}

#endif /* !_UTIL_H */
