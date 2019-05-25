@echo off&setlocal enabledelayedexpansion
set SCRIPTPATH=%~dp0
set INCLUDEPATH=!SCRIPTPATH!..\..\include
set LIBPATH=!SCRIPTPATH!..\..\lib\cpp\debug\dll
set COMMON="common"
set /A ARGS_COUNT=0
FOR %%A in (%*) DO SET /A ARGS_COUNT+=1
if not "%ARGS_COUNT%"=="1" (
   echo Syntax: %~nx0 program
   goto :end
)
if not exist !SCRIPTPATH!\build (md !SCRIPTPATH!\build)
set PROGRAM=%1
set LANG=.cpp
set FULLPROGRAM=!PROGRAM!!LANG!
for /r "%~dp0" %%f in (*.*) do (

   if "%%~nf%%~xf"=="!FULLPROGRAM!"  (
      cl /Fo"!SCRIPTPATH!\!PROGRAM!.obj" /c "!SCRIPTPATH!\!FULLPROGRAM!" /I!SCRIPTPATH!\..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
      cl /Fo"!SCRIPTPATH!\!COMMON!.obj" /c "!SCRIPTPATH!\!COMMON!!LANG!" /I!SCRIPTPATH!\..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
      link /OUT:!SCRIPTPATH!\build\!PROGRAM!.exe /LIBPATH:!SCRIPTPATH!\..\..\lib\cpp\debug\dll sdbcppd.lib "!SCRIPTPATH!\!PROGRAM!.obj" "!SCRIPTPATH!\!COMMON!.obj" /debug
      copy !SCRIPTPATH!\..\..\lib\cpp\debug\dll\sdbcppd.dll !SCRIPTPATH!\build
      goto :end
   )
)
echo Source File !PROGRAM!.cpp does not exist!

:end
