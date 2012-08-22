/*
 * Copyright (C) 2012 Ingmar Runge
 * Copyright (C) 2010 cxxjoe
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "win32_util.h"

// Credit for some of these utility functions:
// cxxjoe @ http://code.google.com/p/infekt/source/browse/trunk/src/lib/util.cpp
// GPLv2 2011

#pragma comment(lib, "Shlwapi.lib")


bool CWinUtils::RemoveCwdFromDllSearchPath()
{
	// available from XP SP1:
	if(CWinUtils::WinVerAtLeast(5, 1, 1))
	{
		typedef BOOL (WINAPI *fsdd)(LPCWSTR);

		// remove the current directory from the DLL search path by calling SetDllDirectory:
		fsdd l_fsdd = (fsdd)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetDllDirectory");
		if(l_fsdd)
		{
			return (l_fsdd(L"") != FALSE);
		}
	}

	return false;
}


bool CWinUtils::HardenHeap()
{
#ifndef _DEBUG
	// Activate program termination on heap corruption.
	// http://msdn.microsoft.com/en-us/library/aa366705%28VS.85%29.aspx
	typedef BOOL (WINAPI *fhsi)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	fhsi l_fHSI = (fhsi)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "HeapSetInformation");
	if(l_fHSI)
	{
		return (l_fHSI(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0) != FALSE);
	}
#endif
	return false;
}


#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x01
#endif

bool CWinUtils::EnforceDEP()
{
#ifndef _WIN64
	// Explicitly activate DEP, especially important for XP SP3.
	// http://msdn.microsoft.com/en-us/library/bb736299%28VS.85%29.aspx
	typedef BOOL (WINAPI *fspdp)(DWORD);
	fspdp l_fSpDp = (fspdp)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetProcessDEPPolicy");
	if(l_fSpDp)
	{
		return (l_fSpDp(PROCESS_DEP_ENABLE) != FALSE);
	}

	return false;
#else
	return true; // always enabled on x64 anyway.
#endif
}


bool CWinUtils::WinVerAtLeast(DWORD dwMajor, DWORD dwMinor, WORD dwServicePack)
{
	OSVERSIONINFOEXW l_osver = { sizeof(OSVERSIONINFOEXW), 0 };
	l_osver.dwPlatformId = VER_PLATFORM_WIN32_NT;
	l_osver.dwMajorVersion = dwMajor;
	l_osver.dwMinorVersion = dwMinor;

	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	DWORD dwlInfo = VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION;

	if(dwServicePack != -1)
	{
		l_osver.wServicePackMajor = dwServicePack;

		VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

		dwlInfo |= VER_SERVICEPACKMAJOR;
	}

	return (::VerifyVersionInfoW(&l_osver, dwlInfo, dwlConditionMask) != FALSE);
}


bool CWinUtils::CreateFolderPath(const std::wstring& a_path)
{
	int r = ::SHCreateDirectoryExW(0, a_path.c_str(), NULL);

	return (r == ERROR_SUCCESS || r == ERROR_FILE_EXISTS || r == ERROR_ALREADY_EXISTS);
}


std::wstring CWinUtils::GetExePath()
{
	WCHAR l_buf[1000] = {0};
	WCHAR l_buf2[1000] = {0};

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);

	return l_buf2;
}


std::wstring CWinUtils::GetExeDir()
{
	WCHAR l_buf[1000] = {0};
	WCHAR l_buf2[1000] = {0};

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);
	::PathRemoveFileSpecW(l_buf2);
	::PathRemoveBackslashW(l_buf2);

	return l_buf2;
}
