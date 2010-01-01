OPTIONALLY copy the dependencies' debug libraries here.
If you don't copy them, ZNC debug builds will use the libraries' release lib files.

Copy these file here:
	c-ares\vc\cares\lib-debug-x64\libcaresd.lib [RENAME TO libcares.lib!]
		(built using "nmake -f Makefile.msvc" from the x64 VS Command Line or the vc6cares.dsw project file)

	OpenSSL x64 debug libraries.
