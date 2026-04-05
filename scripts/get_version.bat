@echo off

set cmd="git rev-parse --short HEAD"
FOR /F "tokens=*" %%i IN (' %cmd% ') DO SET X=%%i

set tag="git tag --points-at HEAD"
FOR /F "tokens=*" %%i IN (' %tag% ') DO SET Y=%%i


echo -------------
echo git rev (short):
echo %X%
echo -------------
set FPATH=../Core/Inc/version.h
echo %FPATH%
echo #ifndef VERSION_H > %FPATH%
echo #define VERSION_H >> %FPATH%
echo. >> %FPATH% 
echo static const char firmware_version[] = "%X%"; >> %FPATH%
echo static const char firmware_tag[] = "%Y%"; >> %FPATH%
echo. >> %FPATH%
echo #endif >> %FPATH%
echo. >> %FPATH%
