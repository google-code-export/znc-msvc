/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

namespace ZNCTray
{


class CTrayIcon
{
public:
	CTrayIcon(HWND a_hwndMainWin, const wchar_t* a_szAppTitle);
	virtual ~CTrayIcon();

	bool Add(char a_color);
	void OnWmTaskbarCreated();
	void OnWmTrayIconEvent(WPARAM wParam, LPARAM lParam);
	bool UpdateColor(char a_color);
	bool Remove();
protected:
	HWND m_hwndMainWin;
	const wchar_t* m_szAppTitle;
	char m_iconColor;
	HICON m_hIcon;
	bool m_addedToTna;

	void _InitNotifyIconData(PNOTIFYICONDATA nid);
	void LoadIconForColor(char a_color);

	void ShowContextMenu(LPPOINT pt);
};


}; // end of namespace
