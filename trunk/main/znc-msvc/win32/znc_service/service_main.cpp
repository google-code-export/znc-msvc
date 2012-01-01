#include "stdafx.hpp"
#include "znc_service.h"
#include <getopt.h>

static const struct option g_LongOpts[] = {
	{ "help",		no_argument,	0,	'h' },
	{ "install",	no_argument,	0,	'i' },
	{ "manual",		no_argument,	0,	'm' },
	{ "uninstall",	no_argument,	0,	'u' },
	{ "datadir",	required_argument, 0, 'd' },
	{ 0, 0, 0, 0 }
};

static void GenerateHelp(const char *appname) {
	CUtils::PrintMessage("USAGE: " + CString(appname) + " [commands / options]");
	CUtils::PrintMessage("Commands are:");
	CUtils::PrintMessage("\t-h, --help         List available command line options (this page)");
	CUtils::PrintMessage("\t-i, --install      Install ZNC service.");
	CUtils::PrintMessage("\t-h, --uninstall    Uninstall ZNC service");
	CUtils::PrintMessage("The only option is:");
	CUtils::PrintMessage("\t-d, --datadir      Set a different znc repository (default is ~/.znc)\n");
	CUtils::PrintMessage("If there's no .znc directory in ZNC's directory, you have to specify its path with --datadir");
}

// ToDo: implement --install and -- uninstall functions

int main(int argc, char *argv[])
{
	CZNCWindowsService Svc;

	CUtils::SeedPRNG();
	CDebug::SetStdoutIsTTY(false);

	// process command line options first, only accept a single command,
	// for example "--install" and "--uninstall" at the same time is meaningless 

	int iArg, iOptIndex = -1;
	enum { cmdNone, cmdErr, cmdHelp, cmdInstall, cmdUninstall } Command = cmdNone;
	bool bManualStartupInstall = false;

	while ((iArg = getopt_long(argc, argv, "hiumd:", g_LongOpts, &iOptIndex)) != -1) {
		switch (iArg) {
		case 'h':
			Command = cmdHelp;
			break;
		case 'i':
			if (Command == cmdNone)
				Command = cmdInstall;
			else
				Command = cmdErr;
			break;
		case 'u':
			if (Command == cmdNone)
				Command = cmdUninstall;
			else
				Command = cmdErr;
			break;
		case 'd':
			Svc.SetDataDir(optarg);
			break;
		case 'm':
			bManualStartupInstall = true;
			break;
		}
	}

	SERVICE_TABLE_ENTRYW ServiceTable[] = {
		{ZNC_SERVICE_NAME, Svc.ServiceMain},
		{NULL, NULL}
	};
	// it was started by the Windows Service Control Manager
	if (StartServiceCtrlDispatcherW(ServiceTable) != NULL)
		return 0;
	else if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
		// it was started as a console app
		switch (Command) {
		case cmdHelp:
			GenerateHelp(argv[0]);
			return 0;
		case cmdInstall:
			if(CZNCWindowsService::InstallService(bManualStartupInstall) == 0)
			{
				std::cout << "OK" << std::endl;
				return 0;
			}
			else
			{
				std::cerr << "Error" << std::endl;
				return 1;
			}
			break;
		case cmdUninstall:
			if(CZNCWindowsService::UninstallService() == 0)
			{
				std::cout << "OK" << std::endl;
				return 0;
			}
			else
			{
				std::cerr << "Error" << std::endl;
				return 1;
			}
			break;

		// unknown command or none specified
		case cmdErr:
			CUtils::PrintMessage("There are conflicting options: --install and --uninstall\n");
		case cmdNone:
			GenerateHelp(argv[0]);
			return 1;
		}
	}
	return 0;
}
