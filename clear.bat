@echo off

SET CURDIRPATH="%CD%"

call :IMPL "%CD%"


goto :EOF

:IMPL

cd %CURDIRPATH%
cd %1
IF EXIST ARMDbg rd ARMDbg /Q /S
IF EXIST ARMRel rd ARMRel /Q /S
IF EXIST Debug rd Debug /Q /S
IF EXIST Release rd Release /Q /S
IF EXIST DebugPS rd DebugPS /Q /S
IF EXIST ReleasePS rd ReleasePS /Q /S
IF EXIST VTune rd VTune /Q /S
IF EXIST *.ncb del *.ncb
IF EXIST *.aps del *.aps
IF EXIST *.vcb del *.vcb
IF EXIST *.VCL del *.VCL
IF EXIST *.VCO del *.VCO
IF EXIST *.i del *.i
IF EXIST *.vspscc del *.vspscc
IF EXIST *.user del *.user
IF EXIST *.suo del /A:H *.suo

goto :EOF

