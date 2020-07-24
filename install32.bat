@echo off

IF "%QHOME%"=="" (
    ECHO ERROR: Enviroment variable QHOME is NOT defined 
    EXIT /B
)

IF NOT EXIST %QHOME%\w32 (
    ECHO ERROR: Installation destination %QHOME%\w32 does not exist
    EXIT /B
)


IF EXIST q (
    ECHO Copying q script to %QHOME%
    COPY q\* %QHOME%
    IF %ERRORLEVEL% NEQ 0 (
        ECHO ERROR: Copy failed
        EXIT /B %ERRORLEVEL%
    )
)

IF EXIST lib (
    ECHO Copying DLL to %QHOME%\w32
    COPY lib\* %QHOME%\w32\
    IF %ERRORLEVEL% NEQ 0 (
        ECHO ERROR: Copy failed
        EXIT /B %ERRORLEVEL%
    )
)

ECHO Installation complete
