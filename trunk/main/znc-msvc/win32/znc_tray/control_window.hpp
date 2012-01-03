/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

#include "tray_icon.hpp"
#include "service_status.hpp"

namespace ZNCTray
{

typedef enum _service_status
{
	ZS_UNKNOWN = 0,
	ZS_NOT_INSTALLED,
	ZS_STANDALONE,
	ZS_STARTING,
	ZS_RUNNING,
	ZS_STOPPING,
	ZS_STOPPED
} EServiceStatus;


class CControlWindow
{
public:
	CControlWindow();
	virtual ~CControlWindow();

	int Run();
	void Show();
	void Hide();

protected:
	HWND m_hwndDlg;
	std::shared_ptr<CTrayIcon> m_trayIcon;

	HICON m_hIconSmall, m_hIconBig;
	std::shared_ptr<CResourceBitmap> m_bmpLogo;

	HWND m_hwndStatusBar;

	std::shared_ptr<CServiceStatus> m_serviceStatus;
	EServiceStatus m_statusFlag;

	bool CreateDlg();
	int MessageLoop();
	static INT_PTR CALLBACK DialogProcStatic(HWND, UINT, WPARAM, LPARAM);
	bool DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool OnWmCommand(UINT uCmd);
	void OnPaint();

	void InitialSetup();
	void OnBeforeDestroy();

	void DetectServiceStatus();
	void UpdateUIWithServiceStatus();
};


}; // end of namespace
