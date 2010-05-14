#include "Modules.h"
#include "Nick.h"

class CNoDCCGlobalMod : public CGlobalModule
{
public:
	GLOBALMODCONSTRUCTOR(CNoDCCGlobalMod) {}

	EModRet OnPrivCTCP(CNick& Nick, CString& sMessage)
	{
		if(sMessage.Token(0).Equals("DCC") || sMessage.Token(0).Equals("RDCC"))
		{
			PutStatus("A DCC [" + sMessage.Token(1) + "] request from [" + Nick.GetNick() + "] has been ignored because "
				"your administrator has disallowed all DCC transfers on this ZNC.");
			return HALT;
		}
		return CONTINUE;
	}

	EModRet OnUserCTCP(CString& sTarget, CString& sMessage)
	{
		if(sMessage.Token(0).Equals("DCC") || sMessage.Token(0).Equals("RDCC"))
		{
			PutStatus("Your administrator has disallowed all DCC transfers on this ZNC. "
				"Your DCC [" + sMessage.Token(1) + "] request has been ignored.");
			return HALT;
		}
		return CONTINUE;
	}

};

GLOBALMODULEDEFS(CNoDCCGlobalMod, "Forcefully ignores all DCC requests.")
