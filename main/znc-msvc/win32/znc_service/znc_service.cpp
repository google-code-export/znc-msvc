/*
 * Copyright (C) 2009 laszlo.tamas.szabo
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "znc_service.h"
#include "registry.h"

#define ERROR_EXITCODE -1


CZNCWindowsService *CZNCWindowsService::thisSvc = 0;

CZNCWindowsService::CZNCWindowsService() :
	bMainLoop(true), sDataDir("")
{
	thisSvc = this;

	memset(&serviceStatus, 0, sizeof(serviceStatus));
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
}


void CZNCWindowsService::RedirectStdStreams()
{
	std::wstring l_logPath;
	wchar_t l_pathBuf[MAX_PATH + 1] = {0};

	if(SUCCEEDED(::SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, l_pathBuf)))
	{
		std::wstringstream l_fnBuf;
		l_fnBuf << L"\\ZNC\\Service.";
		l_fnBuf << ::GetCurrentProcessId();
		l_fnBuf << L".log";

		l_logPath = l_pathBuf + l_fnBuf.str();
		
		FILE *l_pFile = _wfsopen(l_logPath.c_str(), L"w+", _SH_DENYWR);
		if(l_pFile)
		{
			setvbuf(l_pFile, NULL, _IONBF, 0);
			
			*stderr = *l_pFile;
			*stdout = *l_pFile;
		}
	}
}


CZNCWindowsService::~CZNCWindowsService()
{
	if(hEventLog)
	{
		DeregisterEventSource(hEventLog);
		hEventLog = NULL;
	}
}


DWORD CZNCWindowsService::Init()
{
	// redirect stdout/stderr before calling into ZNC land:
	this->RedirectStdStreams();

	CUtils::SeedPRNG();

	_set_fmode(_O_BINARY);
	CRYPTO_malloc_init();

	hEventLog = RegisterEventSourceW(NULL, ZNC_EVENT_PROVIDER);
	if (hEventLog == NULL)
		return ERROR_EXITCODE;

	if(CZNC::GetCoreVersion() != VERSION)
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_DLL_VERSION_MISMATCH, NULL, 0, 0, NULL, NULL);
		return ERROR_EXITCODE;
	}

	if (!InitCsocket())
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_CSOCKET_FAILED, NULL, 0, 0, NULL, NULL);
		return ERROR_EXITCODE;
	}

	if (thisSvc->sDataDir.empty())
	{
		CRegistryKey l_reg(HKEY_LOCAL_MACHINE);

		// get data dir from registry!

		if (l_reg.OpenForReading(L"SOFTWARE\\ZNC"))
		{
			const std::wstring l_pathW = l_reg.ReadString(L"ServiceDataDir", L"");

			if(!l_pathW.empty())
			{
				int l_bufLen = ::WideCharToMultiByte(CP_OEMCP, 0, l_pathW.c_str(), -1, NULL, NULL, NULL, NULL);

				if(l_bufLen > 0)
				{
					char *l_buf = new char[l_bufLen + 1];

					if(l_buf && ::WideCharToMultiByte(CP_OEMCP, 0, l_pathW.c_str(), -1, l_buf, l_bufLen, NULL, NULL))
					{
						thisSvc->sDataDir = l_buf;
					}

					delete[] l_buf;
				}
			}
		}
	}

	CZNC* pZNC = &CZNC::Get();
	pZNC->InitDirs("", thisSvc->sDataDir);

	CString sError;

	if(!pZNC->ParseConfig("", sError))
	{
		LPSTR pInsertStrings[2] = { NULL };
		pInsertStrings[0] = const_cast<char*>(thisSvc->sDataDir.c_str());
		pInsertStrings[1] = const_cast<char*>(sError.c_str());
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_CONFIG_CORRUPTED, NULL, 2, 0, (LPCSTR*)pInsertStrings, NULL);
		CZNC::_Reset();
		return ERROR_EXITCODE;
	}

	if(!pZNC->OnBoot())
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_MODULE_BOOT_ERROR, NULL, 0, 0, NULL, NULL);
		CZNC::_Reset();
		return ERROR_EXITCODE;
	}

	return NO_ERROR;
}


DWORD CZNCWindowsService::Loop()
{
	CZNC* pZNC = &CZNC::Get();
	DWORD dwRet = 0;

	try
	{
		pZNC->Loop(&bMainLoop);
	}
	catch (CException e)
	{
		/// :TODO: implement! ///
		if(e.GetType() == CException::EX_Shutdown)
		{
			ReportEvent(hEventLog, EVENTLOG_WARNING_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_SHUTDOWN, NULL, 0, 0, NULL, NULL);
			dwRet = 0;
		}
		else if(e.GetType() == CException::EX_Restart)
		{
			ReportEvent(hEventLog, EVENTLOG_WARNING_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_RESTART, NULL, 0, 0, NULL, NULL);
			dwRet = ERROR_EXITCODE;
		}
	}

	CZNC::_Reset();
	return dwRet;
}


VOID CZNCWindowsService::ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	serviceStatus.dwCurrentState = dwCurrentState;
	if(dwWin32ExitCode != NO_ERROR)
	{
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		serviceStatus.dwServiceSpecificExitCode = dwWin32ExitCode;
	}
	serviceStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		serviceStatus.dwControlsAccepted = 0;
	else
		serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
		dwCheckPoint = 0;
	else
		dwCheckPoint++;
	serviceStatus.dwCheckPoint = dwCheckPoint;

	SetServiceStatus(hServiceStatus, &serviceStatus);
}


// ----------------
// static callbacks
// ----------------
VOID WINAPI CZNCWindowsService::ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
	thisSvc->hServiceStatus = RegisterServiceCtrlHandlerW(ZNC_SERVICE_NAME, ControlHandler);
	if (thisSvc->hServiceStatus == NULL)
	{
		return;
	}

	thisSvc->ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	DWORD Result = thisSvc->Init();
	if (Result != NO_ERROR)
	{
		thisSvc->ReportServiceStatus(SERVICE_STOPPED, Result, 0);
		return;
	}

	thisSvc->ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);
	Result = thisSvc->Loop();
	thisSvc->ReportServiceStatus(SERVICE_STOPPED, Result, 0);
}


VOID WINAPI CZNCWindowsService::ControlHandler(DWORD dwControl)
{
	switch (dwControl)
	{
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			thisSvc->ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);
			thisSvc->bMainLoop = false;
			break;
		default:
			thisSvc->ReportServiceStatus(thisSvc->serviceStatus.dwCurrentState, NO_ERROR, 0);
			break;
	}
	return;
}

/*void CZNCWindowsService::OutputHookProc(int type, const char* text, void *userData)
{
	if(type == 2)
	{
		LPSTR pInsertStrings[1] = { NULL };
		pInsertStrings[0] = const_cast<char*>(text);
		ReportEvent(thisSvc->hEventLog, EVENTLOG_ERROR_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_ERROR, NULL, 1, 0, (LPCSTR*)pInsertStrings, NULL);
	}
}*/


