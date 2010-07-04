# ---------------------------------
# Makefile for building ZNC modules
# ---------------------------------
# Configuration validation is based on Makefile.msvc from
# c-ares (http://c-ares.haxx.se/)


# don't display commands invoked by NMAKE
.SILENT :

# -------------------------
# Configuration validation
# -------------------------

VALID_CFG = FALSE
!IF "$(CFG)" == "Win32-Release" || "$(CFG)" == "Win32-Debug" || \
  "$(CFG)" == "x64-Release" || "$(CFG)" == "x64-Debug"
VALID_CFG = TRUE
!ENDIF

!IF "$(VALID_CFG)" == "FALSE" && "$(CFG)" != ""
!  MESSAGE ---------------------------------
!  MESSAGE Makefile for building ZNC modules
!  MESSAGE ---------------------------------
!  MESSAGE
!  MESSAGE Usage: nmake /f Modules.make CFG=<config> <target>
!  MESSAGE
!  MESSAGE <config> must be one of: [ Win32-Release | Win32-Debug | x64-Release | x64-Debug ]
!  MESSAGE <target> must be one of: [ ALL | clean ]
!  MESSAGE
!  MESSAGE If neither <config> nor <target> are specified this results in
!  MESSAGE the default Win32-Release config being built.
!  MESSAGE
!  ERROR Choose a valid configuration.
!ENDIF

# set default config
!IF "$(CFG)" == ""
CFG = Win32-Release
!ENDIF

# -------------
# Common macros
# -------------

INCLUDES=/I "..\..\\" /I "..\src" /I "..\..\..\dependencies\include"
LIBS=libeay32.lib ssleay32.lib ZNC.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ws2_32.lib
LIBPATHS=/LIBPATH:"..\..\..\build-temp\ZNC_DLL\$(CFG)"
DEFINES=/D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" \
 /D "WIN_MSVC" /D "_MODULES" /D "HAVE_LIBSSL" /D "HAVE_IPV6" /D "HAVE_C_ARES"
CXXFLAGS=/c /W3 /EHsc /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /NOLOGO /DYNAMICBASE /NXCOMPAT

RSP=_ZNCModules.rsp

# Intermediate directory for .obj and .dll files
INTDIR="..\..\..\build-temp\Modules\$(CFG)\\"
# WARNING! This path must NOT end with '\' else NMAKE will complain!
SRCDIR=..\..\modules
# same as INTDIR, but without the trailing '\'
MAKDIR=..\..\..\build-temp\Modules\$(CFG)

BUILDOUT=..\..\..\build-out\$(CFG)\modules

# -----------------------------------
# Split configuration specific macros
# -----------------------------------

# Win32-Release configuration (default)
PLATFORM=x86
PLATFORM_CFG=release

# Win32-Debug configuration
!IF "$(CFG)" == "Win32-Debug"
PLATFORM_CFG=debug
!ENDIF

# x64-Release configuration
!IF "$(CFG)" == "x64-Release"
PLATFORM=x64
!ENDIF

# x64-Debug configuration
!IF "$(CFG)" == "x64-Debug"
PLATFORM=x64
PLATFORM_CFG=debug
!ENDIF

# -----------------------------
# Configuration specific macros
# -----------------------------

!IF "$(PLATFORM_CFG)" == "release"
DEFINES=$(DEFINES) /D "NDEBUG"
CXXFLAGS=$(CXXFLAGS) /O2 /Oi /GL /Gy /MD
LINKFLAGS=$(LINKFLAGS) /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /MACHINE:$(PLATFORM)
LIBPATHS=$(LIBPATHS) /LIBPATH:"..\..\..\dependencies\lib_$(PLATFORM)\release"
!ENDIF

