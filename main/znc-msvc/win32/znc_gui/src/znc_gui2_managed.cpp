#include "main.h"
#include "guiutils.hpp"
#include <frmMain.h>

void ManagedMain()
{
	Application::EnableVisualStyles();

	Application::Run(gcnew znc_gui2::frmMain());
}
