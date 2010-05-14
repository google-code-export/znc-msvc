#include "Modules.h"
#include "Nick.h"

class CNoDCCUserMod : public CModule
{
public:
	MODCONSTRUCTOR(CNoDCCUserMod) {}

	EModRet OnPrivCTCP(CNick& Nick, CString& sMessage)
	{
		if(sMessage.Token(0).Equals("DCC") || sMessage.Token(0).Equals("RDCC"))
		{
			PutStatus("A DCC [" + sMessage.Token(1) + "] request from [" + Nick.GetNick() + "] has been ignored.");
			return HALT;
		}
		return CONTINUE;
	}

	EModRet OnUserCTCP(CString& sTarget, CString& sMessage)
	{
		if(sMessage.Token(0).Equals("DCC") || sMessage.Token(0).Equals("RDCC"))
		{
			PutStatus("You have disabled DCC functionality. A DCC [" + sMessage.Token(1) + "] request has been ignored.");
			return HALT;
		}
		return CONTINUE;
	}

};

MODULEDEFS(CNoDCCUserMod, "Forcefully ignores all DCC requests.")