!IF "$(PLATFORM_CFG)" == "debug"
DEFINES=$(DEFINES) /D "_DEBUG"
CXXFLAGS=$(CXXFLAGS) /Od /Gm /MDd /RTC1 /Zi
LINKFLAGS=$(LINKFLAGS) /INCREMENTAL /DEBUG /MACHINE:$(PLATFORM)
LIBPATHS=$(LIBPATHS) /LIBPATH:"..\..\..\dependencies\lib_$(PLATFORM)\debug" \
/LIBPATH:"..\..\..\dependencies\lib_$(PLATFORM)\release"
!ENDIF


# --------------------
# List of target files
# --------------------

# how to get the following two lists:
# cd znc-msvc\trunk\main\znc-msvc\win32\project
# Modules_get_make_lists > temp.txt
# copy here & adjust (remove leading new line and last backslash)

DLLS=$(MAKDIR)\admin.dll \
$(MAKDIR)\adminlog.dll \
$(MAKDIR)\autoattach.dll \
$(MAKDIR)\autocycle.dll \
$(MAKDIR)\autoop.dll \
$(MAKDIR)\autoreply.dll \
$(MAKDIR)\away.dll \
$(MAKDIR)\awaynick.dll \
$(MAKDIR)\blockuser.dll \
$(MAKDIR)\buffextras.dll \
$(MAKDIR)\certauth.dll \
$(MAKDIR)\chansaver.dll \
$(MAKDIR)\clientnotify.dll \
$(MAKDIR)\crypt.dll \
$(MAKDIR)\disconkick.dll \
$(MAKDIR)\fail2ban.dll \
$(MAKDIR)\fixfreenode.dll \
$(MAKDIR)\keepnick.dll \
$(MAKDIR)\kickrejoin.dll \
$(MAKDIR)\lastseen.dll \
$(MAKDIR)\nickserv.dll \
$(MAKDIR)\notes.dll \
$(MAKDIR)\partyline.dll \
$(MAKDIR)\perform.dll \
$(MAKDIR)\q.dll \
$(MAKDIR)\raw.dll \
$(MAKDIR)\route_replies.dll \
$(MAKDIR)\savebuff.dll \
$(MAKDIR)\schat.dll \
$(MAKDIR)\simple_away.dll \
$(MAKDIR)\stickychan.dll \
$(MAKDIR)\watch.dll \
$(MAKDIR)\webadmin.dll \
$(MAKDIR)\extra\antiidle.dll \
$(MAKDIR)\extra\autovoice.dll \
$(MAKDIR)\extra\block_motd.dll \
$(MAKDIR)\extra\ctcpflood.dll \
$(MAKDIR)\extra\fakeonline.dll \
$(MAKDIR)\extra\flooddetach.dll \
$(MAKDIR)\extra\imapauth.dll \
$(MAKDIR)\extra\listsockets.dll \
$(MAKDIR)\extra\log.dll \
$(MAKDIR)\extra\motdfile.dll \
$(MAKDIR)\extra\notify_connect.dll \
$(MAKDIR)\extra\send_raw.dll \
$(MAKDIR)\extra_win32\blockserver.dll \
$(MAKDIR)\extra_win32\block_lagchk.dll \
$(MAKDIR)\extra_win32\broadcastquery.dll \
$(MAKDIR)\extra_win32\certchecker.dll \
$(MAKDIR)\extra_win32\clearbuffersonmsg.dll \
$(MAKDIR)\extra_win32\clientpart.dll \
$(MAKDIR)\extra_win32\colloquy.dll \
$(MAKDIR)\extra_win32\fish.dll \
$(MAKDIR)\extra_win32\fix_lagchk.dll \
$(MAKDIR)\extra_win32\highlightattach.dll \
$(MAKDIR)\extra_win32\identserver.dll \
$(MAKDIR)\extra_win32\ignore.dll \
$(MAKDIR)\extra_win32\noctcp.dll \
$(MAKDIR)\extra_win32\noreconnectonkill.dll \
$(MAKDIR)\extra_win32\notify.dll \
$(MAKDIR)\extra_win32\prowl.dll \
$(MAKDIR)\extra_win32\rawlog.dll \
$(MAKDIR)\extra_win32\twitter.dll \
$(MAKDIR)\extra_win32\watch_only_detached.dll

