#include "main.h"
#include "User.h"
#include "Nick.h"
#include "Modules.h"
#include "Chan.h"
#include "znc.h"

class CNotifyMod : public CGlobalModule {
public:
	GLOBALMODCONSTRUCTOR(CNotifyMod) 
	{
		
		
	}
	
	virtual void OnUserAttached() {
		pUserMap = CZNC::Get().GetUserMap();
		Broadcast("has attached to ZNC from " + m_pClient->GetRemoteIP());
	}
	
	virtual void OnUserDetached() {
		pUserMap = CZNC::Get().GetUserMap();	
		Broadcast("has detached out of ZNC from " + m_pClient->GetRemoteIP());
	}
	

	void Broadcast(CString text) {
		
		for (map<CString,CUser*>::iterator a = pUserMap.begin(); a != pUserMap.end(); a++) {
			//PutModule(m_pUser->GetUserName());
			//PutModule(a->second->GetUserName());
			//PutModule("----------");
			if(m_pUser->GetUserName() != a->second->GetUserName()) {
				if( GetNV( m_pUser->GetUserName() + "_sw") != "off") {
					
					if(GetNV( m_pUser->GetUserName() + "_type") == "notice") {
						
						a->second->PutStatusNotice("*** " + m_pUser->GetUserName() + " " + text);
						
					} else {
						
						a->second->PutModule("notify", m_pUser->GetUserName() + " " + text);
						
					}
					
				}
			}
		}
		
	}
	
	virtual void OnModCommand(const CString& sCommand) {
		if (strcasecmp(sCommand.c_str(), "help") == 0) {
			
			PutModule("NOTIFY - enables/disables notification. Arguments ON or OFF.");
			PutModule("TYPE - Notification type. Arguments NOTICE or MESSAGE.");
			PutModule("STATUS - Returns current selected options");
		
		} else {
			if(sCommand.Token(0).MakeUpper() == "NOTIFY")  {
				
				if(sCommand.Token(1).MakeUpper() == "OFF") {
					
					SetNV(m_pUser->GetUserName() + "_sw", "off", true);
					PutModule("Notifications disabled!");
					
				} else {
					if(sCommand.Token(1).MakeUpper() == "ON") {
						
						SetNV(m_pUser->GetUserName() + "_sw" , "on", true);
						PutModule("Notifications enabled!");
						
					} else {
						
						PutModule("Invalid Argument. Try 'help'.");
						
					}
				}
			} else {
				
				if(sCommand.Token(0).MakeUpper() == "TYPE")  {
					if(sCommand.Token(1).MakeUpper() == "NOTICE") {
						
						SetNV(m_pUser->GetUserName() + "_type" , "notice", true);
						PutModule("Notifications set to notice mode!");
						
					} else {
						if(sCommand.Token(1).MakeUpper() == "MESSAGE") {
							
							SetNV(m_pUser->GetUserName() + "_type" , "message", true);
							PutModule("Notifications set to message mode!");
							
							
						} else {
							
							PutModule("Invalid Argument. Try 'help'.");
							
						}
						
					}
					
				} else {
					
					if(sCommand.Token(0).MakeUpper() == "STATUS")  {
						PutModule("Current Options:");
						
						CString sw = GetNV( m_pUser->GetUserName() + "_sw").MakeUpper();
						CString type = GetNV( m_pUser->GetUserName() + "_type").MakeUpper();
						if(sw.length() == 0) {
							SetNV(m_pUser->GetUserName() + "_sw", "on", true);
							sw = "ON";
						}
						if(type.length() == 0) {
							SetNV(m_pUser->GetUserName() + "_type", "message", true);
							type = "MESSAGE";
						}
						
						PutModule("Notifications are " + sw);
						PutModule("Notification type " + type);
						
					
					} else {
						PutModule("Unknown command. Try 'help'.");
					}
					
				}
				
			}
		}
	}
private:
	map<CString,CUser*>	pUserMap;
};
GLOBALMODULEDEFS(CNotifyMod, "Notifies other ZNC users when someone de/attaches.")
