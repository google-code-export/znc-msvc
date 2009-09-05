#ifndef _ZNC_WIN32_H
#define _ZNC_WIN32_H

#define __MINGW_NOTHROW // for some MinGW headers.
#pragma warning(disable:4996) // disable "The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name"
#pragma warning(disable:4190) // 'ZNCModDescription' has C-linkage specified, but returns UDT 'CString' which is incompatible with C.
#pragma warning(disable:4251) // disable "... needs to have dll-interface to be used by clients of class ..."
#pragma warning(disable:4275) // disable "non dll-interface class ... used as base for dll-interface class ..."
	// I don't really feel good about the latter two, but it didn't crash so far. Fingers crossed!

#include <xutility>
using std::_cpp_min;
using std::_cpp_max;

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdint.h>
#include <string>
#include <queue>

#include "exports.h"

typedef int mode_t;
typedef unsigned int uint;

#define strcasecmp _stricmp
int ZNC_API strncasecmp(const char *s1, const char *s2, size_t n);
int ZNC_API gettimeofday(struct timeval *tv, struct timezone *tz);
ZNC_API struct tm* localtime_r(const time_t *clock, struct tm *result);
#define localtime localtime_locked
ZNC_API struct tm* localtime_locked(const time_t *timer);
int ZNC_API rand_r(unsigned int* seed);
long long ZNC_API rand_r(unsigned long long* seed);
std::string getpass(const char *prompt);

int ZNC_API usleep(int useconds);
int ZNC_API sleep(int seconds);

int fsync(int fd);

#define strtoll _strtoi64
#define strtoull _strtoui64
#define snprintf _snprintf

#define ftruncate _chsize

#define uid_t short
#define gid_t short

#define __PRETTY_FUNCTION__ __FUNCTION__

#endif // _ZNC_WIN32_H