#include "stdafx.hpp"
#include "znc_service.h"

HANDLE CZNCWindowsService::ms_hEventLog = NULL;


int CZNCWindowsService::Init()
{
	srand((unsigned int)time(NULL));
	CUtils::SetStdoutIsTTY(false);
	_set_fmode(_O_BINARY);
	CRYPTO_malloc_init();

	ms_hEventLog = RegisterEventSource(NULL, "ZNCServiceEventProvider");
	CUtils::HookOutput(&OutputHookProc, NULL);

	if(CZNC::GetCoreVersion() != MODVERSION)
	{
		ReportEvent(ms_hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_DLL_VERSION_MISMATCH, NULL, 0, 0, NULL, NULL);
		return 1;
	}

	if (!InitCsocket())
	{
		ReportEvent(ms_hEventLog, EVENTLOG_ERROR_TYPE, INIT_CATEGORY, MSG_CSOCKET_FAILED, NULL, 0, 0, NULL, NULL);
		return -1;
	}

	CZNC* pZNC = &CZNC::Get();
	pZNC->InitDirs("", "");

	if (!pZNC->ParseConfig(""))
	{
		ReportEvent(ms_hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_CONFIG_CORRUPTED, NULL, 0, 0, NULL, NULL);
		delete pZNC;
		return 1;
	}

	if (!pZNC->OnBoot())
	{
		ReportEvent(ms_hEventLog, EVENTLOG_ERROR_TYPE, CONFIG_CATEGORY, MSG_MODULE_BOOT_ERROR, NULL, 0, 0, NULL, NULL);
		delete pZNC;
		return 1;
	}

	return 0;
}


int CZNCWindowsService::Loop(SERVICE_STATUS *serviceStatus)
{
	CZNC* pZNC = &CZNC::Get();
	int loopResult = -1;

	try
	{
		loopResult = pZNC->ServiceLoop(serviceStatus);
	}
	catch (CException e)
	{
		if(e.GetType() == CException::EX_Shutdown)
		{
			ReportEvent(ms_hEventLog, EVENTLOG_WARNING_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_SHUTDOWN, NULL, 0, 0, NULL, NULL);
			return 0;
		}
		else if(e.GetType() == CException::EX_Restart)
		{
			ReportEvent(ms_hEventLog, EVENTLOG_WARNING_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_RESTART, NULL, 0, 0, NULL, NULL);
			/* :TODO: implement! */
		}
	}

	delete pZNC;

	return loopResult;
}


void CZNCWindowsService::OutputHookProc(int type, const char* text, void *userData)
{
	if(type == 2)
	{
		LPSTR pInsertStrings[1] = { NULL };

		pInsertStrings[0] = const_cast<char*>(text);

		ReportEvent(ms_hEventLog, EVENTLOG_ERROR_TYPE, RUNTIME_CATEGORY, MSG_RUNTIME_ERROR, NULL, 1, 0, (LPCSTR*)pInsertStrings, NULL);
	}
}


void CZNCWindowsService::CleanUp()
{
	if(ms_hEventLog)
	{
		DeregisterEventSource(ms_hEventLog);
		ms_hEventLog = NULL;
	}
}
