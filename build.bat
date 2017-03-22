@echo off
setlocal ENABLEDELAYEDEXPANSION

for %%V in (14,12,11) do if exist "!VS%%V0COMNTOOLS!" call "!VS%%V0COMNTOOLS!..\..\VC\vcvarsall.bat" x86 && goto compile
echo Unable to detect Visual Studio path!

:compile
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
cl main.cpp /Fe"run.exe" /I"." /DNDEBUG /Ox /EHsc /nologo || goto error
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

goto :EOF

:error
echo FAILED
@%COMSPEC% /C exit 1 >nul
