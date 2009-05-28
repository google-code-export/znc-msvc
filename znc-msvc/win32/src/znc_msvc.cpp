#include <main.h>

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

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	if(n == 0)
		return 0;

	// try to find first non-equal char:
	while(n-- > 0 && tolower(*s1) == tolower(*s2))
	{
		if(n == 0 || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}

	return tolower(*(unsigned char*) s1) - tolower(*(unsigned char*) s2);
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

	if(WriteConsole(out, prompt, strlen(prompt), &count, NULL))
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

