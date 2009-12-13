/*
* Copyright (C) 2009 Ingmar Runge
* See the AUTHORS file for details.
*
* An ident server module for Windows ZNC.
* http://code.google.com/p/znc-msvc/
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published
* by the Free Software Foundation.
*/

#ifdef WIN32
#include "stdafx.hpp"
#endif
#include "znc.h"
#include "User.h"
#include "IRCSock.h"
#include "Modules.h"

/************************************************************************/
/*   CLASS DECLARATIONS                                                 */
/************************************************************************/
class CIdentServer;

class CIdentServerMod : public CGlobalModule
{
protected:
	unsigned short m_serverPort;
	CIdentServer *m_identServer;
	bool m_listenFailed;
	CString m_sLastReply;
	CString m_sLastRequest;

public:
	GLOBALMODCONSTRUCTOR(CIdentServerMod)
	{
		m_serverPort = 113;
		m_identServer = NULL;
		m_listenFailed = false;
	}
	virtual ~CIdentServerMod();

	EModRet OnIRCConnecting(CIRCSock *pIRCSock);
	void OnIRCConnected();
	void NoLongerNeedsIdentServer();
	void OnIRCDisconnected();
	void OnClientLogin();
	EModRet OnDeleteUser(CUser& User);
	void OnModCommand(const CString& sLine);

	CIdentServer *GetIdentServer() { return m_identServer; }

	void SetLastReply(const CString& s) { m_sLastReply = s; };
	void SetLastRequest(const CString& s) { m_sLastRequest = s; };
};


class CIdentAcceptedSocket : public CSocket
{
public:
	CIdentAcceptedSocket(CModule *pMod);
	virtual ~CIdentAcceptedSocket();

	void ReadLine(const CS_STRING & sLine);
};


/**
* Ident server implementation.
* RFC 1413: http://www.faqs.org/rfcs/rfc1413.html
* Not thread safe!
**/
class CIdentServer : public CSocket
{
protected:
	set<CUser*> m_activeUsers;
	CModule *m_pModule;
	unsigned short m_uPort;

public:
	CIdentServer(CModule *pMod, unsigned short uPort);
	virtual ~CIdentServer();

	bool IncreaseUseCount(CUser *pUser);
	bool DecreaseUseCount(CUser *pUser);
	bool InUse() { return !m_activeUsers.empty(); }
	bool StartListening();

	Csock *GetSockObj(const CS_STRING & sHostname, u_short uPort);
	bool ConnectionFrom(const CS_STRING & sHostname, u_short uPort);

	CString GetResponse(const CString& sLine, const CString& sSocketIP, const CString& sRemoteIP);
	const set<CUser*>& GetActiveUsers() { return m_activeUsers; };
};


/************************************************************************/
/* CIdentServer method implementation section                           */
/************************************************************************/

CIdentServer::CIdentServer(CModule *pMod, unsigned short uPort) : CSocket(pMod)
{
	m_pModule = pMod;
	m_uPort = uPort;
}

bool CIdentServer::IncreaseUseCount(CUser *pUser)
{
	if(m_activeUsers.find(pUser) != m_activeUsers.end())
	{
		return false;
	}

	m_activeUsers.insert(pUser);

	return true;
}

bool CIdentServer::DecreaseUseCount(CUser *pUser)
{
	return (m_activeUsers.erase(pUser) != 0);
}

CString CIdentServer::GetResponse(const CString& sLine, const CString& sSocketIP, const CString& sRemoteIP)
{
	unsigned short uLocalPort = 0; // local port that ZNC connected to IRC FROM
	unsigned short uRemotePort = 0; // remote server port that ZNC connected TO, e.g. 6667

	CString sResponseType = "ERROR";
	CString sAddInfo = "INVALID-PORT";

	DEBUG("IDENT request: " << sLine << " from " << sRemoteIP << " on " << sSocketIP);

	if(sscanf(sLine.c_str(), "%hu , %hu", &uLocalPort, &uRemotePort) == 2)
	{
		sAddInfo = "NO-USER";

		const map<CString, CUser*>& msUsers = CZNC::Get().GetUserMap();
		for(map<CString, CUser*>::const_iterator it = msUsers.begin(); it != msUsers.end(); it++)
		{
			CUser* pUser = it->second;
			CIRCSock *pSock = it->second->GetIRCSock();

			if(!pSock)
				continue;

			DEBUG("Checking user (" << pSock->GetLocalPort() << ", " << pSock->GetRemotePort() << ", " << pSock->GetLocalIP() << ")");

			if(pSock->GetLocalPort() == uLocalPort &&
				pSock->GetRemotePort() == uRemotePort &&
				pSock->GetLocalIP() == sSocketIP)
			{
				sResponseType = "USERID";
				sAddInfo = "UNIX : " + pUser->GetIdent();
				break; /* exact match found, leave the loop */
			}

			DEBUG("Checking user fallback (" << pSock->GetRemoteIP() << ", " << pSock->GetRemotePort() << ", " << pSock->GetLocalIP() << ")");

			if(pSock->GetRemoteIP() == sRemoteIP &&
				pSock->GetRemotePort() == uRemotePort &&
				pSock->GetLocalIP() == sSocketIP)
			{
				sResponseType = "USERID";
				sAddInfo = "UNIX : " + pUser->GetIdent();
				/* stay in the loop, we may find something better */
			}
		}
	}

	CString sReply = CString(uLocalPort) + ", " + CString(uRemotePort) + " : " + sResponseType + " : " + sAddInfo;

	DEBUG("IDENT response: " << sReply);

	CIdentServerMod *pMod = reinterpret_cast<CIdentServerMod*>(m_pModule);
	if(pMod)
	{
		pMod->SetLastRequest(sLine.Replace_n("\r", "").Replace_n("\n", " ") + "from " + sRemoteIP + " on " + sSocketIP);
		pMod->SetLastReply(sReply);
	}

	return sReply;
}

