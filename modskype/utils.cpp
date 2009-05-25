#include <string>
#include <windows.h>

#define ISDIGIT(c) (c > 0 && c < 255 && isdigit(c))

std::string stripIRCColorCodes(const std::string& a_str)
{
	std::string l_result;

	std::string::size_type l_pos, l_prevPos = 0;
	bool l_colorOpen = false, l_boldOpen = false, l_underlineOpen = false;
	const std::string l_chars = "\x02\x1F\x16\x0F\x03";

	l_pos = a_str.find_first_of(l_chars);
	while(l_pos != std::string::npos)
	{
		std::string::size_type l_endPos = l_pos + 1;

		if(a_str[l_pos] == 0x03)
		{
			int l_foreIndex = -1, l_backIndex = -1;
			std::string::size_type l_myPos = l_pos + 1;
			int *l_color = &l_foreIndex;
			
			while(l_myPos < a_str.size() && (ISDIGIT(a_str[l_myPos]) || a_str[l_myPos] == ','))
			{
				const char l_ch = a_str[l_myPos];

				if(l_ch == ',')
				{
					if(l_color == &l_foreIndex && l_myPos > l_pos + 1)
						l_color = &l_backIndex;
					else
						break;
				}
				else
				{
					const int l_digit = l_ch - 0x30;
					if(*l_color == -1)
					{
						*l_color = l_digit;
					}
					else if(l_digit >= 0 && (*l_color == 0 || (*l_color == 1 && l_digit <= 5)))
					{
						*l_color = (*l_color) * 10 + l_digit;
					}
					else
					{
						break;
					}
				}

				l_myPos++;
			}

			l_endPos = l_myPos;
		}

		l_result += a_str.substr(l_prevPos, l_pos - l_prevPos);
		l_prevPos = l_endPos;
		l_pos = a_str.find_first_of(l_chars, l_prevPos);
	}

	if(l_prevPos < a_str.size())
	{
		l_result += a_str.substr(l_prevPos);
	}

	return l_result;
}


#define _VALIDATE_BYTE_AT_POS \
  if((a_strInQuestion[c] & 0xc0) != 0x80) \
    return false; \
  l_val <<= 6; \
  l_val |= a_strInQuestion[c] & 0x3f;


bool IsStrValidUTF8(const std::string& a_strInQuestion)
{
	int l_val = 0, l_min = 0;
	unsigned int l_len = a_strInQuestion.size();

	for(unsigned int c = 0; c < l_len; c++)
	{
		if(a_strInQuestion[c] > 0 && a_strInQuestion[c] < 128)
		{
			continue;
		}
		else if((a_strInQuestion[c] & 0xE0) == 0xC0)
		{
			if(l_len - c < 2 || (a_strInQuestion[c] & 0x1E) == 0)
				return false;

			c++;

			if((a_strInQuestion[c] & 0xC0) != 0x80)
				return false;

			continue; // skip final checks
		}
		else if((a_strInQuestion[c] & 0xF0) == 0xE0)
		{
			if(l_len - c < 3)
				return false;

			l_min = (1 << 11);
			l_val = a_strInQuestion[c] & 0x0F;

			c++;
			_VALIDATE_BYTE_AT_POS;
			c++;
			_VALIDATE_BYTE_AT_POS;
		}
		else if((a_strInQuestion[c] & 0xF8) == 0xF0)
		{
			if(l_len - c < 4)
				return false;
			l_min = (1 << 16);
			l_val = a_strInQuestion[c] & 0x07;

			c++;
			_VALIDATE_BYTE_AT_POS;
			c++;
			_VALIDATE_BYTE_AT_POS;
			c++;
			_VALIDATE_BYTE_AT_POS;
		}
		else
			return false;

		// these two checks only apply to unicode chars of a length greater than 2 bytes
		if(l_val < l_min)
			return false;

		if(!((l_val) < 0x110000 && (((l_val) & 0xFFFFF800) != 0xD800) && ((l_val) < 0xFDD0 || (l_val) > 0xFDEF) && ((l_val) & 0xFFFE) != 0xFFFE))
			return false;
	}

	return true;
}


std::string WideToStr(const std::wstring a_wideStr, unsigned int a_targetCodePage)
{
	int l_size = WideCharToMultiByte(a_targetCodePage, 0, a_wideStr.c_str(), -1, NULL, NULL, NULL, NULL);

	if(l_size)
	{
		char *l_buf = new char[l_size];

		if(l_buf)
		{
			int l_chrs = WideCharToMultiByte(a_targetCodePage, 0, a_wideStr.c_str(), -1, l_buf, l_size, NULL, NULL);
			std::string l_result(l_buf);
			delete[] l_buf;
			return l_result;
		}
	}

	return "";
}


std::wstring StrToWide(const std::string a_str, unsigned int a_originCodePage)
{
	int l_size = MultiByteToWideChar(a_originCodePage, 0, a_str.c_str(), -1, NULL, NULL);

	if(l_size)
	{
		wchar_t *l_buf = new wchar_t[l_size];

		if(l_buf)
		{
			int l_chrs = MultiByteToWideChar(a_originCodePage, 0, a_str.c_str(), -1, l_buf, l_size);
			std::wstring l_result(l_buf);
			delete[] l_buf;
			return l_result;
		}
	}

	return L"";
}


std::string WideToUtf8(const std::wstring& a_wideStr)
{
#ifdef WIN32
	return WideToStr(a_wideStr, CP_UTF8);
#else
	size_t l_bufSize = wcstombs(NULL, a_wideStr.c_str(), 0);

	if(l_bufSize > 0)
	{
		char *l_buf = new char[l_bufSize + 2];
		size_t l_converted = wcstombs(l_buf, a_wideStr.c_str(), l_bufSize + 1);
		if(l_converted > 0)
		{
			l_buf[l_converted] = '\0';
			string l_result(l_buf);
			delete l_buf;
			return l_result;
		}
		else
			delete l_buf;
	}

	return "";
#endif
}


std::string AnsiToUtf8(const std::string& a_ansiStr)
{
#ifdef WIN32
	return WideToStr(StrToWide(a_ansiStr, CP_ACP), CP_UTF8);
#else

#endif
}


std::string Utf8ToAnsi(const std::string& a_utfStr)
{
#ifdef WIN32
	return WideToStr(StrToWide(a_utfStr, CP_UTF8), CP_ACP);
#else

#endif
}


std::wstring Utf8ToWide(const std::string& a_utfStr)
{
#ifdef WIN32
	return StrToWide(a_utfStr, CP_UTF8);
#else
	size_t l_bufSize = mbstowcs(NULL, a_utfStr.c_str(), 0);

	if(l_bufSize > 0)
	{
		wchar_t *l_buf = new wchar_t[l_bufSize + 2];
		size_t l_converted = mbstowcs(l_buf, a_utfStr.c_str(), l_bufSize + 1);
		if(l_converted > 0)
		{
			l_buf[l_converted] = L'\0';
			wstring l_result(l_buf);
			delete l_buf;
			return l_result;
		}
		else
			delete l_buf;
	}

	return L"";
#endif
}

