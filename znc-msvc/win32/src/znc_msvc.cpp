#include "stdafx.hpp"

int usleep(int useconds)
{
	Sleep(useconds / 1000);
	return 0;
}

int sleep(int seconds)
{
	Sleep(seconds * 1000);
	return 0;
}

static bool s_bServiceMode = false;

bool ZNCWin32ServiceMode()
{
	return s_bServiceMode;
}

void ZNCWin32SetServiceMode(bool b)
{
	s_bServiceMode = b;
}

// Source: http://www.openasthra.com/c-tidbits/gettimeofday-function-for-windows/

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}

struct tm* localtime_r(const time_t *clock, struct tm *result)
{ 
	if (!clock || !result) return NULL;
	memcpy(result, localtime(clock), sizeof(*result)); 
	return result; 
}

struct tm* localtime_locked(const time_t *timer)
{
	static struct tm l_tm;
	
	if(localtime_s(&l_tm, timer) == 0)
	{
		return &l_tm;
	}
	else
	{
		return NULL;
	}
}

/**
 * get password from console
 * Source: http://svn.openvpn.net/projects/openvpn/obsolete/BETA15/openvpn/io.c
 * License: GNU GPL v2
 * (modified to display * for input)
 **/

std::string getpass(const char *prompt)
{
	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	std::string result;
	DWORD count;

	if(in == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	// the total size of the buffer must be less than 64K chars
	if(WriteConsole(out, prompt, DWORD(strlen(prompt)), &count, NULL))
	{
		int istty = (GetFileType(in) == FILE_TYPE_CHAR);
		DWORD old_mode;
		char c;

		if(istty)
		{
			if(GetConsoleMode(in, &old_mode))
			{
				SetConsoleMode(in, old_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
			}
			else
			{
				istty = 0;
			}
		}

		while(ReadConsole(in, &c, 1, &count, NULL) && (c != '\r') && (c != '\n'))
		{
			if(c == '\b')
			{
				if(!result.empty())
				{
					WriteConsole(out, "\b \b", 3, &count, NULL);
					result.erase(result.end() - 1);
				}
			}
			else
			{
				WriteConsole(out, "*", 1, &count, NULL);
				result += c ;
			}
		}

		WriteConsole(out, "\r\n", 2, &count, NULL);

		if(istty)
		{
			SetConsoleMode(in, old_mode);
		}
	}

	return result;
}


/* Microsoft's strftime does not handle user defined error-prone format input very well, and
according to http://msdn.microsoft.com/en-us/library/fe06s4ak%28VS.71%29.aspx is not supposed to,
so we use this wrapper. */
size_t strftime_validating(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
{
	CString l_safeFormat;
	const char* p = format;

	while(*p)
	{
		if(*p == '%')
		{
			bool l_hash = (*(p + 1) == '#');
			char c = *(p + (l_hash ? 2 : 1));

			switch(c)
			{
			case 'a': case 'A': case 'b': case 'B': case 'c': case 'd': case 'H': case 'I': case 'j': case 'm': case 'M':
			case 'p': case 'S': case 'U': case 'w': case 'W': case 'x': case 'X': case 'y': case 'Y': case 'z': case 'Z':
			case '%':
				// formatting code is fine
				if(l_hash)
				{
					l_safeFormat += *p;
					++p;
				}
				// the current loop run will append % (and maybe #), and the next one will do the actual char.
				break;
			default: // replace bad formatting code with itself, escaped, e.g. "%V" --> "%%V"
				l_safeFormat.append("%%");
				++p;
				break;
			}

			// if c == '%', append it in this run to avoid confusion in the next one (p has already been incremented now!)
			if(c == '%')
			{
				l_safeFormat += *p;
				++p;
			}
		}

		if(*p)
		{
			l_safeFormat += *p;
			++p;
		}
	}

	return strftime(strDest, maxsize, l_safeFormat.c_str(), timeptr);
}

