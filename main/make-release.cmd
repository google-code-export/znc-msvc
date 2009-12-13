@echo off

echo ************************* Making folders... **************************

md build-temp\x86\znc
md build-temp\x86\znc\modules
md build-temp\x64\znc
md build-temp\x64\znc\modules

echo ************************** Copying binaries... ***********************

copy build-out\Win32-Release\ZNC* build-temp\x86\znc
copy build-out\x64-Release\ZNC* build-temp\x64\znc
copy dependencies\lib_x86\release\*.dll build-temp\x86\znc
copy dependencies\lib_x64\release\*.dll build-temp\x64\znc
copy build-temp\ZNC_Service\Win32-Release\service_provider.dll build-temp\x86\znc
copy build-temp\ZNC_Service\x64-Release\service_provider.dll build-temp\x64\znc

echo ********************** Copying support files... **********************

copy release\* build-temp\x86\znc
copy release\* build-temp\x64\znc

echo ********************** Making zip files... ***************************
rem Uses http://gnuwin32.sourceforge.net/packages/zip.htm in the PATH

echo znc-binaries-x86-%date%.zip...
cd build-temp\x86
zip -r -9 -T -o -l -q ../../znc-binaries-x86-%date%.zip znc

echo znc-binaries-x64-%date%.zip...
cd ..\x64
zip -r -9 -T -o -l -q ../../znc-binaries-x64-%date%.zip znc

cd ..\..

echo ***************************** Done! **********************************
PAUSE
