/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_JS_WATCHDOG_H
#define _ZNC_JS_WATCHDOG_H

#include "znc_smjs.h"

// base class for Windows and Linux implementations:
class IJSWatchDog
{
private:
	IJSWatchDog();
	int m_iRefCount;

protected:
	bool m_bIsWatching;
	JSRuntime *m_pRuntime;

	virtual bool StartWatching() = 0;
	virtual bool StopWatching() = 0;

	void WatchThat();

public:
	IJSWatchDog(JSRuntime* pRuntime);
	virtual ~IJSWatchDog();

	bool IsWatching() const { return m_bIsWatching; }

	void Arm();
	void DisArm();
};


#ifdef _WIN32
// Win32 implementation: based on Timer Queue Timers.

class CJSWatchDog : public IJSWatchDog
{
protected:
	HANDLE m_hTimer;

	bool StartWatching();
	bool StopWatching();

	static VOID CALLBACK TimerCallback(PVOID, BOOLEAN);
public:
	CJSWatchDog(JSRuntime* pRuntime);
	virtual ~CJSWatchDog();
};

#else
	// Linux implementation: using pthreads.
#endif

#endif /* !_ZNC_JS_WATCHDOG_H */
