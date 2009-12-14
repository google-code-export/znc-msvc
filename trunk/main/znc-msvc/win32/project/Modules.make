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
RSP=_ZNCModules.rsp
LIBPATHS=/LIBPATH:"..\..\..\build-temp\ZNC_DLL\$(CFG)" /LIBPATH:"..\..\..\dependencies\lib_x86\release"

# Intermediate directory for .obj and .dll files
INTDIR="..\..\..\build-temp\Modules\$(CFG)\\"
# WARNING! This path must NOT end with '\' else NMAKE will complain!
SRCDIR=..\..\modules
# same as INTDIR, but without the trailing '\'
MAKDIR=..\..\..\build-temp\Modules\$(CFG)

BUILDOUT=..\..\..\build-out\$(CFG)\modules


# -----------------------------
# Configuration specific macros
# -----------------------------

# Win32-Release configuration
!IF "$(VALID_CFG)" == "FALSE" || "$(CFG)" == "Win32-Release"
DEFINES=/D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "MODULES_ADMIN_EXPORTS" \
/D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "WIN_MSVC" /D "HAVE_LIBSSL" \
/D "HAVE_IPV6" /D "HAVE_ARES" /D "_WINDLL"
CXXFLAGS=/O2 /Oi /GL /Gy /EHsc /MD /W3 /c /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /NOLOGO \
/DYNAMICBASE /NXCOMPAT /MACHINE:X86
!ENDIF

# Win32-Debug configuration
!IF "$(CFG)" == "Win32-Debug"
DEFINES=/D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "MODULES_ADMIN_EXPORTS" \
/D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "WIN_MSVC" /D "HAVE_LIBSSL" \
/D "HAVE_IPV6" /D "HAVE_ARES" /D "_WINDLL"
CXXFLAGS=/Od /Gm /EHsc /MDd /RTC1 /W3 /c /Zi /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /INCREMENTAL /NOLOGO /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /DEBUG
!ENDIF

# x64-Release configuration
!IF "$(CFG)" == "x64-Release"
DEFINES=/D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "MODULES_ADMIN_EXPORTS" \
/D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "WIN_MSVC" /D "HAVE_LIBSSL" \
/D "HAVE_IPV6" /D "_WINDLL"
CXXFLAGS=/O2 /Oi /GL /Gy /EHsc /MD /W3 /c /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /NOLOGO \
/DYNAMICBASE /NXCOMPAT /MACHINE:X64
!ENDIF

# x64-Debug configuration
!IF "$(CFG)" == "x64-Debug"
DEFINES=/D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "MODULES_ADMIN_EXPORTS" \
/D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "_MODULES" /D "WIN_MSVC" /D "HAVE_LIBSSL" \
/D "HAVE_IPV6" /D "_WINDLL"
CXXFLAGS=/Od /Gm /EHsc /MDd /RTC1 /W3 /c /Zi /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /INCREMENTAL /NOLOGO /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /DEBUG
!ENDIF

# --------------------
# List of target files
# --------------------

DLLS=$(MAKDIR)\admin.dll \
$(MAKDIR)\adminlog.dll \
$(MAKDIR)\autoattach.dll \
$(MAKDIR)\autocycle.dll \
$(MAKDIR)\autoop.dll 
$(MAKDIR)\autoreply.dll \
$(MAKDIR)\away.dll \
$(MAKDIR)\awaynick.dll \
$(MAKDIR)\blockserver.dll \
$(MAKDIR)\block_lagchk.dll \
$(MAKDIR)\block_motd.dll \
$(MAKDIR)\buffextras.dll \
$(MAKDIR)\certauth.dll \
$(MAKDIR)\certchecker.dll \
$(MAKDIR)\chansaver.dll \
$(MAKDIR)\clientpart.dll \
$(MAKDIR)\colloquy.dll \
$(MAKDIR)\crypt.dll \
$(MAKDIR)\extra \
$(MAKDIR)\fail2ban.dll \
$(MAKDIR)\fish.dll \
$(MAKDIR)\fixfreenode.dll \
$(MAKDIR)\fix_lagchk.dll \
$(MAKDIR)\flooddetach.dll \
$(MAKDIR)\highlightattach.dll \
$(MAKDIR)\identserver.dll \
$(MAKDIR)\imapauth.dll \
$(MAKDIR)\keepnick.dll \
$(MAKDIR)\kickrejoin.dll \
$(MAKDIR)\nickserv.dll \
$(MAKDIR)\noctcp.dll \
$(MAKDIR)\notify.dll \
$(MAKDIR)\partyline.dll \
$(MAKDIR)\perform.dll \
$(MAKDIR)\prowl.dll \
$(MAKDIR)\q.dll \
$(MAKDIR)\qauth.dll \
$(MAKDIR)\raw.dll \
$(MAKDIR)\rawlog.dll \
$(MAKDIR)\route_replies.dll \
$(MAKDIR)\savebuff.dll \
$(MAKDIR)\schat.dll \
$(MAKDIR)\simple_away.dll \
$(MAKDIR)\stickychan.dll \
$(MAKDIR)\twitter.dll \
$(MAKDIR)\watch.dll \
$(MAKDIR)\watch_only_detached.dll \
$(MAKDIR)\webadmin \
$(MAKDIR)\webadmin.dll \
$(MAKDIR)\extra\antiidle.dll \
$(MAKDIR)\extra\autovoice.dll \
$(MAKDIR)\extra\blockuser.dll \
$(MAKDIR)\extra\ctcpflood.dll \
$(MAKDIR)\extra\discon_kick.dll \
$(MAKDIR)\extra\fakeonline.dll \
$(MAKDIR)\extra\lastseen.dll \
$(MAKDIR)\extra\listsockets.dll \
$(MAKDIR)\extra\log.dll \
$(MAKDIR)\extra\motdfile.dll \
$(MAKDIR)\extra\notify_connect.dll \
$(MAKDIR)\extra\send_raw.dll

