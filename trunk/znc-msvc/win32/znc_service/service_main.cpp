#include "stdafx.hpp"
#include "znc_service.h"

#define ZNC_SERVICE_NAME "ZNCIRCBouncer"

SERVICE_STATUS g_serviceStatus;
SERVICE_STATUS_HANDLE g_hStatus;

void WINAPI ControlHandler(DWORD dwControl);


void ServiceMain(int argc, char** argv)
{
	memset(&g_serviceStatus, 0, sizeof(SERVICE_STATUS));

	g_serviceStatus.dwServiceType = SERVICE_WIN32;
	g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	g_hStatus = RegisterServiceCtrlHandler(ZNC_SERVICE_NAME, (LPHANDLER_FUNCTION)ControlHandler);
	if(g_hStatus == (SERVICE_STATUS_HANDLE)0)
	{
		return;
	}

	int initResult = CZNCWindowsService::Init();

	if(initResult != 0)
	{
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		g_serviceStatus.dwWin32ExitCode = -1;
		SetServiceStatus(g_hStatus, &g_serviceStatus);
		return;
	}

	g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_hStatus, &g_serviceStatus);

	/* run the main loop */
	int loopResult = CZNCWindowsService::Loop(&g_serviceStatus);

	g_serviceStatus.dwWin32ExitCode = loopResult;
	g_serviceStatus.dwCurrentState = SERVICE_STOPPED;

	SetServiceStatus(g_hStatus, &g_serviceStatus);

	return;
}


void WINAPI ControlHandler(DWORD dwControl)
{
	switch(dwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		g_serviceStatus.dwWin32ExitCode = 0;
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
	}

	SetServiceStatus(g_hStatus, &g_serviceStatus);

	return;
}


void main()
{
	SERVICE_TABLE_ENTRY ServiceTable[2];

	ServiceTable[0].lpServiceName = ZNC_SERVICE_NAME;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	StartServiceCtrlDispatcher(ServiceTable);
}