OBJS=$(MAKDIR)\admin.obj \
$(MAKDIR)\adminlog.obj \
$(MAKDIR)\autoattach.obj \
$(MAKDIR)\autocycle.obj \
$(MAKDIR)\autoop.obj \
$(MAKDIR)\autoreply.obj \
$(MAKDIR)\away.obj \
$(MAKDIR)\awaynick.obj \
$(MAKDIR)\blockuser.obj \
$(MAKDIR)\buffextras.obj \
$(MAKDIR)\certauth.obj \
$(MAKDIR)\chansaver.obj \
$(MAKDIR)\clientnotify.obj \
$(MAKDIR)\crypt.obj \
$(MAKDIR)\disconkick.obj \
$(MAKDIR)\fail2ban.obj \
$(MAKDIR)\fixfreenode.obj \
$(MAKDIR)\keepnick.obj \
$(MAKDIR)\kickrejoin.obj \
$(MAKDIR)\lastseen.obj \
$(MAKDIR)\nickserv.obj \
$(MAKDIR)\notes.obj \
$(MAKDIR)\partyline.obj \
$(MAKDIR)\perform.obj \
$(MAKDIR)\q.obj \
$(MAKDIR)\raw.obj \
$(MAKDIR)\route_replies.obj \
$(MAKDIR)\savebuff.obj \
$(MAKDIR)\schat.obj \
$(MAKDIR)\simple_away.obj \
$(MAKDIR)\stickychan.obj \
$(MAKDIR)\watch.obj \
$(MAKDIR)\webadmin.obj \
$(MAKDIR)\extra\antiidle.obj \
$(MAKDIR)\extra\autovoice.obj \
$(MAKDIR)\extra\block_motd.obj \
$(MAKDIR)\extra\ctcpflood.obj \
$(MAKDIR)\extra\fakeonline.obj \
$(MAKDIR)\extra\flooddetach.obj \
$(MAKDIR)\extra\imapauth.obj \
$(MAKDIR)\extra\listsockets.obj \
$(MAKDIR)\extra\log.obj \
$(MAKDIR)\extra\motdfile.obj \
$(MAKDIR)\extra\notify_connect.obj \
$(MAKDIR)\extra\send_raw.obj \
$(MAKDIR)\extra_win32\blockserver.obj \
$(MAKDIR)\extra_win32\block_lagchk.obj \
$(MAKDIR)\extra_win32\broadcastquery.obj \
$(MAKDIR)\extra_win32\certchecker.obj \
$(MAKDIR)\extra_win32\clearbuffersonmsg.obj \
$(MAKDIR)\extra_win32\clientpart.obj \
$(MAKDIR)\extra_win32\colloquy.obj \
$(MAKDIR)\extra_win32\fish.obj \
$(MAKDIR)\extra_win32\fix_lagchk.obj \
$(MAKDIR)\extra_win32\highlightattach.obj \
$(MAKDIR)\extra_win32\identserver.obj \
$(MAKDIR)\extra_win32\ignore.obj \
$(MAKDIR)\extra_win32\noctcp.obj \
$(MAKDIR)\extra_win32\noreconnectonkill.obj \
$(MAKDIR)\extra_win32\notify.obj \
$(MAKDIR)\extra_win32\prowl.obj \
$(MAKDIR)\extra_win32\rawlog.obj \
$(MAKDIR)\extra_win32\twitter.obj \
$(MAKDIR)\extra_win32\watch_only_detached.obj

# ----------------
# Makefile targets
# ----------------

build: _dir_check $(DLLS)

clean:
  echo Deleting intermediate files for configuration: $(CFG)
  if exist $(INTDIR) rmdir /S /Q $(INTDIR)

