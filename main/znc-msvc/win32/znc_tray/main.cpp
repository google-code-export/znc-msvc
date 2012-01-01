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

	// init GDI+:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

	INITCOMMONCONTROLSEX icce = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	::InitCommonControlsEx(&icce);

	// run main app:
	ZNCTray::CControlWindow *l_mainWin = new ZNCTray::CControlWindow();

	int ret = l_mainWin->Run();

	// make sure this gets destroyed before the following shutdown code is invoked:
	delete l_mainWin;

	// shut down GDI+:
	Gdiplus::GdiplusShutdown(s_gdiplusToken);

	return ret;
}


// global var, extern'd in znc_tray.hpp
HINSTANCE g_hInstance;
