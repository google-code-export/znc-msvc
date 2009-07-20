#include "stdafx.hpp"
#include "Modules.h"

class CLagchk : public CModule {
public:
	MODCONSTRUCTOR(CLagchk) {}
	virtual EModRet OnUserRaw(CString& sLine) {
		if (sLine.Token(0) == "LAGCHK" && sLine.Token(2).empty())
			return HALT;
		return CONTINUE;
	}
};

MODULEDEFS(CLagchk, "Block LAGCHK client messages")
