/*
 * Copyright (C) 2004-2009  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifdef _MODULES

#ifndef _HTTPSOCK_H
#define _HTTPSOCK_H

#include "Modules.h"

class ZNC_API CHTTPSock : public CSocket {
public:
	CHTTPSock(CModule *pMod);
	CHTTPSock(CModule *pMod, const CString& sHostname, unsigned short uPort, int iTimeout = 60);
	virtual ~CHTTPSock();

	// Csocket derived members
	virtual void ReadData(const char* data, int len);
	virtual void ReadLine(const CString& sData);
	virtual void ReachedMaxBuffer();
	virtual void SockError(int iErrno);
	virtual void Timeout();
	virtual void Connected();
	virtual void Disconnected();
	virtual Csock* GetSockObj(const CString& sHost, unsigned short uPort) = 0;
	// !Csocket derived members

	// Hooks
	virtual bool OnPageRequest(const CString& sURI, CString& sPageRet) = 0;
	virtual bool OnLogin(const CString& sUser, const CString& sPass);
	// !Hooks

	void CheckPost();
	bool SentHeader() const;
	bool PrintHeader(off_t uContentLength, const CString& sContentType = "", unsigned int uStatusId = 200, const CString& sStatusMsg = "OK");
	void AddHeader(const CString& sName, const CString& sValue);
	void SetContentType(const CString& sContentType);

	bool PrintNotFound();
	bool Redirect(const CString& sURL);
	bool ForceLogin();
	CString GetErrorPage(unsigned int uStatusId, const CString& sStatusMsg, const CString& sMessage);
	bool PrintErrorPage(unsigned int uStatusId, const CString& sStatusMsg, const CString& sMessage);
	void ParseParams(const CString& sParams);
	void ParseURI();
	void GetPage();
	bool PrintFile(const CString& sFileName, CString sContentType = "");

	// Setters
	void SetDocRoot(const CString& s);
	void SetLoggedIn(bool b) { m_bLoggedIn = b; }
	// !Setters

	// Getters
	bool HasParam(const CString& sName) const;
	CString GetRawParam(const CString& sName) const;
	CString GetParam(const CString& sName, const CString& sFilter = "\r\n") const;
	bool IsLoggedIn() const { return m_bLoggedIn; }
	const CString& GetDocRoot() const;
	const CString& GetUser() const;
	const CString& GetPass() const;
	const CString& GetParamString() const;
	const CString& GetContentType() const;
	size_t GetParamValues(const CString& sName, VCString& vsRet, const CString& sFilter = "\r\n") const;
	size_t GetParamValues(const CString& sName, set<CString>& ssRet, const CString& sFilter = "\r\n") const;
	const map<CString, VCString>& GetParams() const;
	// !Getters
private:
protected:
	void Init();

	bool					m_bSentHeader;
	bool					m_bGotHeader;
	bool					m_bLoggedIn;
	bool					m_bPost;
	bool					m_bDone;
	unsigned long			m_uPostLen;
	CString					m_sPostData;
	CString					m_sURI;
	CString					m_sUser;
	CString					m_sPass;
	CString					m_sContentType;
	CString					m_sDocRoot;
	map<CString, VCString>	m_msvsParams;
	MCString				m_msHeaders;
	bool					m_bHTTP10Client;
	CString					m_sIfNoneMatch;
};

#endif // !_HTTPSOCK_H

#endif // _MODULES
