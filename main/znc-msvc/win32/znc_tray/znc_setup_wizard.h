/*
 * Copyright (C) 2012 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#pragma once

class CZNCSetupWizard
{
public:
	
	static std::wstring GetServiceConfDirPath();
	static std::wstring GetServiceZncConfPath();
	static bool DoesServiceConfigExist();

protected:
	
};

class CInitialZncConf
{
public:
	CInitialZncConf();

	bool GotFreePort() const { return m_port != 0; }

	const std::string GetWebUrl() const;
	const std::string GetAdminPass() const;

	const std::string GetZncConf() const;
	bool WriteZncConf(const std::wstring a_path) const;

protected:
	unsigned short m_port;
	std::string m_adminPass;

	static unsigned short FindFreeListenerPort();
	static std::string GenerateRandomPassword(size_t a_length = 10);
};
