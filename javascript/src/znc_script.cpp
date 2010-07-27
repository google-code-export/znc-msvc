/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "main.h"
#include "znc_script.h"
#include "znc_script_funcs.h"
#include "znc_js_mod.h"
#include "util.h"

#include "znc_js_mod_events.inc"

using namespace std;


/************************************************************************/
/* CONSTRUCTOR                                                          */
/************************************************************************/

CZNCScript::CZNCScript(CJavaScriptMod* pMod, const CString& sName, const CString& sFilePath)
{
	m_sName = sName;
	m_sFilePath = sFilePath;

	m_pMod = pMod;
	m_pUser = pMod->GetUser();
	m_pZNC = &CZNC::Get();

	m_jsContext = NULL;
	m_jsGlobalObj = NULL;
	m_jsScript = NULL;
	m_jsScriptObj = NULL;

	m_uBranchCallbackCount = 0;
	m_uBranchCallbackTime = 0;

	m_nextTimerId = 1;
}


/************************************************************************/
/* SCRIPT LOADER                                                        */
/************************************************************************/

bool CZNCScript::LoadScript(CString& srErrorMessage)
{
	if(m_jsContext)
	{
		srErrorMessage = "Script already in use";
		return false;
	}

	/* set up JS context and global obj */
	m_jsContext = JS_NewContext(m_pMod->GetJSRuntime(), 8192);

	if(!m_jsContext)
	{
		srErrorMessage = "Creating a script context failed!";
		return false;
	}

#ifdef JSOPTION_RELIMIT // supported in 1.8.0 and later only
	// https://developer.mozilla.org/en/JS_SetOptions
	JS_SetOptions(m_jsContext, JSOPTION_VAROBJFIX | JSOPTION_STRICT | JSOPTION_RELIMIT);
#else
	JS_SetOptions(m_jsContext, JSOPTION_VAROBJFIX | JSOPTION_STRICT);
#endif

#if JS_VERSION >= 180
	JS_SetVersion(m_jsContext, JSVERSION_LATEST);
#else
	JS_SetVersion(m_jsContext, JSVERSION_DEFAULT);
#endif

	// save pointer to this instance for error callback etc:
	JS_SetContextPrivate(m_jsContext, this);

	JS_SetErrorReporter(m_jsContext, ScriptErrorCallback);

	if(!SetUpGlobalClasses(srErrorMessage))
	{
		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;
		return false;
	}

	if(!SetUpUserObject())
	{
		srErrorMessage = "Unable to create user object.";
		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;
		return false;
	}

	JS_SetBranchCallback(m_jsContext, ScriptBranchCallback);

	/* read the script from disk */
	CFile cFile(m_sFilePath);

	if(!cFile.Exists() || !cFile.Open())
	{
		srErrorMessage = "Opening the script file [" + m_sFilePath + "] failed.";
		JS_RemoveRoot(m_jsContext, &m_jvUserObj);
		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;
		return false;
	}
	else
	{
		// make sure we got the registry strings ready & available.
		LoadRegistry();
	}

	bool bUtf16 = false, bUtf8 = false;
	unsigned char* szBuf = NULL;
	size_t uBufLen = 0;
	unsigned char bomBuf[3];

	if(cFile.Read((char*)&bomBuf, 3) == 3)
	{
		if(bomBuf[0] == 0xEF && bomBuf[1] == 0xBB && bomBuf[2] == 0xBF)
		{
			bUtf8 = true;
		}
		// fuck big endian:
		else if(bomBuf[0] == 0xFF && bomBuf[1] == 0xFE)
		{
			bUtf16 = true;
		}

		if(!bUtf16 && !bUtf8)
		{
			cFile.Seek(0);
		}
		else if(bUtf16)
		{
			cFile.Seek(2);
		}

		uBufLen = 1024;
		szBuf = (unsigned char*)malloc(uBufLen);
		unsigned char* p = szBuf;
		unsigned char tempBuf[1000];
		while(size_t uBytesRead = cFile.Read((char*)&tempBuf, sizeof(tempBuf)))
		{
			size_t uOff = (p - szBuf);
			if(uOff + uBytesRead >= uBufLen)
			{
				uBufLen = uBufLen + uBytesRead + 1024;
				szBuf = (unsigned char*)realloc(szBuf, uBufLen);
				p = szBuf + uOff;
			}
			memmove(p, tempBuf, uBytesRead);
			p += uBytesRead;
		}
		*(uint32_t*)p = 0; // we definitely have 4 bytes left since we added 1024 above.
		// we use 4 bytes since sizeof(wchar_t) may be 4. Just to be sure.
		uBufLen = (p - szBuf);
	}

	cFile.Close();

	if(!szBuf)
	{
		srErrorMessage = "The script file seems to be empty.";
		JS_RemoveRoot(m_jsContext, &m_jvUserObj);
		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;
		return false;
	}

	/* load (+ run, initialize) the script */
	jsval jvRet;

	m_uBranchCallbackCount = m_uBranchCallbackTime = 0;

	if(bUtf16)
	{
		m_jsScript = JS_CompileUCScript(m_jsContext, m_jsGlobalObj,
			(jschar*)szBuf, uBufLen / sizeof(jschar), m_sName.c_str(), 0);
	}
	else if(bUtf8 || g_utf8_validate((char*)szBuf, uBufLen, NULL))
	{
		size_t uScriptCharCount;
		const jschar* jwcScript = CUtil::MsgCpyToWideEx(CString().append((char*)szBuf, uBufLen), uScriptCharCount);

		m_jsScript = JS_CompileUCScript(m_jsContext, m_jsGlobalObj, jwcScript, uScriptCharCount, m_sName.c_str(), 0);
	}
	else
	{
		srErrorMessage = "WARNING: The script file's encoding is neither ANSI, nor UTF-8, nor UTF-16 (LE+BOM).";

		m_jsScript = JS_CompileScript(m_jsContext, m_jsGlobalObj, (char*)szBuf, uBufLen, m_sName.c_str(), 0);
	}

	free(szBuf); szBuf = 0;

	if(!m_jsScript ||
		!(m_jsScriptObj = JS_NewScriptObject(m_jsContext, m_jsScript)) ||
		!JS_AddRoot(m_jsContext, &m_jsScriptObj))
	{
		srErrorMessage = "Loading " + m_sName + ".js failed!";

		if(m_jsScript)
		{
			JS_DestroyScript(m_jsContext, m_jsScript);
			m_jsScript = NULL;
		}

		JS_RemoveRoot(m_jsContext, &m_jvUserObj);
		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;

		return false;
	}

	if(!JS_ExecuteScript(m_jsContext, m_jsGlobalObj, m_jsScript, &jvRet))
	{
		srErrorMessage = "Running the script failed!";

		JS_RemoveRoot(m_jsContext, &m_jsScriptObj);

		// clear any (rooted) function objects that may have been added before the error:
		ClearEventHandlers();
		// and the same thing for timers:
		ClearTimers();

		JS_RemoveRoot(m_jsContext, &m_jvUserObj);

		JS_GC(m_jsContext); // invoke other destructors

		JS_DestroyContext(m_jsContext);
		m_jsContext = NULL;

		return false;
	}
	else
	{
		jsval jvModArgs = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(m_jsContext, "<:TODO:>"));
		InvokeEventHandler(ModEv_OnLoad, 1, &jvModArgs, false);

		JS_GC(m_jsContext);

		return true;
	}
}


