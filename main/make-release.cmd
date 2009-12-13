@echo off


echo ************************* Making folders... **************************

if exist build-temp\x86 rd /s /q build-temp\x86
md build-temp\x86\znc
md build-temp\x86\znc\modules
md build-temp\x86\znc\modules\webadmin
if exist build-temp\x64 rd /s /q build-temp\x64
md build-temp\x64\znc
md build-temp\x64\znc\modules
md build-temp\x64\znc\modules\webadmin


echo ************************** Copying binaries... ***********************

copy build-out\Win32-Release\ZNC* build-temp\x86\znc
copy build-out\x64-Release\ZNC* build-temp\x64\znc
copy build-out\Win32-Release\modules\* build-temp\x86\znc\modules
copy build-out\x64-Release\modules\* build-temp\x64\znc\modules
copy build-out\ZNC_Service\Win32-Release\service_provider.dll build-temp\x86\znc
copy build-out\ZNC_Service\x64-Release\service_provider.dll build-temp\x64\znc
copy dependencies\lib_x86\release\*.dll build-temp\x86\znc
copy dependencies\lib_x64\release\*.dll build-temp\x64\znc


echo ********************** Copying support files... **********************

copy release\* build-temp\x86\znc
copy release\* build-temp\x64\znc


echo ****************** Copying webadmin skin files... ********************

echo .svn > build-temp\xcopyskipsvn.txt
xcopy znc-msvc\modules\webadmin build-temp\x86\znc\modules\webadmin /Q /S /EXCLUDE:build-temp\xcopyskipsvn.txt
xcopy znc-msvc\modules\webadmin build-temp\x64\znc\modules\webadmin /Q /S /EXCLUDE:build-temp\xcopyskipsvn.txt
del build-temp\xcopyskipsvn.txt


echo ********************** Making zip files... ***************************
rem Uses zip and unzip from GnuWin32
rem Download these packages:
rem http://gnuwin32.sourceforge.net/packages/zip.htm
rem http://gnuwin32.sourceforge.net/packages/unzip.htm
rem http://gnuwin32.sourceforge.net/packages/bzip2.htm
rem and put the binaries in the PATH

rem Use a VBScript to get a locale independent version of %date% as YYYY-MM-DD:
set TmpFile="build-temp\tmp-date.vbs"
echo > %TmpFile% WScript.Echo "set year=" + CStr(Year(Now))
echo >> %TmpFile% WScript.Echo "set month=" + Right(100 + Month(Now), 2)
echo >> %TmpFile% WScript.Echo "set day=" + Right(100 + Day(Now), 2)
cscript /nologo "%TmpFile%" > "build-temp\tmp-date.cmd"
call "build-temp\tmp-date.cmd"
del "build-temp\tmp-date.cmd"
del %TmpFile%

set thedate=%year%-%month%-%day%

echo Compressing [znc-binaries-x86-%thedate%.zip] ...
cd build-temp\x86
zip -r -9 -T -o -l -q ../../znc-binaries-x86-%thedate%.zip znc

echo Compressing [znc-binaries-x64-%thedate%.zip] ...
cd ..\x64
zip -r -9 -T -o -l -q ../../znc-binaries-x64-%thedate%.zip znc

cd ..\..

echo ***************************** Done! **********************************
PAUSE
