/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

class CServiceStatus
{
public:
	CServiceStatus(const wchar_t* a_serviceName);
	virtual ~CServiceStatus();

	bool Init();
	bool IsInstalled();
protected:
	const wchar_t* m_serviceName;
	SC_HANDLE m_scm;
	SC_HANDLE m_hService;

	bool OpenService();
	void CloseService();
};
