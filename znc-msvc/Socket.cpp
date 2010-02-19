/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "Modules.h"
#include "User.h"
#include "znc.h"

unsigned int CSockManager::GetAnonConnectionCount(const CString &sIP) const {
	const_iterator it;
	unsigned int ret = 0;

	for (it = begin(); it != end(); it++) {
		CZNCSock *pSock = *it;
		// Logged in CClients have "USR::<username>" as their sockname
		if (pSock->GetRemoteIP() == sIP && pSock->GetSockName().Left(5) != "USR::") {
			ret++;
		}
	}

	DEBUG("There are [" << ret << "] clients from [" << sIP << "]");

	return ret;
}

#ifdef _MODULES

/////////////////// CSocket ///////////////////
CSocket::CSocket(CModule* pModule) : CZNCSock() {
	m_pModule = pModule;
	m_pModule->AddSocket(this);
	EnableReadLine();
	SetMaxBufferThreshold(10240);
}

CSocket::CSocket(CModule* pModule, const CString& sHostname, unsigned short uPort, int iTimeout) : CZNCSock(sHostname, uPort, iTimeout) {
	m_pModule = pModule;
	m_pModule->AddSocket(this);
	EnableReadLine();
	SetMaxBufferThreshold(10240);
}

CSocket::~CSocket() {
	CUser *pUser = m_pModule->GetUser();

	m_pModule->UnlinkSocket(this);

	if (!m_pModule->IsGlobal() && pUser) {
		pUser->AddBytesWritten(GetBytesWritten());
		pUser->AddBytesRead(GetBytesRead());
	} else {
		CZNC::Get().AddBytesWritten(GetBytesWritten());
		CZNC::Get().AddBytesRead(GetBytesRead());
	}
}

void CSocket::ReachedMaxBuffer() {
	DEBUG(GetSockName() << " == ReachedMaxBuffer()");
	PutModule("Some socket reached its max buffer limit and was closed!");
	Close();
}

void CSocket::SockError(int iErrno) {
	DEBUG(GetSockName() << " == SockError(" << strerror(iErrno) << ")");
	if (iErrno == EMFILE) {
		// We have too many open fds, this can cause a busy loop.
		Close();
	}
}

bool CSocket::ConnectionFrom(const CString& sHost, unsigned short uPort) {
	return CZNC::Get().AllowConnectionFrom(sHost);
}

bool CSocket::Connect(const CString& sHostname, unsigned short uPort, bool bSSL, unsigned int uTimeout) {
	CUser* pUser = m_pModule->GetUser();
	CString sSockName = "MOD::C::" + m_pModule->GetModName();
	CString sVHost;

	if (pUser) {
		sSockName += "::" + pUser->GetUserName();
		sVHost = m_pModule->GetUser()->GetVHost();
	}

	// Don't overwrite the socket name if one is already set
	if (!GetSockName().empty()) {
		sSockName = GetSockName();
	}

	return m_pModule->GetManager()->Connect(sHostname, uPort, sSockName, uTimeout, bSSL, sVHost, this);
}

bool CSocket::Listen(unsigned short uPort, bool bSSL, unsigned int uTimeout) {
	CUser* pUser = m_pModule->GetUser();
	CString sSockName = "MOD::L::" + m_pModule->GetModName();

	if (pUser) {
		sSockName += "::" + pUser->GetUserName();
	}
	// Don't overwrite the socket name if one is already set
	if (!GetSockName().empty()) {
		sSockName = GetSockName();
	}

	return m_pModule->GetManager()->ListenAll(uPort, sSockName, bSSL, SOMAXCONN, this);
}

bool CSocket::PutIRC(const CString& sLine) {
	return (m_pModule) ? m_pModule->PutIRC(sLine) : false;
}

bool CSocket::PutUser(const CString& sLine) {
	return (m_pModule) ? m_pModule->PutUser(sLine) : false;
}

bool CSocket::PutStatus(const CString& sLine) {
	return (m_pModule) ? m_pModule->PutStatus(sLine) : false;
}

bool CSocket::PutModule(const CString& sLine, const CString& sIdent, const CString& sHost) {
	return (m_pModule) ? m_pModule->PutModule(sLine, sIdent, sHost) : false;
}
bool CSocket::PutModNotice(const CString& sLine, const CString& sIdent, const CString& sHost) {
	return (m_pModule) ? m_pModule->PutModNotice(sLine, sIdent, sHost) : false;
}

void CSocket::SetModule(CModule* p) { m_pModule = p; }
CModule* CSocket::GetModule() const { return m_pModule; }
/////////////////// !CSocket ///////////////////

#endif // _MODULES
