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

rem ToDo: fix %date% format, make it independent of regional settings

echo Compressing [znc-binaries-x86-%date%.zip] ...
cd build-temp\x86
zip -r -9 -T -o -l -q ../../znc-binaries-x86-%date%.zip znc

echo Compressing [znc-binaries-x64-%date%.zip] ...
cd ..\x64
zip -r -9 -T -o -l -q ../../znc-binaries-x64-%date%.zip znc

cd ..\..

echo ***************************** Done! **********************************
PAUSE