bool CIdentServer::StartListening()
{
	return GetModule()->GetManager()->ListenAll(m_uPort, "IDENT_SERVER", false, SOMAXCONN, this);
}

Csock *CIdentServer::GetSockObj(const CS_STRING & sHostname, u_short uPort)
{
	return new CIdentAcceptedSocket(m_pModule);
}

bool CIdentServer::ConnectionFrom(const CS_STRING & sHostname, u_short uPort)
{
	DEBUG("IDENT connection from " << sHostname << ":" << uPort << " (on " << GetLocalIP() << ":" << GetLocalPort() << ")");

	return (!m_activeUsers.empty());
}

CIdentServer::~CIdentServer()
{
}


/************************************************************************/
/* CIdentAcceptedSocket method implementation section                   */
/************************************************************************/

CIdentAcceptedSocket::CIdentAcceptedSocket(CModule *pMod) : CSocket(pMod)
{
	EnableReadLine();
}

void CIdentAcceptedSocket::ReadLine(const CS_STRING & sLine)
{
	CIdentServerMod *pMod = reinterpret_cast<CIdentServerMod*>(m_pModule);
	const CString sReply = pMod->GetIdentServer()->GetResponse(sLine, GetLocalIP(), GetRemoteIP());

	Write(sReply + "\r\n");

	Close(CLT_AFTERWRITE);
}

CIdentAcceptedSocket::~CIdentAcceptedSocket()
{
}


/************************************************************************/
/* CIdentServerMod method implementation section                        */
/************************************************************************/

CIdentServerMod::EModRet CIdentServerMod::OnIRCConnecting(CIRCSock *pIRCSock)
{
	assert(m_pUser != NULL);

	DEBUG("CIdentServerMod::OnIRCConnecting");

	if(!m_identServer)
	{
		DEBUG("Starting up IDENT listener.");
		m_identServer = new CIdentServer(this, m_serverPort);

		if(!m_identServer->StartListening())
		{
			DEBUG("WARNING: Opening the listening socket failed!");
			m_listenFailed = true;
			m_identServer = NULL; /* Csock deleted the instance. (gross) */
			return CONTINUE;
		}
		else
		{
			m_listenFailed = false;
		}
	}

	m_identServer->IncreaseUseCount(m_pUser);

	return CONTINUE;
}

void CIdentServerMod::NoLongerNeedsIdentServer()
{
	assert(m_pUser != NULL);

	if(m_identServer)
	{
		m_identServer->DecreaseUseCount(m_pUser);

		if(!m_identServer->InUse())
		{
			DEBUG("Closing down IDENT listener.");
			m_identServer->Close();
			m_identServer = NULL;
		}
	}
}

void CIdentServerMod::OnIRCConnected()
{
	if((!m_pClient) && (m_listenFailed))
	{
		PutModule("*** WARNING: Opening the listening socket failed!");
		PutModule("*** IDENT listener is NOT running.");
	}
	NoLongerNeedsIdentServer();
}

void CIdentServerMod::OnIRCDisconnected()
{
	NoLongerNeedsIdentServer();
}

CIdentServerMod::EModRet CIdentServerMod::OnDeleteUser(CUser& User)
{
	// NoLongerNeedsIdentServer needs m_pUser,
	// so let's set it:
	CUser *pTmp = m_pUser;
	m_pUser = &User;
	// thanks to psychon for figuring this out!

	NoLongerNeedsIdentServer();

	// ident server business is done for this user now.
	m_pUser = pTmp;

	return CONTINUE;
}

void CIdentServerMod::OnClientLogin()
{
	if(m_listenFailed)
	{
		PutModule("*** WARNING: Opening the listening socket failed!");
		PutModule("*** IDENT listener is NOT running.");
	}
}

void CIdentServerMod::OnModCommand(const CString& sLine)
{
	CString sCommand = sLine.Token(0);

	if(sCommand.Equals("HELP"))
	{
		CTable Table;
		Table.AddColumn("Command");
		Table.AddColumn("Description");

		Table.AddRow();
		Table.SetCell("Command", "Status");
		Table.SetCell("Description", "Displays status information about IdentServer");

		PutModule(Table);
		return;
	}
	else if(sCommand.Equals("STATUS"))
	{
		if(m_identServer)
		{
			PutModule("IdentServer is listening on: " + m_identServer->GetLocalIP() + ":" + CString(m_serverPort));

			if(m_pUser->IsAdmin())
			{
				PutModule("List of active users:");

				const set<CUser*> Users = m_identServer->GetActiveUsers();
				for(set<CUser*>::const_iterator it = Users.begin(); it != Users.end(); it++)
					PutModule("* " + (*it)->GetCleanUserName());
			}
		}
		else
		{
			if(m_listenFailed)
			{
				PutModule("WARNING: Opening the listening socket failed!");
			}
			PutModule("IdentServer isn't listening.");
		}
		if(m_pUser->IsAdmin())
		{
			PutModule("Last IDENT request: " + m_sLastRequest);
			PutModule("Last IDENT reply: " + m_sLastReply);
		}
	}
	else
	{
		PutModule("Unknown command [" + sCommand + "] try 'Help'");
	}
}

CIdentServerMod::~CIdentServerMod()
{
	if(m_identServer)
	{
		m_identServer->Close();
	}
}


GLOBALMODULEDEFS(CIdentServerMod, "Provides a simple IDENT server implementation.")
