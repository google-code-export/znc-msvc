/*
* Copyright (C) 2010 Ingmar Runge
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#ifndef _ZNC_SCRIPT_TIMER_H
#define _ZNC_SCRIPT_TIMER_H

#include "Modules.h"
#include "jsapi.h"

class CZNCScript;

class CJSTimer : public CTimer
{
public:
	CJSTimer(CZNCScript* pScript, unsigned int uInterval, bool bRepeat, const jsval& jvCallback);
	virtual ~CJSTimer();

	CZNCScript *GetScript() const { return m_pScript; }
protected:
	CZNCScript*	m_pScript;
	jsval* m_pCallback;
	virtual void RunJob();
};

#endif /* !_ZNC_SCRIPT_TIMER_H */
