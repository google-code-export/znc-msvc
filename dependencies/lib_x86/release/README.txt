Copy the dependencies' release libraries here.

Copy these file here:
	c-ares\msvc90\cares\dll-release\cares.{lib,exp,dll}
		(built using "nmake -f Makefile.msvc CFG=dll-release" or the vc6cares.dsw project file)

	openssl\out32dll\libeay32.{dll,dll.manifest,exp,lib}
	openssl\out32dll\ssleay32.{dll,dll.manifest,exp,lib}
		(built according to INSTALL.W32)

Don't make any sub folders in this folder.
