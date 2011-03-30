// Module by infogulch
// http://people.znc.in/~psychon/znc/modules/partdetach2.cpp

#include "stdafx.hpp"
#include "Chan.h"
#include "Modules.h"
#include "User.h"

class CPartDetach : public CModule {
public:
	MODCONSTRUCTOR(CPartDetach) {}
	virtual ~CPartDetach() {}

	virtual EModRet OnUserPart(CString& sChannel, CString& sMessage) {
		CChan* pChan = m_pUser->FindChan(sChannel);
		if (pChan && !pChan->IsDetached() && pChan->InConfig()) {
			if (sMessage.Token(0).CaseCmp("force") == 0) {
				sMessage = sMessage.Token(1,true);
			} else {
				pChan->DetachUser();
				return HALTCORE;
			}
		}
		return CONTINUE;
	}
	
	virtual void OnModCommand(const CString& sLine) {
		if (sLine.Token(0).CaseCmp("help") == 0) {
			PutModule("If a channel is saved in your config, and you try to part from it, you are detached instead.\nEXCEPT: If the first word in your part message is 'force' then it really does part you from the channel. The remaining part of the part message is used as the real part message sent to the channel.\nFor example: /part #mychan force This is the real part message.");
		}
	}
};

MODULEDEFS(CPartDetach, "Client part detaches instead, for saved chans")
