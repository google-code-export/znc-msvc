#pragma once

#include "main.h"
/*#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <vcclr.h>
#include <msclr\auto_gcroot.h>

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Generic;

using namespace msclr;*/

typedef void (*ZNCRunnerThreadspaceRequestCallback)(long long userData1, void *userData2);


class CGuiTimer : public CCron
{
public:
	CGuiTimer();
	virtual ~CGuiTimer();
protected:
	time_t m_lastAction;
	void RunJob();
};


class CZNCRunner
{
	friend class CGuiTimer;
public:
	static CZNCRunner *Get();
	bool Start();
	bool Stop();
	bool IsRunning();
	void SetOutputHook(outputHook hook);
	bool RequestThreadspace(ZNCRunnerThreadspaceRequestCallback callback, long long userData1, void *userData2);

protected:
	bool m_running;
	bool m_shutDown;
	int m_startupFailReason;
	HANDLE m_actionWaitEvent, m_threadWaitEvent;

	outputHook m_externalOutputHook;

	struct ZNCRunnerThreadspaceRequest
	{
		ZNCRunnerThreadspaceRequestCallback cb;
		long long userData1;
		void *userData2;
	};

	CRITICAL_SECTION m_reqQueueLock;
	std::queue<ZNCRunnerThreadspaceRequest> m_requestQueue;
	
	void OnThreadspaceGranted();
private:
	CZNCRunner();
	~CZNCRunner();
	static void __cdecl ThreadProc(void *arg);
	bool ThreadWorker();
	static void OutputHookProc(int type, const char* text, void *userData);
};

