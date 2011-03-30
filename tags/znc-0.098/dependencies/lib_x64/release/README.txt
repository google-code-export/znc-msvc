Copy the dependencies' release libraries here.

Copy these file here:
	c-ares\vc\cares\dll-release-x64\cares.{lib,exp,dll}
		(built using "nmake -f Makefile.msvc CFG=dll-release" from the x64 VS Command Line or the vc6cares.dsw project file)

	openssl\out32dll\libeay32.{dll,dll.manifest,exp,lib}
	openssl\out32dll\ssleay32.{dll,dll.manifest,exp,lib}
		(built according to INSTALL.W64! +
			comment out the lines with
				$ex_libs.=' bufferoverflowu.lib' if ($FLAVOR =~ /WIN64/);
			in util\pl\VC-32.pl)

Don't make any sub folders in this folder.
