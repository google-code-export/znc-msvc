OPTIONALLY copy the dependencies' debug libraries here.
If you don't copy them, ZNC debug builds will use the libraries' release lib files.

Copy these file here:
	c-ares\vc\cares\dll-debug-x64\caresd.{lib,exp,dll} [RENAME TO cares.{lib,exp,dll}!]
		(built using "nmake -f Makefile.msvc CFG=dll-debug" from the x64 VS Command Line or the vc6cares.dsw project file)

	OpenSSL x64 debug libraries.
