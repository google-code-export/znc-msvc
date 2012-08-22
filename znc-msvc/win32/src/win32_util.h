/*
 * Copyright (C) 2012 Ingmar Runge
 * Copyright (C) 2010 cxxjoe
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

#include <Windows.h>
#include <string>

class CWinUtils
{
public:
	static bool RemoveCwdFromDllSearchPath();
	static bool HardenHeap();
	static bool EnforceDEP();
	static bool WinVerAtLeast(DWORD dwMajor, DWORD dwMinor, WORD dwServicePack = -1);
	static bool CreateFolderPath(const std::wstring& a_path);
	static std::wstring GetExePath();
	static std::wstring GetExeDir();
};
