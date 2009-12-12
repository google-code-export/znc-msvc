#pragma once

#ifdef _WIN32
#define WINVER		0x0500
#define _WIN32_WINNT   0x0500
#define _WIN32_WINDOWS 0x0500
#define _WIN32_IE      0x0600
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN_MSVC
#define VC_EXTRALEAN
#endif