#pragma comment(lib, "Shlwapi.lib") // :TODO: move me

static std::wstring GetExePath() // :TODO: move me
{
	WCHAR l_buf[1000] = {0};
	WCHAR l_buf2[1000] = {0};

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);

	return l_buf2;
}

static std::wstring GetExeDir() // :TODO: move me
{
	WCHAR l_buf[1000] = {0};
	WCHAR l_buf2[1000] = {0};

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);
	::PathRemoveFileSpecW(l_buf2);
	::PathRemoveBackslashW(l_buf2);

	return l_buf2;
}


DWORD CZNCWindowsService::InstallService(bool a_startTypeManual)
{
	SC_HANDLE l_hscm = ::OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW,
		SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

	if(!l_hscm)
	{
		// most probably ERROR_ACCESS_DENIED.
		return ::GetLastError();
	}

	DWORD l_result = 0;
	
	const std::wstring l_exePath = L"\"" + GetExePath() + L"\"";

	SC_HANDLE l_hsvc = ::CreateServiceW(l_hscm, ZNC_SERVICE_NAME, ZNC_SERVICE_NAME,
		SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_STOP,
		SERVICE_WIN32_OWN_PROCESS, (a_startTypeManual ? SERVICE_DEMAND_START : SERVICE_AUTO_START), SERVICE_ERROR_NORMAL,
		l_exePath.c_str(), NULL, NULL, NULL, L"NT AUTHORITY\\LocalService", NULL);

	if(!l_hsvc)
	{
		l_result = ::GetLastError();
	}
	else
	{
		if(!a_startTypeManual)
		{
			// change start type to "auto (delayed)" on supported OSes:

			OSVERSIONINFOEXW l_osver = { sizeof(OSVERSIONINFOEXW), 0 };
			l_osver.dwMajorVersion = 6;

			DWORDLONG dwlVerCond = 0;
			VER_SET_CONDITION(dwlVerCond, VER_MAJORVERSION, VER_GREATER_EQUAL);
			VER_SET_CONDITION(dwlVerCond, VER_MINORVERSION, VER_GREATER_EQUAL);

			if(::VerifyVersionInfoW(&l_osver, VER_MAJORVERSION | VER_MINORVERSION, dwlVerCond))
			{
				SERVICE_DELAYED_AUTO_START_INFO l_sdasi = { 0 };
				l_sdasi.fDelayedAutostart = TRUE;
				::ChangeServiceConfig2(l_hsvc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &l_sdasi);
			}
		}

		// update description:
		SERVICE_DESCRIPTIONW l_sd = { 0 };
		l_sd.lpDescription = ZNC_SERVICE_DESCRIPTION;
		::ChangeServiceConfig2W(l_hsvc, SERVICE_CONFIG_DESCRIPTION, &l_sd);

		::CloseServiceHandle(l_hsvc);

		// register event provider:
		CRegistryKey l_evlKey(HKEY_LOCAL_MACHINE);

		if(l_evlKey.OpenForWriting(L"SYSTEM\\CurrentControlSet\\services\\eventlog\\Application\\" ZNC_EVENT_PROVIDER))
		{
			const std::wstring l_eventProviderDllPath = GetExeDir() + L"\\service_provider.dll";

			l_evlKey.WriteDword(L"CategoryCount", 3);
			l_evlKey.WriteString(L"CategoryMessageFile", l_eventProviderDllPath);
			l_evlKey.WriteString(L"EventMessageFile", l_eventProviderDllPath);
			l_evlKey.WriteString(L"ParameterMessageFile", l_eventProviderDllPath);
			l_evlKey.WriteDword(L"TypesSupported", 0x07);
		}
	}

	::CloseServiceHandle(l_hscm);

	return l_result;
}


DWORD CZNCWindowsService::UninstallService()
{
	SC_HANDLE l_hscm = ::OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_ALL_ACCESS);

	if(!l_hscm)
	{
		// most probably ERROR_ACCESS_DENIED.
		return ::GetLastError();
	}

	DWORD l_result = 0;

	SC_HANDLE l_hsvc = ::OpenServiceW(l_hscm, ZNC_SERVICE_NAME, SERVICE_ALL_ACCESS);

	if(!l_hsvc)
	{
		l_result = ::GetLastError();
	}
	else
	{
		if(!::DeleteService(l_hsvc))
		{
			l_result = ::GetLastError();
		}

		::CloseServiceHandle(l_hsvc);

		// delete event provider information:
		CRegistryKey::DeleteKey(L"SYSTEM\\CurrentControlSet\\services\\eventlog\\Application", ZNC_EVENT_PROVIDER);
	}

	::CloseServiceHandle(l_hscm);

	return l_result;
}
