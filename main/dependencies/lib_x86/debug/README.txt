OPTIONALLY copy the dependencies' debug libraries here.
If you don't copy them, ZNC debug builds will use the libraries' release lib files.

Copy these file here:
	c-ares\msvc90\cares\dll-debug\caresd.{dll,exp,lib} [RENAME TO cares.{dll,exp,lib}!]
		(built using "nmake -f Makefile.msvc CFG=dll-debug" or the vc6cares.dsw project file)

	OpenSSL x86 debug libraries. Please refer to INSTALL.W32 from the OpenSSL
	package for more information.
