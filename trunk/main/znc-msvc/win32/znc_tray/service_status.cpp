/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include "service_status.hpp"

#pragma comment(lib, "Advapi32.lib")


CServiceStatus::CServiceStatus(const wchar_t* a_serviceName) :
	m_serviceName(a_serviceName),
	m_scm(NULL), m_hService(NULL)
{
}


bool CServiceStatus::Init()
{
	if(m_scm)
	{
		// already initialized
		return true;
	}

	m_scm = ::OpenSCManager(NULL, NULL, STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE);

	return (!!m_scm);
}


bool CServiceStatus::IsInstalled()
{
	if(this->OpenService())
	{
		this->CloseService();

		return true;
	}

	return false;
}


bool CServiceStatus::OpenService()
{
	if(m_hService)
	{
		return true;
	}

	m_hService = ::OpenService(m_scm, m_serviceName, STANDARD_RIGHTS_READ);

	return (m_hService != NULL);
}


void CServiceStatus::CloseService()
{
	if(m_hService)
	{
		::CloseServiceHandle(m_hService);
		m_hService = NULL;
	}
}


CServiceStatus::~CServiceStatus()
{
	this->CloseService();

	if(m_scm)
	{
		::CloseServiceHandle(m_scm);
	}
}