/*static*/ void CZNCScript::ScriptErrorCallback(JSContext* cx, const char* message, JSErrorReport* report)
{
	CZNCScript* pScript = static_cast<CZNCScript*>(JS_GetContextPrivate(cx));

	// :TODO: check for new lines and overly long messages...
	pScript->m_pMod->PutModule("[JS ERROR in " +
		CString(report->filename ? report->filename : pScript->GetName()) +
		".js at line " + (report->lineno == 0 ? CString("?") : CString(report->lineno + 1)) + "]: " + 
		CString(message ? message : "(unknown error)"));
}

// Source:
// http://mxr.mozilla.org/firefox2/source/dom/src/base/nsJSEnvironment.cpp#528
// (GPL v2)

// The number of branch callbacks between calls to JS_MaybeGC
#define MAYBE_GC_BRANCH_COUNT_MASK 0x00000fff // 4095

// The number of branch callbacks before we even check if our start
// timestamp is initialized. This is a fairly low number as we want to
// initialize the timestamp early enough to not waste much time before
// we get there, but we don't want to bother doing this too early as
// it's not generally necessary.
#define INITIALIZE_TIME_BRANCH_COUNT_MASK 0x000000ff // 255

/*static*/ JSBool CZNCScript::ScriptBranchCallback(JSContext *cx, JSScript *script)
{
	CZNCScript* pScript = static_cast<CZNCScript*>(JS_GetContextPrivate(cx));

	uint64_t uCallbackCount = ++pScript->m_uBranchCallbackCount;

	if(uCallbackCount & INITIALIZE_TIME_BRANCH_COUNT_MASK)
		return JS_TRUE;

	if(uCallbackCount == INITIALIZE_TIME_BRANCH_COUNT_MASK + 1 &&
		pScript->m_uBranchCallbackTime == 0)
	{
		pScript->m_uBranchCallbackTime = time(NULL);

		return JS_TRUE;
	}

	if(uCallbackCount & MAYBE_GC_BRANCH_COUNT_MASK)
		return JS_TRUE;

	// Run the GC if we get this far.
	JS_MaybeGC(cx);

	time_t tDelta = (time(NULL) - pScript->m_uBranchCallbackTime);

	if(tDelta > MAX_SCRIPT_EXECUTION_SECONDS)
	{
		pScript->GetMod()->PutModule("WARNING: The script " +
			pScript->GetName() + ".js exceeded the maximum run time of " + CString(MAX_SCRIPT_EXECUTION_SECONDS) + 
			" seconds and has been terminated!");
		
		return JS_FALSE;
	}

	return JS_TRUE;
}


