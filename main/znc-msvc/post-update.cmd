@echo off

rem params: PATH  DEPTH  REVISION  ERROR  CWD

echo #define REVISION %~3 > revision.h
echo #define REVISION_STR "%~3" >> revision.h