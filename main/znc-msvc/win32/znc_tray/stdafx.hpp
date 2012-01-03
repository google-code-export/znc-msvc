/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

/* SDK = Vista, so we can use newer features where available, but we do ensure to
only statically link XP-compatible things and use runtime versions checks. */
#define WINVER		0x0600
#define _WIN32_WINNT   WINVER
#define _WIN32_WINDOWS WINVER
#define _WIN32_IE      0x0700

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <Windows.h>
#include <Windowsx.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <process.h>

#include <memory>
#include <string>

#include <Commctrl.h>
#pragma comment(lib, "Comctl32.lib")

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define EnableDlgItem(HWND, ITEM, ENABLE) EnableWindow(::GetDlgItem(HWND, ITEM), (ENABLE) ? TRUE : FALSE)


#define HAVE_COM_SERVICE_CONTROL
