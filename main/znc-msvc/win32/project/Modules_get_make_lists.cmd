@echo off

echo DLLS=
dir /b ..\..\modules\*.cpp | sed "s/cpp$/dll/" | sed "s/^/$(MAKDIR)\\/" | sed "s/$/ \\/"
dir /b ..\..\modules\extra\*.cpp | sed "s/cpp$/dll/" | sed "s/^/$(MAKDIR)\\extra\\/" | sed "s/$/ \\/"
dir /b ..\..\modules\extra_win32\*.cpp | sed "s/cpp$/dll/" | sed "s/^/$(MAKDIR)\\extra_win32\\/" | sed "s/$/ \\/"

echo.

echo OBJS=
dir /b ..\..\modules\*.cpp | sed "s/cpp$/obj/" | sed "s/^/$(MAKDIR)\\/" | sed "s/$/ \\/"
dir /b ..\..\modules\extra\*.cpp | sed "s/cpp$/obj/" | sed "s/^/$(MAKDIR)\\extra\\/" | sed "s/$/ \\/"
dir /b ..\..\modules\extra_win32\*.cpp | sed "s/cpp$/obj/" | sed "s/^/$(MAKDIR)\\extra_win32\\/" | sed "s/$/ \\/"