/************************************************************************/
/* EVENT HANDLER METHODS                                                */
/************************************************************************/

jsval* CZNCScript::StoreEventHandler(const char* szEventName, const jsval& jsvCallback)
{
	EModEvId uModId = _ModEV_Max;

	for(int i = 0; s_mod_event_names[i] != NULL; i++)
	{
		if(strcasecmp(szEventName, s_mod_event_names[i]) == 0)
		{
			uModId = (EModEvId)i;
			break;
		}
	}

	if(uModId != _ModEV_Max)
	{
		jsval *pCallback = new jsval;
		*pCallback = jsvCallback;
		m_eventHandlers.insert(pair<EModEvId, jsval*>(uModId, pCallback));

		return pCallback;
	}

	return NULL;
}


int CZNCScript::InvokeEventHandler(EModEvId eEvent, uintN argc, jsval *argv, bool bModRet)
{
	pair<multimap<EModEvId, jsval*>::iterator, multimap<EModEvId, jsval*>::iterator>
		find = m_eventHandlers.equal_range(eEvent);

	CModule::EModRet eModRet = CModule::CONTINUE;

	m_uBranchCallbackCount = m_uBranchCallbackTime = 0;

	for(multimap<EModEvId, jsval*>::iterator it = find.first; it != find.second; it++)
	{
		jsval jvRet;
		JSBool bOK;

		bOK = JS_CallFunctionValue(m_jsContext, m_jsGlobalObj, *it->second, argc, argv, &jvRet);

		if(!bOK)
		{
			// :TODO: Do something!
			break;
		}

		if(bModRet && JSVAL_IS_INT(jvRet))
		{
			int iModRet = JSVAL_TO_INT(jvRet);

			if(iModRet == CModule::HALT || iModRet == CModule::HALTCORE || iModRet == CModule::HALTMODS)
			{
				eModRet = (CModule::EModRet)iModRet;
			}
		}
	}

	return eModRet;
}


bool CZNCScript::IsEventHooked(EModEvId eEvent)
{
	return (m_eventHandlers.count(eEvent) > 0);
}


bool CZNCScript::RemoveEventHandler(const char* szEventName, const jsval& jvCallback)
{
	EModEvId uModId = _ModEV_Max;

	for(int i = 0; s_mod_event_names[i] != NULL; i++)
	{
		if(strcasecmp(szEventName, s_mod_event_names[i]) == 0)
		{
			uModId = (EModEvId)i;
			break;
		}
	}

	for(multimap<EModEvId, jsval*>::iterator it = m_eventHandlers.begin(); it != m_eventHandlers.end(); it++)
	{
		if(it->first == uModId && *it->second == jvCallback)
		{
			JS_RemoveRoot(m_jsContext, it->second);
			delete it->second;
			m_eventHandlers.erase(it);
			return true;
		}
	}

	return false;
}


