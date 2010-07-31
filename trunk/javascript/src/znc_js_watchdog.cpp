/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifdef _WIN32
#include "winver.h"
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#endif
#include "znc_js_watchdog.h"

#define WATCHDOG_INTERVAL_IN_SECONDS 1


IJSWatchDog::IJSWatchDog(JSRuntime* pRuntime)
{
	m_bIsWatching = false;
	m_pRuntime = pRuntime;
	m_iRefCount = 0;
}


void IJSWatchDog::Arm()
{
	m_iRefCount++;

	if(m_iRefCount == 1)
	{
		StartWatching();
	}
}


void IJSWatchDog::DisArm()
{
	if(m_iRefCount > 0)
	{
		m_iRefCount--;

		if(m_iRefCount == 0)
		{
			StopWatching();
		}
	}
}


void IJSWatchDog::WatchThat()
{
	JS_TriggerAllOperationCallbacks(m_pRuntime);
}


IJSWatchDog::~IJSWatchDog()
{
}


#ifdef _WIN32

CJSWatchDog::CJSWatchDog(JSRuntime* pRuntime) :
	IJSWatchDog(pRuntime)
{
	m_hTimer = INVALID_HANDLE_VALUE;
}


bool CJSWatchDog::StartWatching()
{
	if(m_hTimer != INVALID_HANDLE_VALUE)
	{
		return false;
	}

	if(CreateTimerQueueTimer(&m_hTimer, NULL, TimerCallback, this,
		WATCHDOG_INTERVAL_IN_SECONDS * 2 * 1000, WATCHDOG_INTERVAL_IN_SECONDS * 1000,
		WT_EXECUTEINTIMERTHREAD))
	{
		m_bIsWatching = true;
		return true;
	}

	return false;
}


bool CJSWatchDog::StopWatching()
{
	if(m_hTimer != INVALID_HANDLE_VALUE)
	{
		DeleteTimerQueueTimer(NULL, m_hTimer, INVALID_HANDLE_VALUE);
		// the last argument tells DeleteTimerQueueTimer to wait
		// for the possibly running callback function before returning.

		m_hTimer = INVALID_HANDLE_VALUE;

		return true;
	}

	m_bIsWatching = false;

	return false;
}


/*static*/ VOID CALLBACK CJSWatchDog::TimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	CJSWatchDog* pDog = reinterpret_cast<CJSWatchDog*>(lpParameter);

	if(pDog)
	{
		pDog->WatchThat();
	}
}


CJSWatchDog::~CJSWatchDog()
{
	StopWatching();
}

#else

CJSWatchDog::CJSWatchDog(JSRuntime* pRuntime) :
	IJSWatchDog(pRuntime)
{
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_cond, NULL);

	pthread_attr_init(&m_attr);
	pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_JOINABLE);
}


bool CJSWatchDog::StartWatching()
{
	if(!m_bIsWatching)
	{
		m_bIsWatching = 
			(pthread_create(&m_thread, &m_attr, TimerThreadProc, NULL) == 0);

		return m_bIsWatching;
	}

	return false;
}


bool CJSWatchDog::StopWatching()
{
	if(m_bIsWatching)
	{
		pthread_cancel(m_thread);
		m_bIsWatching = false;

		return true;
	}

	return false;
}


/*static*/ void* CJSWatchDog::TimerThreadProc(void* pArg)
{
	CJSWatchDog* pDog = reinterpret_cast<CJSWatchDog*>(pArg);

	if(pDog)
	{
		pthread_mutex_lock(&pDog->m_mutex);

		struct timespec ts = {0};
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += WATCHDOG_INTERVAL_IN_SECONDS * 2;

		while(pthread_cond_timedwait(&pDog->m_cond, &pDog->m_mutex, &ts) == ETIMEDOUT)
		{
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += WATCHDOG_INTERVAL_IN_SECONDS;

			pDog->WatchThat();
		}

		pthread_mutex_unlock(&pDog->m_mutex);
	}

	pthread_exit(NULL);
}


CJSWatchDog::~CJSWatchDog()
{
	StopWatching();

	pthread_attr_destroy(&m_attr);
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
}

#endif
