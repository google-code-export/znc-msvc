OPTIONALLY copy the dependencies' debug libraries here.
If you don't copy them, ZNC debug builds will use the libraries' release lib files.

Copy these file here:
	c-ares\msvc90\cares\lib-debug\libcaresd.lib [RENAME TO libcares.lib!]
		(built using "nmake -f Makefile.msvc")

	OpenSSL x86 debug libraries. Please refer to INSTALL.W32 from the OpenSSL
	package for more information.
