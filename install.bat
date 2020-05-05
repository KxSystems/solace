@echo off

IF "%QHOME%"=="" (
    ECHO Enviroment variable QHOME is NOT defined 
    EXIT /B
)

IF NOT EXIST %QHOME%\w64 (
    ECHO Installation destination %QHOME%\w64 does not exist
    EXIT /B
)

ECHO Copying scripts to %QHOME%
COPY script\* %QHOME%
ECHO Copying dll to %QHOME%\w64
COPY lib\* %QHOME%\w64\

ECHO Installation complete