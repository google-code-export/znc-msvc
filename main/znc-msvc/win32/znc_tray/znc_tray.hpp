/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

// keep HINSTANCE around
extern HINSTANCE g_hInstance;
// obtained through RegisterWindowMessage:
extern UINT WM_TASKBARCREATED;

#include "util.h"

class CResourceBitmap
{
public:
	CResourceBitmap(int resId);
	virtual ~CResourceBitmap();

	std::shared_ptr<Gdiplus::Bitmap> GetBmp() const { return m_bmp; }
	bool Loaded() const { return !!m_bmp; }
protected:
	std::shared_ptr<Gdiplus::Bitmap> m_bmp;
	HGLOBAL m_hGlobal;
};

#define WM_TRAYICONEVENT (WM_USER + 9)

#define WMC_CLOSE_TRAY (WM_APP + 1)
#define WMC_SHOW_CONTROL_WIN (WM_APP + 2)
