#include "main.h"

void ManagedMain();

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	srand((unsigned int)time(NULL));
	_set_fmode(_O_BINARY);

	// make sure the stuff in ZNC.dll matches this exe's version... bad crashes otherwise.
	if(CZNC::GetCoreVersion() != MODVERSION)
	{
		MessageBoxW(0, L"The version number in ZNC.dll doesn't match. Aborting.", L"ZNC GUI Error", MB_ICONSTOP);
		return 1;
	}

#ifdef HAVE_LIBSSL
	CRYPTO_malloc_init();
#endif

	CUtils::SetStdoutIsTTY(false);

	if (!InitCsocket())
	{
		MessageBoxW(0, L"Could not initialize Csocket!", L"Fatal Error", MB_ICONSTOP);
		return -1;
	}

	ManagedMain();

    return 0;
}
