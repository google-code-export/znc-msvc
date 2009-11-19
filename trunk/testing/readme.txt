I'd like to describe a few things about the build system and the modifications I made.
The main objective was to separate the sources, project files, build dependencies,
intermediate files and the final build output.

Unfortunately Visual Studio puts everything is the project’s directory by default
which quickly becomes chaotic even for small projects. 
There are no absolute paths, only relative ones, so the whole directory can be
copied anywhere. Consequently, there are some strange looking paths,
like : "..\..\.." It’s a bit hard to follow but it serves the purpose :)


The directory structure looks like the following:

\
  +- znc-msvc\
  |  +- modules\
  |  |  +- extra\
  |  |  +- webadmin\
  |  |    +- ...
  |  +- win32\
  |  |  +- project\
  |  |  +- src\
  |  |  +- znc\
  |  |  +- znc_dll\
  |  |  +- znc_service\
  +- dependencies\
  |  +- c-ares\
  |  |  +- include\
  |  |  +- lib-debug\
  |  |  +- lib-release\
  |  +- OpenSSL\
  |  |  +- Win32\
  |  |  |  +- bin\
  |  |  |  +- include\
  |  |  |  +- lib\  
  |  |  +- Win64\
  |  |  |  +- ...
  +- build-temp\
  |  +- Win32-Release\
  |  +- Win32-Debug\
  +- build-out\
  |  +- Win32-Release\
  |  +- Win32-Debug\
  
A few words about the contents
------------------------------

win32\project: contains the main solution file and 3 project files for
ZNC.exe, ZNC.dll and ZNC_service.exe and a makefile for batch building the modules

win32\znc: ; win32\znc_dll ; win32\znc_service:
Source files corresponding the projects
There are separate main.cpp files for ZNC and ZNC_DLL. I left the original main.cpp
from the ZNC project and created new ones in win32\znc & win32\znc_dll.
This way I could remove a lot of preprocessor directives (#ifdefs and #ifndef)
and make the code much cleaner. I think these files are relatively static in contents
and don’t change much between revisions.

dependencies:

Header files, libraries and DLLs copied built by and copied from c-ares and OpenSSL

c-ares\includes: only a few header files are needed for building ZNC:
ares.h; ares_build.h; ares_rules.h and ares_version.h

c-ares\lib-*: areslib.lib for different configurations
A quick note: it doesn't work with v1.6.1 sources (daily snapshots taken from SVN),
only with the stable version: 1.6.0
It seems the c-ares authors made a few changes which broke Win32 compatibility :(

OpenSSL\Win32\bin: 32-bit OpenSSL DLLs: libeay32.dll and ssleay32.dll

OpenSSL\Win32\include: header files

OpenSSL\Win32\: 32-bit import libraries for OpenSSL DLLs

OpenSSL\Win64: the same files for 64-bit


Building the modules with makefile
----------------------------------

Open a Visual Studio command prompt, change directory to win32\project and execute:

nmake –f Modules.make CFG=<config> <target>

where <config> must be one of the following: [ Win32-Release | Win32-Debug ]
<target> must be one of: [ ALL | clean ]

If neither <config> nor <target> are specified this results in the default
Win32-Release config being built.


Pre-build and post-build events
-------------------------------

The projects contain post-build events which copy output files (.exe; .dll; .pbd)
to the build-out directory

ZNC_Service has pre-build events for compiling the message text file and building
the service provider DLL.


About 64-bit builds
-------------------
Windows and Unix–like operating systems use a different data model under 64-bit
(more info here: http://www.viva64.com/content/articles/64-bit-development/?f=20_issues_of_porting_C++_code_on_the_64-bit_platform.html&lang=en&content=64-bit-developmen)
The main difference is that long and unsigned long is 32 bit wide under Windows
but 64 bit under Unixes. This problem appears as compiler warnings when long type
parameters used for array indexes in 64-bit build.

I saw you bravely modified ZNC sources in a lot of places, but unfortunately
the same issue applies to c-ares as well.

I think there are two alternatives: build 64-bit version without c-ares or
omit the 64-bit build altogether (do we really need it right now?)

and it would be nice if we could somehow convince ZNC developers to use
size_t for array indexes :)
