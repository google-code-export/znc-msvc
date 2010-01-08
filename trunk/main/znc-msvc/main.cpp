/*
 * Copyright (C) 2004-2010  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "stdafx.hpp"
#include <getopt.h>
#include <sys/wait.h>

static const struct option g_LongOpts[] = {
	{ "help",			no_argument,	0,	'h' },
	{ "version",			no_argument,	0,	'v' },
	{ "debug",			no_argument,	0,	'D' },
	{ "makeconf",			no_argument,	0,	'c' },
	{ "makepass",			no_argument,	0,	's' },
#ifdef HAVE_LIBSSL
	{ "makepem",			no_argument,	0,	'p' },
#endif /* HAVE_LIBSSL */
	{ "datadir",                    required_argument,	0,   'd' },
	{ 0, 0, 0, 0 }
};

static void GenerateHelp(const char *appname) {
	CUtils::PrintMessage("USAGE: " + CString(appname) + " [options] [config]");
	CUtils::PrintMessage("Options are:");
	CUtils::PrintMessage("\t-h, --help         List available command line options (this page)");
	CUtils::PrintMessage("\t-v, --version      Output version information and exit");
	CUtils::PrintMessage("\t-D, --debug        Output debugging information (Implies -f)");
	CUtils::PrintMessage("\t-c, --makeconf     Interactively create a new config");
	CUtils::PrintMessage("\t-s, --makepass     Generates a password for use in config");
#ifdef HAVE_LIBSSL
	CUtils::PrintMessage("\t-p, --makepem      Generates a pemfile for use with SSL");
#endif /* HAVE_LIBSSL */
	CUtils::PrintMessage("\t-d, --datadir      Set a different znc repository (default is ~/.znc)");
}

// removed: die() rehash() isRoot()

// false => exit main select() loop.
bool g_bMainLoop = true;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		g_bMainLoop = false;
		return TRUE;
	default:
		return FALSE;
	}
}


int main(int argc, char** argv) {
	CString sConfig;
	CString sDataDir = "";

	srand((unsigned int)time(NULL));
	// Win32 doesn't support shell escape codes, so we do this.
	CUtils::SetStdoutIsTTY(false);

	CString sConsoleTitle = "ZNC " + CZNC::GetVersion();
	SetConsoleTitle(sConsoleTitle.c_str());

	// register Ctrl handler
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE) == NULL)
	{
		CUtils::PrintMessage("Couldn't register console Ctrl handler function!");
	}

	// this prevents open()/read() and friends from stripping \r
	// from files... simply adding _O_BINARY to the modes doesn't seem
	// to be enough for some reason...
	// if we don't do this, Template.cpp will break
	// because it uses string.size() for file pos
	// calculations.
	_set_fmode(_O_BINARY);

#ifdef HAVE_LIBSSL
	CRYPTO_malloc_init();
#endif

	// make sure the stuff in ZNC.dll matches this exe's version... bad crashes otherwise.
	if(CZNC::GetCoreVersion() != MODVERSION)
	{
		CUtils::PrintError("The version number in ZNC.dll doesn't match. Aborting.");
		return 1;
	}

	// process command line arguments

	int iArg, iOptIndex = -1;
	bool bMakeConf = false;
	bool bMakePass = false;
	bool bAllowRoot = false;

#ifdef HAVE_LIBSSL
	bool bMakePem = false;

	while ((iArg = getopt_long(argc, argv, "hvcspd:D", g_LongOpts, &iOptIndex)) != -1) {
#else
	while ((iArg = getopt_long(argc, argv, "hvcsd:D", g_LongOpts, &iOptIndex)) != -1) {
#endif /* HAVE_LIBSSL */
		switch (iArg) {
		case 'h':
			GenerateHelp(argv[0]);
			return 0;
		case 'v':
			cout << CZNC::GetTag() << endl;
			return 0;
		case 'c':
			bMakeConf = true;
			break;
		case 's':
			bMakePass = true;
			break;
#ifdef HAVE_LIBSSL
		case 'p':
			bMakePem = true;
			break;
#endif /* HAVE_LIBSSL */
		case 'd':
			sDataDir = CString(optarg);
			break;
		case 'D':
			CUtils::SetDebug(true);
			break;
		case '?':
		default:
			GenerateHelp(argv[0]);
			return 1;
		}
	}

	if (optind < argc) {
		sConfig = argv[optind];
	}

	if (!InitCsocket()) {
		CUtils::PrintError("Failed to initialize Csocket!");
		exit(-1);
	}

	CZNC* pZNC = &CZNC::Get();
	pZNC->InitDirs(((argc) ? argv[0] : ""), sDataDir);

#ifdef HAVE_LIBSSL
	if (bMakePem) {
		pZNC->WritePemFile();

		delete pZNC;
		return 0;
	}
#endif /* HAVE_LIBSSL */

	if (bMakePass) {
		CString sSalt;
		CString sHash = CUtils::GetSaltedHashPass(sSalt);
		CUtils::PrintMessage("Use this in the <User> section of your config:");
		CUtils::PrintMessage("Pass = " + CUtils::sDefaultHash + "#" + sHash + "#" + sSalt + "#");

		delete pZNC;
		return 0;
	}

	if (bMakeConf) {
		if (!pZNC->WriteNewConfig(sConfig)) {
			delete pZNC;
			return 0;
		}
		/* Fall through to normal bootup */
	}

	if (!pZNC->ParseConfig(sConfig)) {
		CUtils::PrintError("Unrecoverable config error.");
		delete pZNC;
		return 1;
	}

	if (!pZNC->OnBoot()) {
		CUtils::PrintError("Exiting due to module boot errors.");
		delete pZNC;
		return 1;
	}

	// removed: checks for isRoot, bForeground, forking and signal handlers

	int iRet = 0;

	try {
		pZNC->Loop(&g_bMainLoop);
		CUtils::PrintMessage("Terminating ...");	
	} catch (CException e) {
		switch (e.GetType()) {
			case CException::EX_Shutdown:
				iRet = 0;
				CUtils::PrintMessage("************** Shutting down ZNC... **************");
				break;
			case CException::EX_Restart: {
				// strdup() because GCC is stupid
				char *args[] = {
					strdup(argv[0]),
					strdup("--datadir"),
					strdup(pZNC->GetZNCPath().c_str()),
					NULL,
					NULL,
					NULL,
					NULL,
					NULL
				};
				int pos = 3;
				if (CUtils::Debug())
					args[pos++] = strdup("--debug");
#if 0
				else if (bForeground)
					args[pos++] = strdup("--foreground");
				if (!CUtils::StdoutIsTTY())
					args[pos++] = strdup("--no-color");
				if (bAllowRoot)
					args[pos++] = strdup("--allow-root");
#endif
				args[pos++] = strdup(pZNC->GetConfigFile().c_str());
				// The above code adds 4 entries to args tops
				// which means the array should be big enough

				CUtils::PrintMessage("************** Restarting ZNC... **************");
				delete pZNC; /* stuff screws up real bad if we don't close all sockets etc. */
				pZNC = NULL;

				execvp(args[0], args);
				CUtils::PrintError("Unable to restart znc [" + CString(strerror(errno)) + "]");
			} /* Fall through */
			default:
				iRet = 1;
		}
	}

	if(pZNC) delete pZNC;

	return iRet;
}
