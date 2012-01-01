/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "service_status.hpp"

// for QueryServiceStatus & friends:
#pragma comment(lib, "Advapi32.lib")


CServiceStatus::CServiceStatus(const wchar_t* a_serviceName) :
	m_serviceName(a_serviceName),
	m_scm(NULL), m_hService(NULL),
	m_hwndWatch(NULL), m_hWatchThread(NULL), m_doWatch(false)
{
}


bool CServiceStatus::Init()
{
	if(m_scm)
	{
		// already initialized
		return true;
	}

	m_scm = ::OpenSCManager(NULL, NULL, STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE);

	return (!!m_scm);
}


bool CServiceStatus::IsInstalled()
{
	if(this->OpenService())
	{
		// service can be opened, so it must be installed

		this->CloseService();

		return true;
	}

	return false;
}


bool CServiceStatus::IsRunning()
{
	if(this->OpenService())
	{
		SERVICE_STATUS ss = {0};

		if(::QueryServiceStatus(m_hService, &ss))
		{
			return (ss.dwCurrentState == SERVICE_RUNNING ||
				ss.dwCurrentState == SERVICE_START_PENDING);
		}

		// we are lazy and don't CloseService here,
		// destructor or the next call will take care of it
	}

	return false;
}


bool CServiceStatus::OpenService()
{
	if(!m_scm)
	{
		return false;
	}

	if(m_hService)
	{
		// already open
		return true;
	}

	// read-only handle!
	m_hService = ::OpenService(m_scm, m_serviceName, SERVICE_QUERY_STATUS);

	return (m_hService != NULL);
}


void CServiceStatus::CloseService()
{
	if(m_hService)
	{
		::CloseServiceHandle(m_hService);
		m_hService = NULL;
	}
}


bool CServiceStatus::StartWatchingStatus(HWND a_targetWindow)
{
	if(m_hWatchThread != NULL)
	{
		// already watching
		return false;
	}

	m_hwndWatch = a_targetWindow;
	m_doWatch = true;

	m_hWatchThread = (HANDLE)_beginthreadex(NULL, 0, &WatchThreadProc, static_cast<void*>(this), 0, NULL);

	return (!!m_hWatchThread);
}


unsigned __stdcall CServiceStatus::WatchThreadProc(void *ptr)
{
	// get instance:
	CServiceStatus *instance = static_cast<CServiceStatus*>(ptr);

	if(!instance)
		return -1;

	instance->WatchWait();

	return 0;
}


void CServiceStatus::WatchWait()
{
	// get a separate service handle for this thread:
	SC_HANDLE l_service = ::OpenService(m_scm, m_serviceName, SERVICE_QUERY_STATUS);

	// check for OS support:
	OSVERSIONINFOEXW l_osver = { sizeof(OSVERSIONINFOEXW), 0 };
	l_osver.dwMajorVersion = 6;

	DWORDLONG dwlVerCond = 0;
	VER_SET_CONDITION(dwlVerCond, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlVerCond, VER_MINORVERSION, VER_GREATER_EQUAL);

	bool bFallback = false;

	if(::VerifyVersionInfo(&l_osver, VER_MAJORVERSION | VER_MINORVERSION, dwlVerCond))
	{
		// the good version: NotifyServiceStatusChange
		typedef DWORD (WINAPI *tfnssc)(SC_HANDLE hService, DWORD dwNotifyMask, PSERVICE_NOTIFY pNotifyBuffer);
		tfnssc nssc = (tfnssc)::GetProcAddress(::GetModuleHandle(L"Advapi32.dll"), "NotifyServiceStatusChange");

		if(!nssc)
		{
			bFallback = true;
		}
		else
		{
			SERVICE_NOTIFY serviceNotify = { 0 };
			serviceNotify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
			serviceNotify.pfnNotifyCallback = &ServiceNotifyCallback;
			serviceNotify.pContext = static_cast<void*>(this);

			while(m_doWatch)
			{
				if(nssc(l_service, SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_START_PENDING |
					SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_STOP_PENDING, &serviceNotify) == ERROR_SUCCESS)
				{
					::SleepEx(INFINITE, TRUE); // Wait for the notification
				}
				else
					break;
			}
			// exit proc
		}
	}
	else
	{
		// legacy OS (pre-Vista):
		bFallback = true;
	}

	if(bFallback)
	{
		SERVICE_STATUS_PROCESS ssp = {0};
		DWORD dwBytesNeeded = 0;
		DWORD dwState = 0;

		do
		{
			if(!::QueryServiceStatusEx(l_service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
				break;

			if(ssp.dwCurrentState != dwState)
			{
				this->OnWatchEvent(&ssp);

				dwState = ssp.dwCurrentState;
			}

			::SleepEx(1000, TRUE); // probe every second
		}
		while(m_doWatch);
	}

	::CloseServiceHandle(l_service);
}


void CALLBACK CServiceStatus::ServiceNotifyCallback(void *ptr)
{
	SERVICE_NOTIFY* serviceNotify = static_cast<SERVICE_NOTIFY*>(ptr);
	CServiceStatus *instance = static_cast<CServiceStatus*>(serviceNotify->pContext);

	if(instance && serviceNotify->dwNotificationStatus == ERROR_SUCCESS)
	{
		instance->OnWatchEvent(&serviceNotify->ServiceStatus);
	}
}


void CServiceStatus::OnWatchEvent(const LPSERVICE_STATUS_PROCESS pssp)
{
	// notify handler window:
	switch(pssp->dwCurrentState)
	{
	case SERVICE_RUNNING:
		::PostMessage(m_hwndWatch, WM_SERVICESTARTED, 0, 0);
		break;
	case SERVICE_START_PENDING:
		::PostMessage(m_hwndWatch, WM_SERVICESTARTING, 0, 0);
		break;
	case SERVICE_STOP_PENDING:
		::PostMessage(m_hwndWatch, WM_SERVICESTOPPING, 0, 0);
		break;
	case SERVICE_STOPPED:
		::PostMessage(m_hwndWatch, WM_SERVICESTOPPED, 0, 0);
		break;
	}
}


static VOID CALLBACK APCDummyProc(ULONG_PTR) {}

void CServiceStatus::StopWatchingStatus()
{
	if(m_doWatch && m_hWatchThread)
	{
		m_doWatch = false;

		// post APC shit to wake up SleepEx:
		::QueueUserAPC(&APCDummyProc, m_hWatchThread, NULL);

		// wait till the thread has terminated:
		::WaitForSingleObject(m_hWatchThread, INFINITE);
		// ... then close the handle:
		::CloseHandle(m_hWatchThread);
		m_hWatchThread = NULL;
	}
}


CServiceStatus::~CServiceStatus()
{
	this->StopWatchingStatus();

	this->CloseService();

	if(m_scm)
	{
		::CloseServiceHandle(m_scm);
	}
}
