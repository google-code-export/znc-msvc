/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#ifndef SOCKET_H
#define SOCKET_H

#include "Csocket.h"

class CModule;

class ZNC_API CZNCSock : public Csock {
public:
	CZNCSock(int timeout = 60) : Csock(timeout) {}
	CZNCSock(const CString& sHost, u_short port, int timeout = 60) : Csock(sHost, port, timeout) {}
	~CZNCSock() {}
};

class ZNC_API CSockManager : public TSocketManager<CZNCSock> {
public:
	CSockManager() {}
	virtual ~CSockManager() {}

	bool ListenHost(u_short iPort, const CString& sSockName, const CString& sBindHost, bool bSSL = false, int iMaxConns = SOMAXCONN, CZNCSock *pcSock = NULL, u_int iTimeout = 0, bool bIsIPv6 = false) {
		CSListener L(iPort, sBindHost);

		L.SetSockName(sSockName);
		L.SetIsSSL(bSSL);
		L.SetTimeout(iTimeout);
		L.SetMaxConns(iMaxConns);

#ifdef HAVE_IPV6
		if (bIsIPv6) {
			L.SetAFRequire(CSSockAddr::RAF_INET6);
		}
#endif

		return Listen(L, pcSock);
	}

	bool ListenAll(u_short iPort, const CString& sSockName, bool bSSL = false, int iMaxConns = SOMAXCONN, CZNCSock *pcSock = NULL, u_int iTimeout = 0, bool bIsIPv6 = false) {
		return ListenHost(iPort, sSockName, "", bSSL, iMaxConns, pcSock, iTimeout, bIsIPv6);
	}

	u_short ListenRand(const CString& sSockName, const CString& sBindHost, bool bSSL = false, int iMaxConns = SOMAXCONN, CZNCSock *pcSock = NULL, u_int iTimeout = 0, bool bIsIPv6 = false) {
		unsigned short uPort = 0;
		CSListener L(0, sBindHost);

		L.SetSockName(sSockName);
		L.SetIsSSL(bSSL);
		L.SetTimeout(iTimeout);
		L.SetMaxConns(iMaxConns);

#ifdef HAVE_IPV6
		if (bIsIPv6) {
			L.SetAFRequire(CSSockAddr::RAF_INET6);
		}
#endif

		Listen(L, pcSock, &uPort);

		return uPort;
	}

	u_short ListenAllRand(const CString& sSockName, bool bSSL = false, int iMaxConns = SOMAXCONN, CZNCSock *pcSock = NULL, u_int iTimeout = 0, bool bIsIPv6 = false) {
		return(ListenRand(sSockName, "", bSSL, iMaxConns, pcSock, iTimeout, bIsIPv6));
	}

	bool Connect(const CString& sHostname, u_short iPort , const CString& sSockName, int iTimeout = 60, bool bSSL = false, const CString& sBindHost = "", CZNCSock *pcSock = NULL) {
		CSConnection C(sHostname, iPort, iTimeout);

		C.SetSockName(sSockName);
		C.SetIsSSL(bSSL);
		C.SetBindHost(sBindHost);

		return TSocketManager<CZNCSock>::Connect(C, pcSock);
	}

	unsigned int GetAnonConnectionCount(const CString &sIP) const;
private:
protected:
};

class ZNC_API CSocket : public CZNCSock {
public:
	CSocket(CModule* pModule);
	CSocket(CModule* pModule, const CString& sHostname, unsigned short uPort, int iTimeout = 60);
	virtual ~CSocket();

	using Csock::Connect;
	using Csock::Listen;

	// This defaults to closing the socket, feel free to override
	virtual void ReachedMaxBuffer();
	virtual void SockError(int iErrno);
	// This limits the global connections from this IP to defeat DoS
	// attacks, feel free to override
	virtual bool ConnectionFrom(const CString& sHost, unsigned short uPort);

	bool Connect(const CString& sHostname, unsigned short uPort, bool bSSL = false, unsigned int uTimeout = 60);
	bool Listen(unsigned short uPort, bool bSSL = false, unsigned int uTimeout = 0);
	virtual bool PutIRC(const CString& sLine);
	virtual bool PutUser(const CString& sLine);
	virtual bool PutStatus(const CString& sLine);
	virtual bool PutModule(const CString& sLine, const CString& sIdent = "", const CString& sHost = "znc.in");
	virtual bool PutModNotice(const CString& sLine, const CString& sIdent = "", const CString& sHost = "znc.in");

	// Setters
	void SetModule(CModule* p);
	// !Setters

	// Getters
	CModule* GetModule() const;
	// !Getters
private:
protected:
	CModule*	m_pModule;
};

#endif /* SOCKET_H */