OBJS=$(MAKDIR)\admin.obj \
$(MAKDIR)\adminlog.obj \
$(MAKDIR)\autoattach.obj \
$(MAKDIR)\autocycle.obj \
$(MAKDIR)\autoop.obj 
$(MAKDIR)\autoreply.obj \
$(MAKDIR)\away.obj \
$(MAKDIR)\awaynick.obj \
$(MAKDIR)\blockserver.obj \
$(MAKDIR)\block_lagchk.obj \
$(MAKDIR)\block_motd.obj \
$(MAKDIR)\buffextras.obj \
$(MAKDIR)\certauth.obj \
$(MAKDIR)\certchecker.obj \
$(MAKDIR)\chansaver.obj \
$(MAKDIR)\clientpart.obj \
$(MAKDIR)\colloquy.obj \
$(MAKDIR)\crypt.obj \
$(MAKDIR)\extra \
$(MAKDIR)\fail2ban.obj \
$(MAKDIR)\fish.obj \
$(MAKDIR)\fixfreenode.obj \
$(MAKDIR)\fix_lagchk.obj \
$(MAKDIR)\flooddetach.obj \
$(MAKDIR)\highlightattach.obj \
$(MAKDIR)\identserver.obj \
$(MAKDIR)\imapauth.obj \
$(MAKDIR)\keepnick.obj \
$(MAKDIR)\kickrejoin.obj \
$(MAKDIR)\nickserv.obj \
$(MAKDIR)\noctcp.obj \
$(MAKDIR)\notify.obj \
$(MAKDIR)\partyline.obj \
$(MAKDIR)\perform.obj \
$(MAKDIR)\prowl.obj \
$(MAKDIR)\q.obj \
$(MAKDIR)\qauth.obj \
$(MAKDIR)\raw.obj \
$(MAKDIR)\rawlog.obj \
$(MAKDIR)\route_replies.obj \
$(MAKDIR)\savebuff.obj \
$(MAKDIR)\schat.obj \
$(MAKDIR)\simple_away.obj \
$(MAKDIR)\stickychan.obj \
$(MAKDIR)\twitter.obj \
$(MAKDIR)\watch.obj \
$(MAKDIR)\watch_only_detached.obj \
$(MAKDIR)\webadmin \
$(MAKDIR)\webadmin.obj \
$(MAKDIR)\extra\antiidle.obj \
$(MAKDIR)\extra\autovoice.obj \
$(MAKDIR)\extra\blockuser.obj \
$(MAKDIR)\extra\ctcpflood.obj \
$(MAKDIR)\extra\discon_kick.obj \
$(MAKDIR)\extra\fakeonline.obj \
$(MAKDIR)\extra\lastseen.obj \
$(MAKDIR)\extra\listsockets.obj \
$(MAKDIR)\extra\log.obj \
$(MAKDIR)\extra\motdfile.obj \
$(MAKDIR)\extra\notify_connect.obj \
$(MAKDIR)\extra\send_raw.obj \


# ----------------
# Makefile targets
# ----------------

build: _dir_check _pch $(DLLS)

clean:
  echo Deleting intermediate files for configuration: $(CFG)
  if exist $(INTDIR) rmdir /S /Q $(INTDIR)
  
rebuild: clean build

_dir_check:
  if not exist $(INTDIR) md $(INTDIR)
  if not exist $(INTDIR)\extra md $(INTDIR)\extra
  if not exist $(BUILDOUT) md $(BUILDOUT)
  
_pch:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
  echo /Yc"stdafx.hpp" >>$(RSP)
  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR) >>$(RSP)
  echo /Fd$(INTDIR)vc90.pdb >>$(RSP)
  echo ..\..\stdafx.cpp >>$(RSP)
  cl @$(RSP)
  del $(RSP)

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
