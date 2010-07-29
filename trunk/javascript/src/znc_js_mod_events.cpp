/*
* Copyright (C) 2010 Ingmar Runge
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#include "main.h"
#include "Modules.h"
#include "znc_smjs.h"
#include "znc_js_mod.h"
#include "znc_js_mod_events.h"
#include "util.h"


CModule::EModRet CJavaScriptMod::InvokeNoArgScriptCallbacks(EModEvId eEvent, bool bModRet)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		if((*it)->IsEventHooked(eEvent))
		{
			EModRet eRet = (EModRet)(*it)->InvokeEventHandler(eEvent, 0, NULL, bModRet);

			if(bModRet && eRet != CONTINUE)
			{
				return eRet;
			}
		}
	}

	return CONTINUE;
}

void CJavaScriptMod::OnPreRehash() { InvokeNoArgScriptCallbacks(ModEv_OnPreRehash, false); }
void CJavaScriptMod::OnPostRehash() { InvokeNoArgScriptCallbacks(ModEv_OnPostRehash, false); }
void CJavaScriptMod::OnIRCDisconnected() { InvokeNoArgScriptCallbacks(ModEv_OnIRCDisconnected, false); }
void CJavaScriptMod::OnIRCConnected() { InvokeNoArgScriptCallbacks(ModEv_OnIRCConnected, false); }
void CJavaScriptMod::OnClientLogin() { InvokeNoArgScriptCallbacks(ModEv_OnClientLogin, false); }
void CJavaScriptMod::OnClientDisconnect() { InvokeNoArgScriptCallbacks(ModEv_OnClientDisconnect, false); }


CModule::EModRet CJavaScriptMod::_OnOneVariableStringArg(EModEvId eEventId, const char* szPropName, CString& sArg)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		JSObject* joContainer = pScript->MakeAnonObject();
		jsval jvLine = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg));

		if(JS_SetProperty(pScript->GetContext(), joContainer, szPropName, &jvLine))
		{
			jsval jvTmp = OBJECT_TO_JSVAL(joContainer);
			EModRet eRet = (EModRet)pScript->InvokeEventHandler(eEventId, 1, &jvTmp, true);

			jsval jvNewLine;
			if(JS_GetProperty(pScript->GetContext(), joContainer, szPropName, &jvNewLine) &&
				JSVAL_IS_STRING(jvNewLine))
			{
				sArg = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNewLine));
			}

			if(eRet != CONTINUE) return eRet;
		}
	}

	return CONTINUE;
}

void CJavaScriptMod::_OnOneConstStringArg(EModEvId eEventId, const CString& sArg)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		jsval jvLine = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg));
		pScript->InvokeEventHandler(eEventId, 1, &jvLine, true);
	}
}

CModule::EModRet CJavaScriptMod::OnRaw(CString& sLine) { return _OnOneVariableStringArg(ModEv_OnRaw, "line", sLine); }
CModule::EModRet CJavaScriptMod::OnBroadcast(CString& sMessage) { return _OnOneVariableStringArg(ModEv_OnBroadcast, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnUserTopicRequest(CString& sChannel) { return _OnOneVariableStringArg(ModEv_OnUserTopicRequest, "channel", sChannel); }

CModule::EModRet CJavaScriptMod::OnStatusCommand(CString& sCommand)
{
	if(!m_pUser->DenyLoadMod())
	{
		const CString sCmd = sCommand.Token(0).AsUpper();
		CString sMod = sCommand.Token(1);

		if((sCmd.Equals("LOADMOD") || sCmd.Equals("LOADMODULE")) && sMod.TrimSuffix(".js"))
		{
			CString sArgs = sCommand.Token(2, true);
			CString sError;

			if(!LoadModule(sMod, sArgs, sError))
			{
				PutStatus("Unable to load [" + sMod + ".js] [" + sError + "].");
			}
			else
			{
				PutStatus("Successfully loaded " + sMod + ".js!");
				SaveToDisk();
			}

			return HALT;
		}
		else if((sCmd.Equals("UNLOADMOD") || sCmd.Equals("UNLOADMODULE")) && sMod.TrimSuffix(".js"))
		{
			CString sError;

			if(!UnLoadModule(sMod, sError))
			{
				PutStatus("Unable to unload [" + sMod + ".js] [" + sError + "].");
			}
			else
			{
				PutStatus("Successfully unloaded " + sMod + ".js!");
				SaveToDisk();
			}

			return HALT;
		}
	}

	return _OnOneVariableStringArg(ModEv_OnStatusCommand, "command", sCommand);
}


CModule::EModRet CJavaScriptMod::OnUserRaw(CString& sLine)
{
	EModRet eModRet = _OnOneVariableStringArg(ModEv_OnUserRaw, "line", sLine);

	if(eModRet == CONTINUE)
	{
		CString sMyLine(sLine);
		if(sMyLine.Left(1) == ":") sMyLine.LeftChomp();
		const CString sCommand(sMyLine.Token(0));

		if(sCommand.Equals("NOTICE") || sCommand.Equals("PRIVMSG"))
		{
			CString sTarget = sMyLine.Token(1);
			CString sMsg = sMyLine.Token(2, true);
			if(sMsg.Left(1) == ":") sMsg.LeftChomp();

			if(sTarget.TrimPrefix(m_pUser->GetStatusPrefix()) &&
				!sTarget.Equals("status") && !sMsg.empty())
			{
				for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
				{
					if(!sTarget.Equals((*it)->GetName()))
						continue;

					if(sCommand.Equals("NOTICE"))
					{
						_OnOneConstStringArg(ModEv_OnModNotice, sMsg);
					}
					else if(sMsg.WildCmp("\001*\001"))
					{
						CString sCTCP = sMsg; sCTCP.LeftChomp(); sCTCP.RightChomp();
						if(!sCTCP.Equals("DCC ", false, 4))
						{
							_OnOneConstStringArg(ModEv_OnModCTCP, sCTCP);
						}
					}
					else
					{
						_OnOneConstStringArg(ModEv_OnModCommand, sMsg);
					}
					return HALT;
				}
			}
		}
	}

	return eModRet;
}

CModule::EModRet CJavaScriptMod::_OnTwoVariableStringArgs(EModEvId eEventId, const char* szPropName1, const char* szPropName2, CString& sArg1, CString& sArg2)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		JSObject* joContainer = pScript->MakeAnonObject();
		jsval jvLine1 = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg1));
		jsval jvLine2 = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg2));

		if(JS_SetProperty(pScript->GetContext(), joContainer, szPropName1, &jvLine1) &&
			JS_SetProperty(pScript->GetContext(), joContainer, szPropName2, &jvLine2))
		{
			jsval jvTmp = OBJECT_TO_JSVAL(joContainer);
			EModRet eRet = (EModRet)pScript->InvokeEventHandler(eEventId, 1, &jvTmp, true);

			jsval jvNewLine[2];
			if(JS_GetProperty(pScript->GetContext(), joContainer, szPropName1, &jvNewLine[0]) &&
				JSVAL_IS_STRING(jvNewLine[0]) &&
				JS_GetProperty(pScript->GetContext(), joContainer, szPropName2, &jvNewLine[1]) &&
				JSVAL_IS_STRING(jvNewLine[2]))
			{
				sArg1 = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNewLine[0]));
				sArg2 = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNewLine[1]));
			}

			if(eRet != CONTINUE) return eRet;
		}
	}

	return CONTINUE;
}

CModule::EModRet CJavaScriptMod::OnUserCTCPReply(CString& sTarget, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserCTCPReply, "target", "message", sTarget, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserCTCP(CString& sTarget, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserCTCP, "target", "message", sTarget, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserAction(CString& sTarget, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserAction, "target", "message", sTarget, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserMsg(CString& sTarget, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserMsg, "target", "message", sTarget, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserNotice(CString& sTarget, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserNotice, "target", "message", sTarget, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserJoin(CString& sChannel, CString& sKey)
{ return _OnTwoVariableStringArgs(ModEv_OnUserJoin, "channel", "key", sChannel, sKey); }
CModule::EModRet CJavaScriptMod::OnUserPart(CString& sChannel, CString& sMessage)
{ return _OnTwoVariableStringArgs(ModEv_OnUserPart, "channel", "message", sChannel, sMessage); }
CModule::EModRet CJavaScriptMod::OnUserTopic(CString& sChannel, CString& sTopic)
{ return _OnTwoVariableStringArgs(ModEv_OnUserTopic, "channel", "topic", sChannel, sTopic); }


CModule::EModRet CJavaScriptMod::_OnCNickAndOneVarStringArg(EModEvId eEventId, const CNick& Nick, const char* szPropName, CString& sArg)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		JSObject* jsoNick = pScript->MakeNickObject(&Nick);
		JSObject* joContainer = pScript->MakeAnonObject();
		jsval jvMessage = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg));

		if(JS_SetProperty(pScript->GetContext(), joContainer, szPropName, &jvMessage))
		{
			jsval jvArgs[2];
			jvArgs[0] = OBJECT_TO_JSVAL(jsoNick);
			jvArgs[1] = OBJECT_TO_JSVAL(joContainer);
			EModRet eRet = (EModRet)pScript->InvokeEventHandler(eEventId, 2, jvArgs, true);

			jsval jvNewArg;
			if(JS_GetProperty(pScript->GetContext(), joContainer, szPropName, &jvNewArg) &&
				JSVAL_IS_STRING(jvNewArg))
			{
				sArg = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNewArg));
			}

			if(eRet != CONTINUE) return eRet;
		}
	}
	return CONTINUE;
}

CModule::EModRet CJavaScriptMod::OnPrivCTCP(CNick& Nick, CString& sMessage)
{ return _OnCNickAndOneVarStringArg(ModEv_OnPrivCTCP, Nick, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnCTCPReply(CNick& Nick, CString& sMessage)
{ return _OnCNickAndOneVarStringArg(ModEv_OnCTCPReply, Nick, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnPrivNotice(CNick& Nick, CString& sMessage)
{ return _OnCNickAndOneVarStringArg(ModEv_OnPrivNotice, Nick, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnPrivMsg(CNick& Nick, CString& sMessage)
{ return _OnCNickAndOneVarStringArg(ModEv_OnPrivMsg, Nick, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnPrivAction(CNick& Nick, CString& sMessage)
{ return _OnCNickAndOneVarStringArg(ModEv_OnPrivAction, Nick, "message", sMessage); }


CModule::EModRet CJavaScriptMod::_OnCNickAndCChanAndOneVarStringArg(EModEvId eEventId, const CNick& Nick,
																	const CChan& Chan, const char* szPropName, CString& sArg)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		JSObject* jsoNick = pScript->MakeNickObject(&Nick);
		JSObject* jsoChan = pScript->MakeChanObject(Chan);
		JSObject* joContainer = pScript->MakeAnonObject();
		jsval jvMessage = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg));

		if(JS_SetProperty(pScript->GetContext(), joContainer, szPropName, &jvMessage))
		{
			jsval jvArgs[3];
			jvArgs[0] = OBJECT_TO_JSVAL(jsoNick);
			jvArgs[1] = OBJECT_TO_JSVAL(jsoChan);
			jvArgs[2] = OBJECT_TO_JSVAL(joContainer);
			EModRet eRet = (EModRet)pScript->InvokeEventHandler(eEventId, 3, jvArgs, true);

			jsval jvNewArg;
			if(JS_GetProperty(pScript->GetContext(), joContainer, szPropName, &jvNewArg) &&
				JSVAL_IS_STRING(jvNewArg))
			{
				sArg = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNewArg));
			}

			if(eRet != CONTINUE) return eRet;
		}
	}
	return CONTINUE;
}

CModule::EModRet CJavaScriptMod::OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage)
{ return _OnCNickAndCChanAndOneVarStringArg(ModEv_OnChanMsg, Nick, Channel, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnChanCTCP(CNick& Nick, CChan& Channel, CString& sMessage)
{ return _OnCNickAndCChanAndOneVarStringArg(ModEv_OnChanCTCP, Nick, Channel, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage)
{ return _OnCNickAndCChanAndOneVarStringArg(ModEv_OnChanAction, Nick, Channel, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnChanNotice(CNick& Nick, CChan& Channel, CString& sMessage)
{ return _OnCNickAndCChanAndOneVarStringArg(ModEv_OnChanNotice, Nick, Channel, "message", sMessage); }
CModule::EModRet CJavaScriptMod::OnTopic(CNick& Nick, CChan& Channel, CString& sTopic)
{ return _OnCNickAndCChanAndOneVarStringArg(ModEv_OnTopic, Nick, Channel, "topic", sTopic); }

void CJavaScriptMod::_OnTwoCNicksAndOneCChanAndChangedFlag(EModEvId eEventId,
														   const CNick& OpNick, const CNick& Nick, const CChan& Chan, bool bNoChange)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		JSObject* jsoOpNick = pScript->MakeNickObject(&OpNick);
		JSObject* jsoNick = pScript->MakeNickObject(&Nick);
		JSObject* jsoChan = pScript->MakeChanObject(Chan);

		jsval jvArgs[4];
		jvArgs[0] = OBJECT_TO_JSVAL(jsoOpNick);
		jvArgs[1] = OBJECT_TO_JSVAL(jsoNick);
		jvArgs[2] = OBJECT_TO_JSVAL(jsoChan);
		jvArgs[3] = BOOLEAN_TO_JSVAL(bNoChange);

		pScript->InvokeEventHandler(eEventId, 4, jvArgs, false);
	}
}

void CJavaScriptMod::OnOp(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange)
{ _OnTwoCNicksAndOneCChanAndChangedFlag(ModEv_OnOp, OpNick, Nick, Channel, bNoChange); }
void CJavaScriptMod::OnDeop(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange)
{ _OnTwoCNicksAndOneCChanAndChangedFlag(ModEv_OnDeop, OpNick, Nick, Channel, bNoChange); }
void CJavaScriptMod::OnVoice(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange)
{ _OnTwoCNicksAndOneCChanAndChangedFlag(ModEv_OnVoice, OpNick, Nick, Channel, bNoChange); }
void CJavaScriptMod::OnDevoice(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange)
{ _OnTwoCNicksAndOneCChanAndChangedFlag(ModEv_OnDevoice, OpNick, Nick, Channel, bNoChange); }


CModule::EModRet CJavaScriptMod::OnIRCConnecting(CIRCSock *pIRCSock)
{
#if 1
	InvokeNoArgScriptCallbacks(ModEv_OnIRCConnecting, false);
	// the data we try to copy below is not yet available in this callback...
#else
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnIRCConnecting))
			continue;

		JSObject* jsoSockInfo = pScript->MakeAnonObject();

		jsval jvIPv6 = BOOLEAN_TO_JSVAL(pIRCSock->GetIPv6());
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "ipv6", &jvIPv6);
		
		jsval jvSSL = BOOLEAN_TO_JSVAL(pIRCSock->GetSSL());
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "ssl", &jvSSL);

		jsval jvLocalIP = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), pIRCSock->GetLocalIP()));
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "localIP", &jvLocalIP);

		jsval jvLocalPort = INT_TO_JSVAL(pIRCSock->GetLocalPort());
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "localPort", &jvLocalPort);

		jsval jvRemoteIP = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), pIRCSock->GetRemoteIP()));
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "remoteIP", &jvRemoteIP);

		jsval jvRemotePort = INT_TO_JSVAL(pIRCSock->GetRemotePort());
		JS_SetProperty(pScript->GetContext(), jsoSockInfo, "localPort", &jvRemotePort);

		// :TODO: find out whether the GC likes what we do here...

		jsval jvArg = OBJECT_TO_JSVAL(jsoSockInfo);
		EModRet eRet = (EModRet)pScript->InvokeEventHandler(ModEv_OnIRCConnecting, 1, &jvArg, true);

		if(eRet != CONTINUE) return CONTINUE;
	}
#endif
	return CONTINUE;
}

CModule::EModRet CJavaScriptMod::OnIRCRegistration(CString& sPass, CString& sNick, CString& sIdent, CString& sRealName)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnIRCRegistration))
			continue;

		JSObject* joContainer = pScript->MakeAnonObject();
		jsval jvArgs[4];
		jvArgs[0] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sPass));
		jvArgs[1] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sNick));
		jvArgs[2] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sIdent));
		jvArgs[3] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sRealName));

		if(JS_SetProperty(pScript->GetContext(), joContainer, "pass", &jvArgs[0]) &&
			JS_SetProperty(pScript->GetContext(), joContainer, "nick", &jvArgs[1]) &&
			JS_SetProperty(pScript->GetContext(), joContainer, "ident", &jvArgs[2]) &&
			JS_SetProperty(pScript->GetContext(), joContainer, "realName", &jvArgs[3]))
		{
			jsval jvTmpCo = OBJECT_TO_JSVAL(joContainer);
			EModRet eRet = (EModRet)pScript->InvokeEventHandler(ModEv_OnIRCRegistration, 1, &jvTmpCo, true);

			jsval jvNew[4];
			if(JS_GetProperty(pScript->GetContext(), joContainer, "pass", &jvNew[0]) && JSVAL_IS_STRING(jvNew[0]) &&
				JS_GetProperty(pScript->GetContext(), joContainer, "nick", &jvNew[1]) && JSVAL_IS_STRING(jvNew[1]) &&
				JS_GetProperty(pScript->GetContext(), joContainer, "ident", &jvNew[2]) && JSVAL_IS_STRING(jvNew[2]) &&
				JS_GetProperty(pScript->GetContext(), joContainer, "realName", &jvNew[3]) && JSVAL_IS_STRING(jvNew[3]))
			{
				sPass = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNew[0]));
				sNick = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNew[1]));
				sIdent = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNew[2]));
				sRealName = CUtil::WideToUtf8(JSVAL_TO_STRING(jvNew[3]));
			}

			if(eRet != CONTINUE) return eRet;
		}
	}

	return CONTINUE;
}


void CJavaScriptMod::OnChanPermission(const CNick& OpNick, const CNick& Nick, CChan& Channel, unsigned char uMode, bool bAdded, bool bNoChange)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnChanPermission))
			continue;

		jsval jvArgs[6];
		JSObject *joOpNick = pScript->MakeNickObject(&OpNick);
		JSObject *joNick = pScript->MakeNickObject(&Nick);
		JSObject *joChan = pScript->MakeChanObject(Channel);
		CString sModeChar(uMode);

		jvArgs[0] = OBJECT_TO_JSVAL(joOpNick);
		jvArgs[1] = OBJECT_TO_JSVAL(joNick);
		jvArgs[2] = OBJECT_TO_JSVAL(joChan);
		jvArgs[3] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sModeChar));
		jvArgs[4] = BOOLEAN_TO_JSVAL(bAdded);
		jvArgs[5] = BOOLEAN_TO_JSVAL(bNoChange);

		pScript->InvokeEventHandler(ModEv_OnChanPermission, 6, jvArgs, false);
	}
}


void CJavaScriptMod::OnRawMode(const CNick& OpNick, CChan& Channel, const CString& sModes, const CString& sArgs)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnRawMode))
			continue;

		jsval jvArgs[4];
		JSObject *joOpNick = pScript->MakeNickObject(&OpNick);
		JSObject *joChan = pScript->MakeChanObject(Channel);

		jvArgs[0] = OBJECT_TO_JSVAL(joOpNick);
		jvArgs[1] = OBJECT_TO_JSVAL(joChan);
		jvArgs[2] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sModes));
		jvArgs[3] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArgs));

		pScript->InvokeEventHandler(ModEv_OnRawMode, 4, jvArgs, false);
	}
}


void CJavaScriptMod::OnMode(const CNick& OpNick, CChan& Channel, char uMode, const CString& sArg, bool bAdded, bool bNoChange)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnMode))
			continue;

		jsval jvArgs[6];
		JSObject *joOpNick = pScript->MakeNickObject(&OpNick);
		JSObject *joChan = pScript->MakeChanObject(Channel);
		CString sModeChar(uMode);

		jvArgs[0] = OBJECT_TO_JSVAL(joOpNick);
		jvArgs[1] = OBJECT_TO_JSVAL(joChan);
		jvArgs[2] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sModeChar));
		jvArgs[3] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sArg));
		jvArgs[4] = BOOLEAN_TO_JSVAL(bAdded);
		jvArgs[5] = BOOLEAN_TO_JSVAL(bNoChange);

		pScript->InvokeEventHandler(ModEv_OnMode, 6, jvArgs, false);
	}
}


void CJavaScriptMod::OnKick(const CNick& Nick, const CString& sOpNick, CChan& Channel, const CString& sMessage)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(ModEv_OnKick))
			continue;

		jsval jvArgs[4];
		JSObject *joNick = pScript->MakeNickObject(&Nick);
		JSObject *joChan = pScript->MakeChanObject(Channel);

		jvArgs[0] = OBJECT_TO_JSVAL(joNick);
		jvArgs[1] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sOpNick));
		jvArgs[2] = OBJECT_TO_JSVAL(joChan);
		jvArgs[3] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sMessage));

		pScript->InvokeEventHandler(ModEv_OnKick, 4, jvArgs, false);
	}
}


void CJavaScriptMod::_OnOneCNickAndOneCChan(EModEvId eEventId, const CNick& Nick, const CChan& Channel)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		jsval jvArgs[2];
		JSObject *joNick = pScript->MakeNickObject(&Nick);
		JSObject *joChan = pScript->MakeChanObject(Channel);

		jvArgs[0] = OBJECT_TO_JSVAL(joNick);
		jvArgs[1] = OBJECT_TO_JSVAL(joChan);

		pScript->InvokeEventHandler(eEventId, 2, jvArgs, false);
	}
}

void CJavaScriptMod::OnJoin(const CNick& Nick, CChan& Channel)
	{ _OnOneCNickAndOneCChan(ModEv_OnJoin, Nick, Channel); }

void CJavaScriptMod::OnPart(const CNick& Nick, CChan& Channel)
	{ _OnOneCNickAndOneCChan(ModEv_OnPart, Nick, Channel); }


void CJavaScriptMod::_OnOneCNickAndOneStringAndOneChanList(EModEvId eEventId,
	const CNick& Nick, const CString& sString, const vector<CChan*>& vChans)
{
	for(set<CZNCScript*>::iterator it = m_scripts.begin(); it != m_scripts.end(); it++)
	{
		CZNCScript* pScript = (*it);
		if(!pScript->IsEventHooked(eEventId))
			continue;

		jsval jvArgs[3];
		JSObject* joNick = pScript->MakeNickObject(&Nick);
		JSObject* joChans = JS_NewArrayObject(pScript->GetContext(), 0, NULL);

		jvArgs[0] = OBJECT_TO_JSVAL(joNick);
		jvArgs[1] = STRING_TO_JSVAL(CUtil::MsgCpyToJSStr(pScript->GetContext(), sString));

		// as per Notes at:
		// https://developer.mozilla.org/en/SpiderMonkey/JSAPI_Reference/JS_NewArrayObject
		JS_AddObjectRoot(pScript->GetContext(), &joChans);
		jsint i = 0;
		for(vector<CChan*>::const_iterator cit = vChans.begin(); cit != vChans.end(); cit++, i++)
		{
			jsval jvChan = OBJECT_TO_JSVAL(pScript->MakeChanObject(**cit));
			JS_SetElement(pScript->GetContext(), joChans, i, &jvChan);
		}
		jvArgs[2] = OBJECT_TO_JSVAL(joChans);

		pScript->InvokeEventHandler(eEventId, 3, jvArgs, false);

		JS_RemoveObjectRoot(pScript->GetContext(), &joChans);
	}
}

void CJavaScriptMod::OnQuit(const CNick& Nick, const CString& sMessage, const vector<CChan*>& vChans)
	{ _OnOneCNickAndOneStringAndOneChanList(ModEv_OnQuit, Nick, sMessage, vChans); }

void CJavaScriptMod::OnNick(const CNick& Nick, const CString& sNewNick, const vector<CChan*>& vChans)
	{ _OnOneCNickAndOneStringAndOneChanList(ModEv_OnNick, Nick, sNewNick, vChans); }
