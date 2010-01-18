#include "main.h"
#include "znc_script_timer.h"
#include "znc_js_mod.h"


CJSTimer::CJSTimer(CZNCScript* pScript, unsigned int uInterval, bool bRepeat, const jsval& jvCallback) :
	CTimer((CModule*)pScript->GetMod(), uInterval, (bRepeat ? 0 : 1),
		"JavaScript Timer", "Used for setInterval and setTimeout from JS scripts.")
{
	m_pScript = pScript;
	m_pCallback = new jsval;
	*m_pCallback = jvCallback;

	JS_AddRoot(m_pScript->GetContext(), m_pCallback);
}


void CJSTimer::RunJob()
{
	m_pScript->RunTimerProc(this, m_pCallback);
}


CJSTimer::~CJSTimer()
{
	JS_RemoveRoot(m_pScript->GetContext(), m_pCallback);
	m_pScript->DeleteExpiredTimer(this);
}
