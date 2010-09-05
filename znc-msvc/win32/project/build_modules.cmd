@echo off

if [%1] == [] goto usage
if [%2] == [] goto usage

if not defined vcinstalldir goto vcprompt

if /i %1 == Win32-Release (
	call "%vcinstalldir%\vcvarsall.bat" x86
	nmake -f Modules.make CFG=%1 %2
	goto end
)

if /i %1 == Win32-Debug (
	call "%vcinstalldir%\vcvarsall.bat" x86
	nmake -f Modules.make CFG=%1 %2
	goto end
)

if /i %1 == x64-Release (
	call "%vcinstalldir%\vcvarsall.bat" amd64
	nmake -f Modules.make CFG=%1 %2
	goto end
)

if /i %1 == x64-Debug (
	call "%vcinstalldir%\vcvarsall.bat" amd64
	nmake -f Modules.make CFG=%1 %2
	goto end
)

goto end

:usage
echo Simple wrapper script for building ZNC modules
echo.
echo Usage: %~nx0 ^<config^> ^<target^>
echo.
echo where ^<config^> is: [ Win32-Release ^| Win32-Debug ^| x64-Release ^| x64-Debug ]
echo and   ^<target^> is: [ build ^| rebuild ^| clean ]
goto end

:vcprompt
echo This batch file must be executed from a Visual Studio or Windows SDK
echo command prompt!

:end
