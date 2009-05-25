#include "SkypeLink.hpp"

#ifdef _WIN32

#include <stdio.h>

SkypeMessaging *SkypeMessaging::ms_theInstance = NULL;


/**************************************************************************************/
void HandleLastError(const TCHAR *msg = L"Error occured") {
        DWORD errCode = GetLastError();
        TCHAR *err;
        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           errCode,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
                           (LPTSTR) &err,
                           0,
                           NULL))
            return;

        //TRACE("ERROR: %s: %s", msg, err);
        static TCHAR buffer[4024];
        _snwprintf(buffer, sizeof(buffer), L"ERROR: %s: %s\n", msg, err);
        OutputDebugString(buffer);
        LocalFree(err);
}
/**************************************************************************************/

SkypeMessaging *SkypeMessaging::GetInstance()
{
	if(!ms_theInstance)
	{
		ms_theInstance = new SkypeMessaging();
	}

	return ms_theInstance;
}


void SkypeMessaging::ClearInstance()
{
	if(ms_theInstance)
	{
		delete ms_theInstance;
		ms_theInstance = NULL;
	}
}


SkypeMessaging::SkypeMessaging()
{
	m_winMsgID_SkypeControlAPIAttach = m_winMsgID_SkypeControlAPIDiscover = 0;
	m_initialized = false;
	m_winClassName = NULL;
	m_hInstance = 0;
	m_hWindow = m_hSkypeWin = 0;
	m_threadWaitEvent = NULL;
	m_eventCallback = NULL;
	m_eventCallbackUserData = NULL;
}


SkypeMessaging::~SkypeMessaging()
{
	if(m_hWindow)
	{
		m_threadWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		PostMessage(m_hWindow, WM_CLOSE, 0, 0);

		// wait for the thread to exit...
		WaitForSingleObject(m_threadWaitEvent, INFINITE);
	}

	UnInitialize();
}


bool SkypeMessaging::Initialize(SkypeMessageEventCallback a_eventCallback, void *a_callbackUserData)
{
	if(m_initialized) return false;

	m_eventCallback = a_eventCallback;
	m_eventCallbackUserData = a_callbackUserData;
	
	m_winMsgID_SkypeControlAPIAttach = RegisterWindowMessage(_T("SkypeControlAPIAttach"));
	m_winMsgID_SkypeControlAPIDiscover = RegisterWindowMessage(_T("SkypeControlAPIDiscover"));

	if(!m_winMsgID_SkypeControlAPIAttach || !m_winMsgID_SkypeControlAPIDiscover)
		return false;
	
	m_threadWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	_beginthread(&WindowMessageLoop, 0, this);

	WaitForSingleObject(m_threadWaitEvent, INFINITE);
	m_threadWaitEvent = NULL;

	m_initialized = true;
	return m_initialized;
}


void __cdecl SkypeMessaging::WindowMessageLoop(void *arg)
{
	static bool l_msgLoopRunning = false;

	if(!l_msgLoopRunning)
	{
		SkypeMessaging *l_msging = (SkypeMessaging*)arg;
		MSG l_msg;

		l_msgLoopRunning = true;

		l_msging->Init_WindowClass();
		l_msging->Init_MainWindow();

		if(l_msging->m_threadWaitEvent)
		{
			SetEvent(l_msging->m_threadWaitEvent);
		}

		if(l_msging->m_hWindow)
		{
			UpdateWindow(l_msging->m_hWindow);

			while(GetMessage(&l_msg, NULL, 0, 0) != 0)
			{
				TranslateMessage(&l_msg);
				DispatchMessage(&l_msg);
			}

			if(l_msging->m_threadWaitEvent)
			{
				SetEvent(l_msging->m_threadWaitEvent);
			}
		}

		l_msging->UnInit_MainWindow();
		l_msging->UnInit_WindowClass();

		l_msgLoopRunning = false;
	}

	_endthread();
}


LRESULT CALLBACK SkypeMessaging::WindowProc(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	LRESULT l_result = 0;
	bool l_callDefProc = false;
	SkypeMessaging *l_msging = SkypeMessaging::GetInstance();

	switch(uiMessage)
	{
	case WM_DESTROY:
		l_msging->m_hWindow = 0;
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
		DestroyWindow(hWindow);
		break;

	case WM_COPYDATA:
		if(l_msging->m_hSkypeWin == (HWND)wParam)
		{
			PCOPYDATASTRUCT poCopyData = (PCOPYDATASTRUCT)lParam;

			printf("Message from Skype(%u): %.*s\n", poCopyData->dwData, poCopyData->cbData, poCopyData->lpData);

			if(l_msging->m_eventCallback)
			{
				char *l_buf = new char[poCopyData->cbData + 1];

				memset(l_buf, 0, poCopyData->cbData + 1);
				memcpy(l_buf, poCopyData->lpData, poCopyData->cbData);

				l_msging->m_eventCallback(poCopyData->dwData, l_buf, l_msging->m_eventCallbackUserData);

				delete[] l_buf;
			}

			l_result = 1;
		}
	break;

	default:
		if(uiMessage == l_msging->m_winMsgID_SkypeControlAPIAttach)
		{
			switch(lParam)
			{
			case SKYPECONTROLAPI_ATTACH_SUCCESS:
				// successfully attached. save window handle to identify WM_COPYDATA stuff.
				l_msging->m_hSkypeWin = (HWND)wParam;

				if(l_msging->m_hSkypeWin)
				{
					l_msging->SendCommand("PROTOCOL 8");
				}
				break;
			case SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION:
				//printf("!!! Pending authorization\n");
				break;
			case SKYPECONTROLAPI_ATTACH_REFUSED:
				//printf("!!! Connection refused\n");
				break;
			case SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE:
				//printf("!!! Skype API not available\n");
				break;
			case SKYPECONTROLAPI_ATTACH_API_AVAILABLE:
				// skype just launched, so let's connect.
				l_msging->SendConnect();
				break;
			}
			l_result = 1;
			break; // leave outer switch
		}
		l_callDefProc = true;
	}

	if(l_callDefProc)
	{
		l_result = DefWindowProc(hWindow, uiMessage, wParam, lParam);
	}
	
	return l_result;
}


