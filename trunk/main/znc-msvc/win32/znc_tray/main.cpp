/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc_tray.hpp"
#include "control_window.hpp"

static ULONG_PTR s_gdiplusToken;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR wszCommandLine, int nShowCmd)
{
	g_hInstance = hInstance;

	// enforce some things:
	CUtil::EnforceDEP();
	CUtil::HardenHeap();
	CUtil::RemoveCwdFromDllSearchPath();

	// init COM:
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	
	// init GDI+:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

	// init CC:
	INITCOMMONCONTROLSEX icce = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	::InitCommonControlsEx(&icce);

	// init winsock (for communication with service helper module):
	WSADATA wsaData;
	int iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	// srand:
	int seed = ::GetTickCount();
	seed ^= getpid();
	seed ^= rand();
	::srand(seed);

	// run main app:
	ZNCTray::CControlWindow *l_mainWin = new ZNCTray::CControlWindow();

	int ret = l_mainWin->Run();

	// make sure this gets destroyed before the following shutdown code is invoked:
	delete l_mainWin;

	// shut down winsock:
	::WSACleanup();

	// shut down GDI+:
	Gdiplus::GdiplusShutdown(s_gdiplusToken);

	// shut down COM:
	::CoUninitialize();

	return ret;
}


// enable "visual styles":
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// global var, extern'd in znc_tray.hpp
HINSTANCE g_hInstance;
