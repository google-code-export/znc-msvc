/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "winver.h"
#include <windows.h>
#include "znc_js_watchdog.h"


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
	//StopWatching(); // this causes an unresolved external symbol? wtf? why?
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
		2000, 1000, WT_EXECUTEINTIMERTHREAD))
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

#endif