bool SkypeMessaging::SendConnect()
{
	return m_winMsgID_SkypeControlAPIDiscover && m_hWindow &&
		(SendMessage(HWND_BROADCAST, m_winMsgID_SkypeControlAPIDiscover, (WPARAM)m_hWindow, 0) != 0);
}


bool SkypeMessaging::SendCommand(const char *a_command)
{
	if(m_hSkypeWin && m_hWindow && a_command && *a_command)
	{
		COPYDATASTRUCT oCopyData;

		oCopyData.dwData = 0;
		oCopyData.lpData = const_cast<char*>(a_command);
		oCopyData.cbData = strlen(a_command) + 1;

		if(!SendMessage(m_hSkypeWin, WM_COPYDATA, (WPARAM)m_hWindow, (LPARAM)&oCopyData))
		{
			m_hSkypeWin = NULL;
		}
		else
		{
			return true;
		}
	}

	return false;
}


bool SkypeMessaging::IsConnected()
{
	if(m_hSkypeWin)
	{
		if(IsWindow(m_hSkypeWin))
		{
			return true;
		}
		m_hSkypeWin = NULL;
	}
	return false;
}


void SkypeMessaging::Disconnect()
{
	m_hSkypeWin = NULL;
}


bool SkypeMessaging::Init_WindowClass()
{
	UUID l_uuid;
	RPC_STATUS l_uuidHandle;

	bool l_success = false;

	l_uuidHandle = UuidCreate(&l_uuid);
	m_hInstance = (HINSTANCE)OpenProcess(PROCESS_DUP_HANDLE, false, GetCurrentProcessId());

	if(m_hInstance && (l_uuidHandle == RPC_S_OK || l_uuidHandle == RPC_S_UUID_LOCAL_ONLY))
	{
		RPC_WSTR l_uuidStr;

		if(UuidToString(&l_uuid, &l_uuidStr) == RPC_S_OK)
		{
			WNDCLASS l_newCls;
			const size_t l_bufSize = _tcslen((TCHAR*)l_uuidStr) + 9;

			if(m_winClassName) delete[] m_winClassName;
			m_winClassName = new TCHAR[l_bufSize];
			_tcscpy_s(m_winClassName, l_bufSize, _T("modskype"));
			_tcscat_s(m_winClassName, l_bufSize, (TCHAR*)l_uuidStr);

			memset(&l_newCls, 0, sizeof(WNDCLASS));
			l_newCls.style = CS_HREDRAW | CS_VREDRAW;
			l_newCls.hInstance = m_hInstance;
			l_newCls.lpszClassName = m_winClassName;
			l_newCls.lpfnWndProc = &WindowProc;

			l_success = (RegisterClass(&l_newCls) != 0);

			RpcStringFree(&l_uuidStr);
		}
	}

	if(!l_success)
	{
		CloseHandle(m_hInstance);
		m_hInstance = 0;
	}

	return l_success;
}


void SkypeMessaging::UnInit_WindowClass()
{
	if(m_winClassName)
	{
		UnregisterClass(m_winClassName, m_hInstance);

		delete[] m_winClassName;
		m_winClassName = NULL;
	}

	if(m_hInstance)
	{
		CloseHandle(m_hInstance);
		m_hInstance = 0;
	}
}


bool SkypeMessaging::Init_MainWindow()
{
	if(m_winClassName && m_hInstance)
	{
		m_hWindow = CreateWindow(m_winClassName, m_winClassName, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, m_hInstance, NULL);

		if(m_hWindow)
		{
			ShowWindow(m_hWindow, SW_HIDE);

			return true;
		}
		else
			HandleLastError();
	}
	return false;
}


void SkypeMessaging::UnInit_MainWindow()
{
	if(m_hWindow)
	{
		DestroyWindow(m_hWindow);
		m_hWindow = 0;
	}
}


bool SkypeMessaging::UnInitialize()
{
	m_initialized = false;

	return true;
}


#endif // _WIN32

