@echo off&setlocal enabledelayedexpansion
set SCRIPTPATH=%~dp0
set INCLUDEPATH=!SCRIPTPATH!..\..\include
set LIBPATH=!SCRIPTPATH!..\..\lib\c\debug\dll
set BUILDPATH=!SCRIPTPATH!build
set COMMON=common
set STATIC=static
set /A ARGS_COUNT=0
FOR %%A in (%*) DO SET /A ARGS_COUNT+=1
if not "%ARGS_COUNT%"=="1" (
   echo Syntax: %~nx0 program
   goto :end
)
set PROGRAM=%1
set LANG=.c
set FULLPROGRAM=!PROGRAM!!LANG!

if exist !BUILDPATH! (
   goto :begin
)else (
   md !BUILDPATH!
)

:begin

for /r "%~dp0" %%f in (*.*) do (

   if "%%~nf%%~xf"=="!FULLPROGRAM!" (
      cl /Fo"!BUILDPATH!\!PROGRAM!.obj" /c "!SCRIPTPATH!\!FULLPROGRAM!" /I!SCRIPTPATH!\..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TC
      cl /Fo"!BUILDPATH!\!COMMON!.obj" /c "!SCRIPTPATH!\!COMMON!!LANG!" /I!SCRIPTPATH!\..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TC
      link /OUT:!BUILDPATH!\!PROGRAM!.exe /LIBPATH:!SCRIPTPATH!\..\..\lib\c\debug\dll sdbcd.lib !BUILDPATH!\!PROGRAM!.obj !BUILDPATH!\!COMMON!.obj /debug
      copy !SCRIPTPATH!\..\..\lib\c\debug\dll\sdbcd.dll !BUILDPATH!

      :: we are not going to build app with static libraray
      ::cl /Fo"!BUILDPATH!\!PROGRAM!!STATIC!.obj" /c "!SCRIPTPATH!\!FULLPROGRAM!" /I!SCRIPTPATH!\..\..\include /wd4047 /DSDB_STATIC_BUILD
      ::cl /Fo"!BUILDPATH!\!COMMON!!STATIC!.obj" /c "!SCRIPTPATH!\!COMMON!!LANG!" /I!SCRIPTPATH!\..\..\include /wd4047 /DSDB_STATIC_BUILD
      ::link /OUT:!BUILDPATH!\!PROGRAM!!STATIC!.exe /LIBPATH:!SCRIPTPATH!\..\..\lib\c\debug\static staticsdbcd.lib "!BUILDPATH!\!PROGRAM!!STATIC!.obj" "!BUILDPATH!\!COMMON!!STATIC!.obj" gdi32.lib advapi32.lib crypt32.lib ::user32.lib /debug /NODEFAULTLIB:LIBCMT
      
      goto :end
   )
)
echo Source File !PROGRAM!.c does not exist!

:end

