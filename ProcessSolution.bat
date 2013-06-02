@echo off 
set DEST_PATH=c:\tmp

if not exist "%DEST_PATH%" call :CheckError mkdir "%DEST_PATH%"

call :MoveImpl "%~1\%~2" "%DEST_PATH%\%~2"

goto :EOF

rem ##########################################################################

:MoveImpl
  set DEST_DIR=%~dp2
  set DEST_FILE=%DEST_DIR%\%~nx1
  
  if not exist "%~1" call :ShowError Dir not exists: "%~1"

  if exist "%~1\.\" (
    for /F "delims=" %%S in ('"dir "%~1" /B "') do call :MoveImpl "%~1\%%~S" "%~2\%%~S"
    call :CheckError rd "%~1"
  ) else (  
    if not exist "%DEST_DIR%" call :CheckError mkdir "%DEST_DIR%"
    if exist "%DEST_FILE%" call :ShowError File already exists: "%DEST_FILE%"
    
    call :CheckError move "%~1" "%DEST_DIR%" > NUL
    echo move "%~1"
  )
goto :EOF

rem ##########################################################################
:DoPause
    if "%DO_NOT_PAUSE%" neq "true" pause
goto :EOF

rem ##########################################################################
:ShowError
   @echo.
   @echo ======================================================================
   @echo  ERROR: %*
   @echo ======================================================================
   @echo. 
   call :DoPause
   exit 1
goto :EOF

rem ##########################################################################
:CheckError
    %*
    if %ERRORLEVEL% neq 0 (
       call :ShowError %* 
    )
goto :EOF

rem ##########################################################################