void CZNCScript::ClearEventHandlers()
{
	for(multimap<EModEvId, jsval*>::iterator it = m_eventHandlers.begin(); it != m_eventHandlers.end(); it++)
	{
		JS_RemoveRoot(m_jsContext, it->second);
		delete it->second;
	}
	m_eventHandlers.clear();
}


/************************************************************************/
/* TIMER HANDLING                                                       */
/************************************************************************/

int CZNCScript::AddTimer(unsigned int uInterval, bool bRepeat, const jsval& jvCallback)
{
	int iNewId = m_nextTimerId++;

	CJSTimer* pTimer = new CJSTimer(this, uInterval, bRepeat, jvCallback);
	m_timers[iNewId] = pTimer;
	m_pMod->AddTimer(pTimer);

	return iNewId;
}


void CZNCScript::RunTimerProc(CTimer *pTimer, jsval *pCallback)
{
	JSBool bOK;
	jsval jvRet;

	m_uBranchCallbackCount = m_uBranchCallbackTime = 0;

	bOK = JS_CallFunctionValue(m_jsContext, m_jsGlobalObj, *pCallback, 0, NULL, &jvRet);

	if(!bOK)
	{
		// :TODO: do something...
	}
}


bool CZNCScript::RemoveTimer(int iTimerId)
{
	if(m_timers.count(iTimerId) > 0)
	{
		m_pMod->RemTimer(m_timers[iTimerId]);
		m_timers.erase(iTimerId);

		return true;
	}

	return false;
}


void CZNCScript::DeleteExpiredTimer(CTimer *pTimer)
{
	for(map<int, CJSTimer*>::iterator it = m_timers.begin(); it != m_timers.end(); it++)
	{
		if(it->second == pTimer)
		{
			m_timers.erase(it);
			break;
		}
	}
}


void CZNCScript::ClearTimers()
{
	for(map<int, CJSTimer*>::iterator it = m_timers.begin(); it != m_timers.end(); )
	{
		size_t uOldCount = m_timers.size();

		m_pMod->RemTimer(it->second);

		if(m_timers.size() == uOldCount)
		{
			m_timers.erase(it);
		}

		it = m_timers.begin();
	}

	m_timers.clear();
}


/************************************************************************/
/* NV / REGISTRY STUFF                                                  */
/************************************************************************/

bool CZNCScript::LoadRegistry()
{
	return (m_mssRegistry.ReadFromDisk(m_pMod->GetSavePath() + "/" + m_sName + ".registry", 0600) == MCString::MCS_SUCCESS);
}


bool CZNCScript::SaveRegistry()
{
	return (m_mssRegistry.WriteToDisk(m_pMod->GetSavePath() + "/" + m_sName + ".registry", 0600) == MCString::MCS_SUCCESS);
}


bool CZNCScript::SetNV(const CString& sName, const CString& sValue, bool bWriteToDisk)
{
	m_mssRegistry[sName] = sValue;

	if(bWriteToDisk)
		return SaveRegistry();
	else
		return true;
}


CString CZNCScript::GetNV(const CString& sName)
{
	MCString::iterator it = m_mssRegistry.find(sName);

	if(it != m_mssRegistry.end())
		return it->second;
	else
		return "";
}


bool CZNCScript::DelNV(const CString& sName, bool bWriteToDisk)
{
	m_mssRegistry.erase(sName);

	if(bWriteToDisk)
		return SaveRegistry();
	else
		return true;
}


bool CZNCScript::ClearNV(bool bWriteToDisk)
{
	m_mssRegistry.clear();

	if(bWriteToDisk)
		return SaveRegistry();
	else
		return true;
}


/************************************************************************/
/* MISC UTILS                                                           */
/************************************************************************/

JSObject* CZNCScript::MakeAnonObject() const
{
	return JS_NewObject(m_jsContext, NULL, NULL, NULL);
}


/************************************************************************/
/* DESTRUCTOR                                                           */
/************************************************************************/

CZNCScript::~CZNCScript()
{
	if(m_jsContext)
	{
		ClearTimers();
		ClearEventHandlers();
		JS_RemoveRoot(m_jsContext, &m_jvUserObj);
		JS_RemoveRoot(m_jsContext, &m_jsScriptObj);
		JS_DestroyContext(m_jsContext);
	}
}
