#ifndef _SKYPELINK_H
#define _SKYPELINK_H

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#include <Rpc.h>
#include <process.h>
#endif // _WIN32

typedef unsigned int uint32_t;

typedef int (*SkypeMessageEventCallback)(ULONG_PTR a_reserved, const char *a_str_utf8, void *a_userData);

class SkypeMessaging
{
private:
	static SkypeMessaging *ms_theInstance;
	SkypeMessaging();
	~SkypeMessaging();
protected:
#ifdef _WIN32
	uint32_t m_winMsgID_SkypeControlAPIAttach;
	uint32_t m_winMsgID_SkypeControlAPIDiscover;
	TCHAR *m_winClassName;
	HINSTANCE m_hInstance;
	HWND m_hWindow;
	HWND m_hSkypeWin;
	HANDLE m_threadWaitEvent;
	SkypeMessageEventCallback m_eventCallback;
	void *m_eventCallbackUserData;
	
	bool Init_WindowClass();
	void UnInit_WindowClass();
	bool Init_MainWindow();
	void UnInit_MainWindow();

	static void __cdecl WindowMessageLoop(void *arg);
	static LRESULT APIENTRY WindowProc(HWND, UINT, WPARAM, LPARAM);
#endif
	bool m_initialized;
public:
	static SkypeMessaging *GetInstance();
	static void ClearInstance();

	bool Initialize(SkypeMessageEventCallback a_eventCallback, void *a_callbackUserData);
	bool UnInitialize();

	bool SendConnect();
	bool IsConnected();
	bool SendCommand(const char *a_command);
	
	void Disconnect();
};


enum {
	SKYPECONTROLAPI_ATTACH_SUCCESS = 0,				// Client is successfully attached and API window handle can be found in wParam parameter
	SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION,	// Skype has acknowledged connection request and is waiting for confirmation from the user.
													// The client is not yet attached and should wait for SKYPECONTROLAPI_ATTACH_SUCCESS message
	SKYPECONTROLAPI_ATTACH_REFUSED,					// User has explicitly denied access to client
	SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE,			// API is not available at the moment. For example, this happens when no user is currently logged in.
													// Client should wait for SKYPECONTROLAPI_ATTACH_API_AVAILABLE broadcast before making any further
													// connection attempts.
	SKYPECONTROLAPI_ATTACH_API_AVAILABLE=0x8001
};

#endif // _SKYPELINK_H