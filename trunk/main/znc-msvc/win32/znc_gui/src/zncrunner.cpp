#include "zncrunner.hpp"


CZNCRunner::CZNCRunner()
{
	m_running = m_shutDown = false;
	m_actionWaitEvent = m_threadWaitEvent = NULL;
	m_startupFailReason = 0;
	m_externalOutputHook = NULL;

	InitializeCriticalSection(&m_reqQueueLock);
}


CZNCRunner *CZNCRunner::Get()
{
	static CZNCRunner *ls_instance = new CZNCRunner();
	return ls_instance;
}


bool CZNCRunner::Start()
{
	if(!IsRunning())
	{
		m_threadWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		CUtils::HookOutput(&OutputHookProc, NULL);

		if(_beginthread(&ThreadProc, 0, this) != -1)
		{
#if 0
			if(WaitForSingleObject(m_threadWaitEvent, 10 * 1000) == WAIT_OBJECT_0)
#endif
			{
				m_threadWaitEvent = NULL;

				return true;
			}
		}

		m_actionWaitEvent = NULL;
	}

	return false;
}


void CZNCRunner::OutputHookProc(int type, const char* text, void *userData)
{
	/* runs in ZNC's thread */

	if(CZNCRunner::Get()->m_externalOutputHook)
	{
		// not 100% thread safe...
		CZNCRunner::Get()->m_externalOutputHook(type, text, userData);
	}
}


void CZNCRunner::SetOutputHook(outputHook hook)
{
	m_externalOutputHook = hook;
}


bool CZNCRunner::ThreadWorker()
{
	CZNC &pZNC = CZNC::Get();

	pZNC.InitDirs("", "");

	if(!pZNC.ParseConfig(""))
	{
		m_startupFailReason = 1;
	}
	else if(!pZNC.OnBoot())
	{
		m_startupFailReason = 2;
	}

	if(m_threadWaitEvent)
	{
		// used for Wait... in Start()
		SetEvent(m_threadWaitEvent);
	}

	if(m_startupFailReason == 0)
	{
		CGuiTimer *l_timer = new CGuiTimer();
		bool l_dummy = true;

		try
		{
			pZNC.GetManager().AddCron(l_timer);
			pZNC.Loop(&l_dummy);
		}
		catch (CException e)
		{
			if(e.GetType() == CException::EX_Shutdown)
			{
				m_shutDown = true;
			}
		}
	}
	else
	{
		m_shutDown = true;
	}

	CZNC::_Reset();

	return !m_shutDown;
}


void __cdecl CZNCRunner::ThreadProc(void *arg)
{
	CZNCRunner *l_instance = reinterpret_cast<CZNCRunner*>(arg);

	if(l_instance)
	{
		l_instance->m_running = true;

		while(l_instance->ThreadWorker())
		{
			// call ThreadWorker again
		}

		l_instance->m_running = false;

		if(l_instance->m_threadWaitEvent)
		{
			SetEvent(l_instance->m_threadWaitEvent);
		}
	}

	_endthread();
}


static void ShutDownDelegate(long long, void*)
{
	CZNC::Get().Broadcast("ZNC is shutting down. Requested via UI.");
	throw CException(CException::EX_Shutdown);
}


bool CZNCRunner::Stop()
{
	if(IsRunning())
	{
		// send shutdown command to ZNC:

		RequestThreadspace(&ShutDownDelegate, 0, NULL);

#if 0
		// give the "worker" thread some time to shut down:
		m_threadWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(WaitForSingleObject(m_threadWaitEvent, 10 * 1000) == WAIT_OBJECT_0)
#endif
		{
			return true;
		}
	}

	return false;
}


bool CZNCRunner::IsRunning()
{
	return m_running;
}


bool CZNCRunner::RequestThreadspace(ZNCRunnerThreadspaceRequestCallback callback, long long userData1, void *userData2)
{
	if(callback != NULL && IsRunning())
	{
		EnterCriticalSection(&m_reqQueueLock);

		m_requestQueue.push(ZNCRunnerThreadspaceRequest());
		m_requestQueue.front().cb = callback;
		m_requestQueue.front().userData1 = userData1;
		m_requestQueue.front().userData2 = userData2;

		LeaveCriticalSection(&m_reqQueueLock);

		return true;
	}

	return false;
}


void CZNCRunner::OnThreadspaceGranted()
{
	time_t l_endTime = time(NULL) + 1;

	do
	{
		ZNCRunnerThreadspaceRequest l_req;
		bool l_gotReq = false;

		// get a request from the queue:
		EnterCriticalSection(&m_reqQueueLock);

		if(!m_requestQueue.empty())
		{
			l_req = m_requestQueue.front();
			m_requestQueue.pop();
			l_gotReq = true;
		}

		LeaveCriticalSection(&m_reqQueueLock);

		if(l_gotReq)
		{
			// give time in ZNC's thread to the callback, as requested:
			l_req.cb(l_req.userData1, l_req.userData2);
		}

	} while(time(NULL) <= l_endTime); // Try not to spend more than one or two seconds in this loop.
}


CZNCRunner::~CZNCRunner()
{
	Stop();
	DeleteCriticalSection(&m_reqQueueLock);
}


CGuiTimer::CGuiTimer()
{
	m_lastAction = time(NULL);
	Start(1);
}


void CGuiTimer::RunJob()
{
	// just a security measure to make sure we don't block ZNC's thread for too long after standy, etc.:
	if(m_lastAction < time(NULL))
	{
		CZNCRunner *l_runner = CZNCRunner::Get(); 

		if(!l_runner || !l_runner->IsRunning()) return;

		l_runner->OnThreadspaceGranted();

		m_lastAction = time(NULL);
	}
}


CGuiTimer::~CGuiTimer()
{
}
