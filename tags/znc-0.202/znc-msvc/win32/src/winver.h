#pragma once

#ifdef _WIN32
#define WINVER		0x0501
#define _WIN32_WINNT   WINVER
#define _WIN32_WINDOWS WINVER
#define _WIN32_IE      0x0600
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN_MSVC
#define VC_EXTRALEAN
#endif
