#ifndef _MODSKYPE_H
#define _MODSKYPE_H

#include "SkypeLink.hpp"


class CModSkypeTimer : public CTimer
{
public:
	CModSkypeTimer(CModule* pModule, unsigned int uInterval, unsigned int uCycles, const CString& sLabel, const CString& sDescription) : CTimer(pModule, uInterval, uCycles, sLabel, sDescription) {}
	~CModSkypeTimer() {}
protected:
	void RunJob();
};


class CSkypeMsgQueue
{
protected:
	vector<CString> m_waiting; 
	CRITICAL_SECTION m_waitingLock;
public:
	CSkypeMsgQueue();
	~CSkypeMsgQueue();
	static int SkypeMessagingCallback(ULONG_PTR a_reserved, const char *a_str_utf8, void *a_userData);
	bool GetEntry(string &ar_result);
};

std::string stripIRCColorCodes(const std::string& a_str);
bool IsStrValidUTF8(const string& a_strInQuestion);
std::string AnsiToUtf8(const std::string& a_ansiStr);
std::wstring Utf8ToWide(const std::string& a_utfStr);
std::string WideToUtf8(const std::wstring& a_wideStr);

#endif // !_MODSKYPE_H
