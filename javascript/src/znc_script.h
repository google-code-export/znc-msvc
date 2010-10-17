/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_JS_SCRIPT_H
#define _ZNC_JS_SCRIPT_H

#include "znc_smjs.h"
#include "znc_js_mod_events.h"
#include "znc_script_timer.h"

// ZNC headers:
#include "User.h"
#include "Chan.h"
#include "znc.h"

class CJavaScriptMod;

#define MAX_SCRIPT_EXECUTION_SECONDS 20

class CZNCScript
{
protected:
	CString m_sName;
	CString m_sFilePath;
	CString m_sArguments;

	CZNC* m_pZNC;
	CJavaScriptMod* m_pMod;
	CUser* m_pUser;

	JSContext* m_jsContext;
	JSObject* m_jsGlobalObj;

	JSScript* m_jsScript;
	JSObject* m_jsScriptObj;

	JSObject* m_jsUserObj;

	std::multimap<EModEvId, jsval*> m_eventHandlers;
	int m_nextTimerId;
	map<int, CJSTimer*> m_timers;

	uint64_t m_uBranchCallbackCount;
	uint64_t m_uBranchCallbackTime;

	MCString m_mssRegistry;

	bool SetUpGlobalClasses(CString& srErrorMessage);
	bool SetUpUserObject();

	void ClearEventHandlers();
	void ClearTimers();

	void EnterJS();
	void LeftJS();

	static void ScriptErrorCallback(JSContext* cx, const char* message, JSErrorReport* report);
	static JSBool ScriptOperationCallback(JSContext *cx);

public:
	CZNCScript(CJavaScriptMod* pMod, const CString& sName, const CString& sFilePath);
	virtual ~CZNCScript();

	bool LoadScript(CString& srErrorMessage);

	CZNC* GetZNC() const { return m_pZNC; }
	CJavaScriptMod* GetMod() const { return m_pMod; }
	CUser* GetUser() const { return m_pUser; }
	JSObject* GetJSUser() { return m_jsUserObj; }

	const CString& GetName() const { return m_sName; }
	const CString& GetArguments() const { return m_sArguments; }

	jsval* StoreEventHandler(const char* szEventName, const jsval& jvCallback);
	int InvokeEventHandler(EModEvId eEvent, uintN argc, jsval *argv, bool bModRet);
	bool IsEventHooked(EModEvId eEvent);
	bool RemoveEventHandler(const char* szEventName, const jsval& jvCallback);

	int AddTimer(unsigned int uInterval, bool bRepeat, const jsval& jvCallback);
	void RunTimerProc(CTimer *pTimer, jsval *pCallback);
	void DeleteExpiredTimer(CTimer *pTimer);
	bool RemoveTimer(int iTimerId);

	JSContext* GetContext() const { return m_jsContext; }
	JSObject* MakeAnonObject() const;
	JSObject* MakeNickObject(const CNick* pNick) const;
	JSObject* MakeChanObject(const CChan& Chan) const;

	bool LoadRegistry();
	bool SaveRegistry();
	bool SetNV(const CString& sName, const CString& sValue, bool bWriteToDisk = true);
	CString GetNV(const CString& sName);
	bool DelNV(const CString& sName, bool bWriteToDisk = true);
	bool ClearNV(bool bWriteToDisk = true);
};

#endif /* !_ZNC_JS_SCRIPT_H */
