#include "stdafx.hpp"
#include "znc_service.h"


CZNCWindowsService *CZNCWindowsService::thisSvc = 0;

CZNCWindowsService::CZNCWindowsService() :
	bMainLoop(true), sDataDir("")
{
	thisSvc = this;

	memset(&serviceStatus, 0, sizeof(serviceStatus));
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
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
	srand((unsigned int)time(NULL));
	_set_fmode(_O_BINARY);
#ifdef HAVE_LIBSSL
	CRYPTO_malloc_init();
#endif

	hEventLog = RegisterEventSource(NULL, ZNC_EVENT_PROVIDER);
	if (hEventLog == NULL)
		return ERROR_EXITCODE;

	CUtils::HookOutput(&OutputHookProc, NULL);

	if(CZNC::GetCoreVersion() != MODVERSION)
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_DLL_VERSION_MISMATCH, NULL, 0, 0, NULL, NULL);
		return ERROR_EXITCODE;
	}

	if (!InitCsocket())
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_CSOCKET_FAILED, NULL, 0, 0, NULL, NULL);
		return ERROR_EXITCODE;
	}

	CZNC* pZNC = &CZNC::Get();
	// pZNC->InitDirs(((argc) ? argv[0] : ""), sDataDir);
	pZNC->InitDirs("", thisSvc->sDataDir);

	// if (!pZNC->ParseConfig(sConfig))
	if (!pZNC->ParseConfig(""))
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_CONFIG_CORRUPTED, NULL, 0, 0, NULL, NULL);
		delete pZNC;
		return ERROR_EXITCODE;
	}

	if (!pZNC->OnBoot())
	{
		ReportEvent(hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_MODULE_BOOT_ERROR, NULL, 0, 0, NULL, NULL);
		delete pZNC;
		return ERROR_EXITCODE;
	}

	return 0;
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

	delete pZNC;
	return dwRet;
}

VOID CZNCWindowsService::ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	serviceStatus.dwCurrentState = dwCurrentState;
	serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
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
VOID WINAPI CZNCWindowsService::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	thisSvc->hServiceStatus = RegisterServiceCtrlHandler(ZNC_SERVICE_NAME, ControlHandler);
	if (thisSvc->hServiceStatus == NULL)
	{
		return;
	}

	thisSvc->ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	DWORD Result = thisSvc->Init();
	if (Result != 0)
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

void CZNCWindowsService::OutputHookProc(int type, const char* text, void *userData)
{
	if(type == 2)
	{
		LPSTR pInsertStrings[1] = { NULL };
		pInsertStrings[0] = const_cast<char*>(text);
		ReportEvent(thisSvc->hEventLog, EVENTLOG_ERROR_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_ERROR, NULL, 1, 0, (LPCSTR*)pInsertStrings, NULL);
	}
}