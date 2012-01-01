/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

#define WM_SERVICESTOPPED (WM_USER + 101)
#define WM_SERVICESTARTED (WM_USER + 102)
#define WM_SERVICESTARTING (WM_USER + 103)
#define WM_SERVICESTOPPING (WM_USER + 104)

class CServiceStatus
{
public:
	CServiceStatus(const wchar_t* a_serviceName);
	virtual ~CServiceStatus();

	bool Init();
	bool IsInstalled();
	bool IsRunning();

	bool StartWatchingStatus(HWND a_targetWindow);
	void StopWatchingStatus();
protected:
	const wchar_t* m_serviceName;
	SC_HANDLE m_scm;
	SC_HANDLE m_hService;

	HWND m_hwndWatch;
	HANDLE m_hWatchThread;
	bool m_doWatch;

	bool OpenService();
	void CloseService();

	static unsigned __stdcall WatchThreadProc(void *ptr);
	void WatchWait();
	static void CALLBACK ServiceNotifyCallback(void *ptr);
	void OnWatchEvent(const LPSERVICE_STATUS_PROCESS pss);
};
