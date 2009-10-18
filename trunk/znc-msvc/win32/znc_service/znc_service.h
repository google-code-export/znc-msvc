#pragma once

#include "service_provider.h"

class CZNCWindowsService
{
public:
	static int Init();
	static int Loop(SERVICE_STATUS *serviceStatus);
	static void CleanUp();

protected:
	static HANDLE ms_hEventLog;

	static void OutputHookProc(int type, const char* text, void *userData);
};
