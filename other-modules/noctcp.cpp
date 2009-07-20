#include "stdafx.hpp"
#include "Modules.h"
class CNoCTCPMod : public CModule {
public:
	MODCONSTRUCTOR(CNoCTCPMod) {}
	virtual EModRet OnPrivCTCP(CNick& Nick, CString& sMessage) {
		if (sMessage.Token(0).Equals("ACTION"))
			return CONTINUE;
		return HALT;
	}
	virtual EModRet OnChanCTCP(CNick& Nick, CChan& Channel, CString& sMessage) {
		if (sMessage.Token(0).Equals("ACTION"))
			return CONTINUE;
		return HALT;
	}
};
MODULEDEFS(CNoCTCPMod, "Ignore all CTCPs, except /me")
