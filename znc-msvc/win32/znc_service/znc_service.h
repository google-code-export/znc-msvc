#ifndef _ZNC_SERVICE_H_
#define _ZNC_SERVICE_H_

#include <windows.h>
#include "service_provider.h"

#include "znc_service_defs.h"

class CString;

class CZNCWindowsService
{
public:
	CZNCWindowsService();
	~CZNCWindowsService();
	static VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
	static VOID WINAPI ControlHandler(DWORD dwControl);
	void SetDataDir(char *dataDir) { sDataDir = CString(dataDir); };

	static DWORD InstallService(bool a_startTypeManual);
	static DWORD UninstallService();
protected:	
	DWORD Init();
	DWORD Loop();
	VOID ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

	void RedirectStdStreams();

	SERVICE_STATUS serviceStatus;
	SERVICE_STATUS_HANDLE hServiceStatus;
	// checkpoint value, incremented periodically if SERVICE_*_PENDING state
	// is sent to the SCM with SetServiceStatus
	DWORD dwCheckPoint;
	HANDLE hEventLog;
	// for exiting the ZNC loop when SERVICE_CONTROL_STOP is received
	bool bMainLoop;
	CString sDataDir;

	static CZNCWindowsService *thisSvc;
};

#endif // _ZNC_SERVICE_H_