/*
 * Copyright (C) 2012 Ingmar Runge
 * Copyright (C) 2010 cxxjoe
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc_tray.hpp"

// Credit for most of these utility functions:
// cxxjoe @ http://code.google.com/p/infekt/source/browse/trunk/src/lib/util.cpp
// GPLv2 2011


bool CUtil::RemoveCwdFromDllSearchPath()
{
	// available from XP SP1:
	if(CUtil::WinVerAtLeast(5, 1, 1))
	{
		typedef BOOL (WINAPI *fsdd)(LPCTSTR);

		// remove the current directory from the DLL search path by calling SetDllDirectory:
		fsdd l_fsdd = (fsdd)GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetDllDirectory");
		if(l_fsdd)
		{
			return (l_fsdd(L"") != FALSE);
		}
	}

	return false;
}


bool CUtil::HardenHeap()
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

bool CUtil::EnforceDEP()
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


bool CUtil::WinVerAtLeast(DWORD dwMajor, DWORD dwMinor, WORD dwServicePack)
{
	OSVERSIONINFOEX l_osver = { sizeof(OSVERSIONINFOEX), 0 };
	l_osver.dwMajorVersion = dwMajor;
	l_osver.dwMinorVersion = dwMinor;

	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	if(dwServicePack != -1)
	{
		l_osver.wServicePackMajor = dwServicePack;

		VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	}

	return (::VerifyVersionInfo(&l_osver, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE);
}


CResourceBitmap::CResourceBitmap(int resId) :
	m_hGlobal(NULL)
{
	HRSRC hRes = ::FindResource(g_hInstance, MAKEINTRESOURCE(resId), L"PNG");

	if(!hRes)
	{
		return;
	}

	DWORD dwSize = ::SizeofResource(g_hInstance, hRes);
	const void *pResData = ::LockResource(
		::LoadResource(g_hInstance, hRes));

	if(dwSize < 1 || !pResData)
	{
		return;
	}

	HGLOBAL hGlob = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);

	if(hGlob)
	{
		void *pGlobBuf = ::GlobalLock(hGlob);

		if(pGlobBuf)
		{
			memcpy_s(pGlobBuf, dwSize, pResData, dwSize);

			IStream *pStream;
			if(SUCCEEDED(::CreateStreamOnHGlobal(hGlob, FALSE, &pStream)))
			{
				Gdiplus::Bitmap* gdipBmp = Gdiplus::Bitmap::FromStream(pStream);

				// done its duty here, Bitmap probably keeps a ref:
				pStream->Release();

				if(gdipBmp && gdipBmp->GetLastStatus() == Gdiplus::Ok)
				{
					// it worked!
					m_hGlobal = hGlob;
					m_bmp = std::shared_ptr<Gdiplus::Bitmap>(gdipBmp);

					return; // postpone cleanup to destructor! ;)
				}

				delete gdipBmp;
			}

			::GlobalUnlock(pGlobBuf);
		}

		::GlobalFree(hGlob);
	}
}

CResourceBitmap::~CResourceBitmap()
{
	m_bmp.reset();

	/*if(m_hGlobal)
	{
		::GlobalUnlock(m_hGlobal);
		::GlobalFree(m_hGlobal);
	}*/
}
