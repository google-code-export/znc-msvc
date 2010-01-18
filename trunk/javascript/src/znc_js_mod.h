/*
 * Copyright (C) 2010 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef _ZNC_JS_MOD_H
#define _ZNC_JS_MOD_H

#include "znc_script.h"

class CJavaScriptMod : public CModule
{
public:
	// this will break compilation if the signature changes, but we can't use MODCONSTRUCTOR
	// here because we want to be able to include this header file from multiple source files.
	CJavaScriptMod(ModHandle pDLL, CUser* pUser, const CString& sModName, const CString& sModPath);
	virtual ~CJavaScriptMod();

	// module specific calls:
	JSRuntime* GetJSRuntime() const { return ms_jsRuntime; }

	// ZNC module call-ins:
	bool OnLoad(const CString& sArgsi, CString& sMessage);
	void OnPreRehash();
	void OnPostRehash();
	void OnIRCDisconnected();
	void OnIRCConnected();
	EModRet OnIRCConnecting(CIRCSock *pIRCSock);
	EModRet OnIRCRegistration(CString& sPass, CString& sNick, CString& sIdent, CString& sRealName);
	EModRet OnBroadcast(CString& sMessage);

	void OnChanPermission(const CNick& OpNick, const CNick& Nick, CChan& Channel, unsigned char uMode, bool bAdded, bool bNoChange);
	void OnOp(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange);
	void OnDeop(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange);
	void OnVoice(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange);
	void OnDevoice(const CNick& OpNick, const CNick& Nick, CChan& Channel, bool bNoChange);
	void OnRawMode(const CNick& OpNick, CChan& Channel, const CString& sModes, const CString& sArgs);
	void OnMode(const CNick& OpNick, CChan& Channel, char uMode, const CString& sArg, bool bAdded, bool bNoChange);

	EModRet OnRaw(CString& sLine);

	EModRet OnStatusCommand(CString& sCommand);
	void OnModCommand(const CString& sCommand); // forwarded to JS mods via OnUserRaw
#if 0 // entirely handled via OnUserRaw:
	void OnModNotice(const CString& sMessage);
	void OnModCTCP(const CString& sMessage);
#endif

	void OnQuit(const CNick& Nick, const CString& sMessage, const vector<CChan*>& vChans);
	void OnNick(const CNick& Nick, const CString& sNewNick, const vector<CChan*>& vChans);
	void OnKick(const CNick& Nick, const CString& sOpNick, CChan& Channel, const CString& sMessage);
	void OnJoin(const CNick& Nick, CChan& Channel);
	void OnPart(const CNick& Nick, CChan& Channel);

	void OnClientLogin();
	void OnClientDisconnect();
	EModRet OnUserRaw(CString& sLine);
	EModRet OnUserCTCPReply(CString& sTarget, CString& sMessage);
	EModRet OnUserCTCP(CString& sTarget, CString& sMessage);
	EModRet OnUserAction(CString& sTarget, CString& sMessage);
	EModRet OnUserMsg(CString& sTarget, CString& sMessage);
	EModRet OnUserNotice(CString& sTarget, CString& sMessage);
	EModRet OnUserJoin(CString& sChannel, CString& sKey);
	EModRet OnUserPart(CString& sChannel, CString& sMessage);
	EModRet OnUserTopic(CString& sChannel, CString& sTopic);
	EModRet OnUserTopicRequest(CString& sChannel);

	EModRet OnCTCPReply(CNick& Nick, CString& sMessage);
	EModRet OnPrivCTCP(CNick& Nick, CString& sMessage);
	EModRet OnPrivAction(CNick& Nick, CString& sMessage);
	EModRet OnPrivMsg(CNick& Nick, CString& sMessage);
	EModRet OnPrivNotice(CNick& Nick, CString& sMessage);
	EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage);
	EModRet OnChanCTCP(CNick& Nick, CChan& Channel, CString& sMessage);
	EModRet OnChanAction(CNick& Nick, CChan& Channel, CString& sMessage);
	EModRet OnChanNotice(CNick& Nick, CChan& Channel, CString& sMessage);
	EModRet OnTopic(CNick& Nick, CChan& Channel, CString& sTopic);

protected:
	set<CZNCScript*> m_scripts;

	static JSRuntime* ms_jsRuntime;
	static int ms_uNumberOfInstances;

	// module call-in wrappers:
	EModRet InvokeNoArgScriptCallbacks(EModEvId eEvent, bool bModRet);
	EModRet _OnOneVariableStringArg(EModEvId, const char*, CString&);
	EModRet _OnTwoVariableStringArgs(EModEvId, const char*, const char*, CString&, CString&);
	void _OnOneConstStringArg(EModEvId, const CString&);
	EModRet _OnCNickAndOneVarStringArg(EModEvId, const CNick&, const char*, CString&);
	EModRet _OnCNickAndCChanAndOneVarStringArg(EModEvId, const CNick&, const CChan&, const char*, CString&);
	void _OnTwoCNicksAndOneCChanAndChangedFlag(EModEvId, const CNick&, const CNick&, const CChan&, bool);
	void _OnOneCNickAndOneCChan(EModEvId, const CNick&, const CChan&);
	void _OnOneCNickAndOneStringAndOneChanList(EModEvId, const CNick&, const CString&, const vector<CChan*>& vChans);
};

#endif /* !_ZNC_JS_MOD_H */
