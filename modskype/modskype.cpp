/*
 * Copyright (C) 2009
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "znc.h"
#include "Chan.h"
#include "User.h"
#include "Modules.h"
#include "modskype.hpp"


class CReceivedSkypeMsg
{
public:
	uint64_t msgID;
	CString body;
	time_t timestamp;
	CString chatName;
	CString fromDispname;
	CString type;
	bool edited;

	CReceivedSkypeMsg()
	{
		msgID = 0;
		timestamp = 0;
		edited = false;
	}

	bool isComplete()
	{
		return (msgID > 0 && timestamp > 0 && !type.empty() &&
			!body.empty() && !chatName.empty() && !fromDispname.empty());
	}
};


class CSkypeMod : public CModule
{
protected:
	CSkypeMsgQueue *m_queue;

	map<uint64_t, CReceivedSkypeMsg*> m_rcvdMsgs;
	CRITICAL_SECTION m_rcvdMsgLock;

	MCString m_chanNameMap; // #channel => chatname
public:
	MODCONSTRUCTOR(CSkypeMod) {}

	static CSkypeMod *ms_instance;

	bool OnLoad(const CString& sArgs, CString& sMessage)
	{
		if(ms_instance) return false;
		ms_instance = this;
		
		InitializeCriticalSection(&m_rcvdMsgLock);

		for(MCString::iterator it = BeginNV(); it != EndNV(); it++)
		{
			if(it->first.size() > 0 && it->first[0] == '#')
			{
				m_chanNameMap[it->first.AsLower()] = it->second;
			}
		}

		SkypeMessaging *l_msging = SkypeMessaging::GetInstance();
		m_queue = new CSkypeMsgQueue();

		if(!l_msging->Initialize(&CSkypeMsgQueue::SkypeMessagingCallback, m_queue))
		{
			PutModule("FATAL ERROR: Initializing Skype messaging failed!");
		}
		else
		{
			if(!l_msging->SendConnect())
			{
				PutModule("FATAL ERROR: Sending discovery signal failed!");
			}
			else
			{
				AddTimer(new CModSkypeTimer(this, 1, 0, "ModSkypeMessaging", "Timer for modskype to process pending events/messages."));
			}
		}

		return true;
	}

	static CSkypeMod *GetInstance()
	{
		return ms_instance;
	}

	CSkypeMsgQueue *GetQueue()
	{
		return m_queue;
	}

	~CSkypeMod()
	{
		SkypeMessaging::ClearInstance();

		DeleteCriticalSection(&m_rcvdMsgLock);

		ms_instance = NULL;
		delete m_queue;
	}

	bool SendSkypeCommand(const CString &a_cmd)
	{
		return SkypeMessaging::GetInstance()->SendCommand(a_cmd.c_str());
	}

	bool SendIRCMsgToSkype(const CString& a_nick, const CString& a_channel, const CString& a_message)
	{
		if(m_chanNameMap.find(a_channel.AsLower()) != m_chanNameMap.end() && !a_message.empty())
		{
			CString l_message = stripIRCColorCodes(a_message);

			if(!IsStrValidUTF8(l_message))
			{
				l_message = AnsiToUtf8(l_message);
			}

			if(!a_nick.empty())
			{
				l_message = "<" + a_nick + "> " + l_message;
			}

			return SendSkypeCommand("CHATMESSAGE " + m_chanNameMap[a_channel.AsLower()] + " " + l_message);
		}
		return false;
	}

	EModRet OnUserMsg(CString& sTarget, CString& sMessage)
	{
		SendIRCMsgToSkype(m_pUser->GetIRCNick().GetNick(), sTarget, sMessage);
		return CONTINUE;
	}

	void OnModCommand(const CString& sLine) {
		CString sCommand = sLine.Token(0).AsUpper();

		if(sCommand == "RELAY")
		{
			if(SendIRCMsgToSkype(sLine.Token(1), sLine.Token(2), sLine.Token(3, true)))
			{
				PutModule("RELAY: message sent!");
				return;
			}
			PutModule("RELAY: message failed!");
		}
		else if(sCommand == "SET")
		{
			const CString& sChan = sLine.Token(1), sChatName = sLine.Token(2);

			if(!sChan.empty() && !sChatName.empty())
			{
				m_chanNameMap[sChan.AsLower()] = sChatName;
				SetNV(sChan, sChatName);
				PutModule("MAPPED: chatname " + sChatName + " to " + sChan + "!");
			}
		}
	}

	EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage)
	{
		SendIRCMsgToSkype(Nick.GetNick(), Channel.GetName(), sMessage); // will do nothing if the channel is not a skype mapped chan.
		return CONTINUE;
	}

	EModRet OnChanCTCP(CNick& Nick, CChan& Channel, CString& sMessage)
	{
		if(sMessage.Token(0).Equals("ACTION"))
		{
			SendIRCMsgToSkype("", Channel.GetName(), Nick.GetNick() + " " + sMessage.Token(1, true));
		}
		return CONTINUE;
	}

	void SendSkypeMsgToIRC(const CReceivedSkypeMsg *a_msg)
	{
		CString l_msgBuffer;

		if(a_msg->timestamp < time(0) - 120 && !a_msg->edited)
		{
			char l_timeBuf[100];
			strftime(l_timeBuf, 100, "[%a %H:%M:%S] ", localtime(&a_msg->timestamp));
			l_msgBuffer = l_timeBuf;
		}

		if(!a_msg->edited)
		{
			if(a_msg->type == "EMOTED")
			{
				l_msgBuffer += a_msg->fromDispname + " ";
			}
			else
			{
				l_msgBuffer += "<" + a_msg->fromDispname + "> ";
			}

			l_msgBuffer += a_msg->body;
		}
		else
		{
			char l_timeBuf[100];
			strftime(l_timeBuf, 100, "%H:%M:%S", localtime(&a_msg->timestamp));

			if(a_msg->body == "[deleted]")
			{
				l_msgBuffer += "(" + a_msg->fromDispname + " deleted their message from " + CString(l_timeBuf) + ")";
			}
			else
			{
				l_msgBuffer += a_msg->fromDispname + " edited their message from " + CString(l_timeBuf) + ": " + a_msg->body;
			}
		}

		l_msgBuffer.Replace("\r", "");
		l_msgBuffer.Replace("\n", " ");

		for(MCString::iterator it = m_chanNameMap.begin(); it != m_chanNameMap.end(); it++)
		{
			if(it->second.Equals(a_msg->chatName))
			{
				std::wstring l_buffer = Utf8ToWide(l_msgBuffer);
				std::wstring l_line;
				CString l_utfLine;

				do
				{
					l_line = l_buffer.substr(0, 400);
					l_buffer.erase(0, 400);

					if(!l_buffer.empty())
					{
						l_line += L"...";
						l_buffer = L"..." + l_buffer;
					}

					l_utfLine = WideToUtf8(l_line);

					PutIRC("PRIVMSG " + it->first + " :" + l_utfLine);
					PutUser(":" + m_pUser->GetIRCNick().GetNick() + "!x@y PRIVMSG " + it->first + " :" + l_utfLine);
				} while(!l_buffer.empty());

				return;
			}
		}

		PutModule("WARNING: Group chat " + a_msg->chatName + " not mapped to any channel. A message has been lost.");
	}

	void Skype_GotNewMessage(uint64_t a_msgID, bool a_edited)
	{
		EnterCriticalSection(&m_rcvdMsgLock);

		try
		{
			m_rcvdMsgs[a_msgID] = new CReceivedSkypeMsg();
			m_rcvdMsgs[a_msgID]->msgID = a_msgID;
			m_rcvdMsgs[a_msgID]->edited = a_edited;

			SendSkypeCommand("GET CHATMESSAGE " + CString(a_msgID) + " TIMESTAMP");
			if(!a_edited)
			{
				SendSkypeCommand("GET CHATMESSAGE " + CString(a_msgID) + " BODY");
			}
			SendSkypeCommand("GET CHATMESSAGE " + CString(a_msgID) + " CHATNAME");
			SendSkypeCommand("GET CHATMESSAGE " + CString(a_msgID) + " FROM_DISPNAME");
			SendSkypeCommand("GET CHATMESSAGE " + CString(a_msgID) + " TYPE");
		}
		catch(...) {}

		LeaveCriticalSection(&m_rcvdMsgLock);
	}
	
	void Skype_GotMessageProperty(uint64_t a_msgID, const CString& a_property, const CString& a_value)
	{
		EnterCriticalSection(&m_rcvdMsgLock);

		try
		{
			if(m_rcvdMsgs.find(a_msgID) != m_rcvdMsgs.end())
			{
				CReceivedSkypeMsg *l_msg = m_rcvdMsgs[a_msgID];

				if(a_property.Equals("BODY"))
				{
					l_msg->body = (l_msg->edited && a_value.empty() ? "[deleted]" : a_value);
				}
				else if(a_property.Equals("TIMESTAMP"))
					l_msg->timestamp = a_value.ToULong();
				if(a_property.Equals("CHATNAME"))
					l_msg->chatName = a_value;
				else if(a_property.Equals("FROM_DISPNAME"))
					l_msg->fromDispname = a_value;
				else if(a_property.Equals("TYPE"))
					l_msg->type = a_value;

				if(l_msg->isComplete())
				{
					if(l_msg->type == "SAID" || l_msg->type == "EMOTED" || l_msg->edited)
					{
						SendSkypeMsgToIRC(l_msg);
					}
					m_rcvdMsgs.erase(m_rcvdMsgs.find(a_msgID));
					delete l_msg;
				}
			}
		}
		catch(...) {}

		LeaveCriticalSection(&m_rcvdMsgLock);
	}
};


CSkypeMsgQueue::CSkypeMsgQueue()
{
	InitializeCriticalSection(&m_waitingLock);
}

CSkypeMsgQueue::~CSkypeMsgQueue()
{
	DeleteCriticalSection(&m_waitingLock);
}

int CSkypeMsgQueue::SkypeMessagingCallback(ULONG_PTR a_reserved, const char *a_str_utf8, void *a_userData)
{
	CSkypeMsgQueue *l_qi = reinterpret_cast<CSkypeMsgQueue*>(a_userData);

	if(!l_qi) return 0;
	
	EnterCriticalSection(&l_qi->m_waitingLock);

	try
	{
		l_qi->m_waiting.push_back(a_str_utf8);
	}
	catch(...) {}
	
	LeaveCriticalSection(&l_qi->m_waitingLock);

	return 0;
}

bool CSkypeMsgQueue::GetEntry(string &ar_result)
{
	bool l_result = false;

	EnterCriticalSection(&m_waitingLock);

	try
	{
		if(m_waiting.size() > 0)
		{
			ar_result = *m_waiting.begin();

			m_waiting.erase(m_waiting.begin());
			l_result = true;
		}
	}
	catch(...) {}
	
	LeaveCriticalSection(&m_waitingLock);

	return l_result;
}


void CModSkypeTimer::RunJob()
{
	CSkypeMod *m = (CSkypeMod*)m_pModule;
	CString l_raw;

	if(!m || !m->GetQueue())
		return;
	
	while(m->GetQueue()->GetEntry(l_raw))
	{
		if(l_raw.Token(0) == "CHATMESSAGE")
		{
			uint64_t l_msgID = l_raw.Token(1).ToLongLong();

			if(l_msgID)
			{
				if(l_raw.Token(2) == "STATUS" &&
					l_raw.Token(3) == "RECEIVED")
				{
					m->Skype_GotNewMessage(l_msgID, false);
				}
				else if(l_raw.Token(2) == "BODY" || l_raw.Token(2) == "TIMESTAMP" || l_raw.Token(2) == "TYPE" ||
					l_raw.Token(2) == "CHATNAME" || l_raw.Token(2) == "FROM_DISPNAME")
				{
					m->Skype_GotMessageProperty(l_msgID, l_raw.Token(2), l_raw.Token(3, true));
				}
				else if(l_raw.Token(2) == "EDITED_TIMESTAMP")
				{
					m->Skype_GotNewMessage(l_msgID, true);
				}
			}
		}
	}
}


MODULEDEFS(CSkypeMod, "Skype-IRC Group Chat Bridge Bot.");

CSkypeMod *CSkypeMod::ms_instance = NULL;
