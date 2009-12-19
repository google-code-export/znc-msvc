#include "guiutils.hpp"

namespace ZNCGUIUtils
{

	// credits: http://blogs.powersoft.ca/erict/archive/2008/03/17/loading-an-icon-in-ccli.aspx
	Icon^ GetIconFromRes(int a_resID)
	{
		// Get the primary module
		Module ^ mod = Assembly::GetExecutingAssembly()->GetModules()[0];

		// Get the instance handle 
		IntPtr hinst = Marshal::GetHINSTANCE(mod);

		// Get the icon as unmanaged
		HICON hic = LoadIcon((HINSTANCE) hinst.ToPointer(), MAKEINTRESOURCE(a_resID)); 

		// import the unmanaged icon into the managed side 
		Icon^ ic = Icon::FromHandle(IntPtr(hic));

		// destroy the unmanaged icon 
		DestroyIcon(static_cast<HICON>(ic->Handle.ToPointer() ));

		return ic;
	}

}