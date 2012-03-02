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
DEFINES=/D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_CRT_SECURE_NO_WARNINGS" /D "NOMINMAX" /D "WIN_MSVC"
CXXFLAGS=/c /W3 /EHsc /TP /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /NOLOGO /DYNAMICBASE /NXCOMPAT

RSP=_ZNCModules.rsp

# Intermediate directory for .obj and .dll files
INTDIR=..\..\..\build-temp\Modules\$(CFG)\ 
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

!INCLUDE ModulesList.make

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
  echo /Fd$(INTDIR)vc100.pdb >>$(RSP)
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
  echo /Fd$(INTDIR)vc100.pdb >>$(RSP)
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
  echo /Fd$(INTDIR)vc100.pdb >>$(RSP)
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