rebuild: clean build

_dir_check:
  if not exist $(INTDIR) md $(INTDIR)
  if not exist $(INTDIR)\extra md $(INTDIR)\extra
  if not exist $(INTDIR)\extra_win32 md $(INTDIR)\extra_win32
  if not exist $(BUILDOUT) md $(BUILDOUT)

# compile .obj files using inference rules
$(OBJS):


# ---------------
# Inference rules
# ---------------

# cpp => obj
{$(SRCDIR)}.cpp{$(MAKDIR)}.obj:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
  echo /Yc"stdafx.hpp" >>$(RSP)
  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR) >>$(RSP)
  echo /Fd$(INTDIR)vc90.pdb >>$(RSP)
  echo $< >>$(RSP)
  cl @$(RSP)
  del $(RSP)

# extra\ cpp => obj
{$(SRCDIR)\extra}.cpp{$(MAKDIR)\extra}.obj:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
  echo /Yc"stdafx.hpp" >>$(RSP)
  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR)extra\ >>$(RSP)
  echo /Fd$(INTDIR)vc90.pdb >>$(RSP)
  echo $< >>$(RSP)
  cl @$(RSP)
  del $(RSP)

# extra_win32\ cpp => obj
{$(SRCDIR)\extra_win32}.cpp{$(MAKDIR)\extra_win32}.obj:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
  echo /Yc"stdafx.hpp" >>$(RSP)
  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR)extra_win32\ >>$(RSP)
  echo /Fd$(INTDIR)vc90.pdb >>$(RSP)
  echo $< >>$(RSP)
  cl @$(RSP)
  del $(RSP)

# obj => dll
{$(MAKDIR)}.obj{$(MAKDIR)}.dll:
  if exist $(RSP) del $(RSP)
  echo /OUT:$(INTDIR)$@ >>$(RSP)
  echo $(LIBPATHS) >>$(RSP)
  echo $(LINKFLAGS) >>$(RSP)
  echo /PDB:$(INTDIR)$(@B).pdb >>$(RSP)
  echo $(LIBS) >>$(RSP)
  echo $< >>$(RSP)
  link @$(RSP)
  xcopy /y /c $(@R).dll $(BUILDOUT)
  if exist $(@R).pdb  xcopy /y /c $(@R).pdb $(BUILDOUT)
  del $(RSP)

# extra\ obj => dll
{$(MAKDIR)\extra}.obj{$(MAKDIR)\extra}.dll:
  if exist $(RSP) del $(RSP)
  echo /OUT:$(INTDIR)$@ >>$(RSP)
  echo $(LIBPATHS) >>$(RSP)
  echo $(LINKFLAGS) >>$(RSP)
  echo /PDB:$(INTDIR)extra\$(@B).pdb >>$(RSP)
  echo $(LIBS) >>$(RSP)
  echo $< >>$(RSP)
  link @$(RSP)
  xcopy /y /c $(@R).dll $(BUILDOUT)
  if exist $(@R).pdb xcopy /y /c $(@R).pdb $(BUILDOUT)
  del $(RSP)

# extra_win32\ obj => dll
{$(MAKDIR)\extra_win32}.obj{$(MAKDIR)\extra_win32}.dll:
  if exist $(RSP) del $(RSP)
  echo /OUT:$(INTDIR)$@ >>$(RSP)
  echo $(LIBPATHS) >>$(RSP)
  echo $(LINKFLAGS) >>$(RSP)
  echo /PDB:$(INTDIR)extra_win32\$(@B).pdb >>$(RSP)
  echo $(LIBS) >>$(RSP)
  echo $< >>$(RSP)
  link @$(RSP)
  xcopy /y /c $(@R).dll $(BUILDOUT)
  if exist $(@R).pdb xcopy /y /c $(@R).pdb $(BUILDOUT)
  del $(RSP)
