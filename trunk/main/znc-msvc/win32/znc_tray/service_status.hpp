/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

// instances of this class are NOT safe to call from multiple threads!

class CServiceStatus
{
public:
	// watch this service:
	CServiceStatus(const wchar_t* a_serviceName);
	virtual ~CServiceStatus();

	// call this before doing anything else:
	bool Init();
	// probes:
	bool IsInstalled();
	bool IsRunning();

	// watching functionality:
	bool StartWatchingStatus(HWND a_targetWindow);
	void StopWatchingStatus();

protected:
	const wchar_t* m_serviceName;
	// valid from calling Init to the instance being destroyed:
	SC_HANDLE m_scm;
	// access this after calling OpenService only:
	// it's a read-only (SERVICE_QUERY_STATUS) handle.
	SC_HANDLE m_hService;

	// for watching:
	HWND m_hwndWatch;
	HANDLE m_hWatchThread;
	bool m_doWatch;

	bool OpenService();
	void CloseService();

	// watching implementation internals:
	static unsigned __stdcall WatchThreadProc(void *ptr);
	void WatchWait();
	static void CALLBACK ServiceNotifyCallback(void *ptr);
	void OnWatchEvent(const LPSERVICE_STATUS_PROCESS pss);
};


// Notifications sent to the window registered with CServiceStatus::StartWatchingStatus:

#define WM_SERVICESTOPPED (WM_USER + 101)
#define WM_SERVICESTARTED (WM_USER + 102)
#define WM_SERVICESTARTING (WM_USER + 103)
#define WM_SERVICESTOPPING (WM_USER + 104)
