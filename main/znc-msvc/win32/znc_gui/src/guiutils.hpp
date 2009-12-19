#include "winver.h"
#include <windows.h>
#include "resource.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Drawing;

namespace ZNCGUIUtils
{
	Icon^ GetIconFromRes(int a_resID);
}