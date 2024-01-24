@echo off

set pandocFile="C:\Program Files (x86)\Pandoc\pandoc.exe"
set convertFile="md2troffConvertor\sundown-master\bin\md2TroffTool.exe"

if exist %pandocFile% (
   echo ��� Pandoc �ɹ�.
) else (
   echo %pandocFile% ������
   echo ��ȥ��װ ������:\soft\pandoc\pandoc-1.18-windows.msi
)

if exist %convertFile% (
   echo ��� ConvertFile �ɹ�.
) else (
   echo %convertFile% ������
   cd md2troffConvertor\sundown-master
   call scons
   cd ..\..\

   echo %convertFile% �������
)

:again

set file=

@echo.
@echo.

set/p file=���Ҫת�����ļ��Ͻ���(�˳�ֱ�Ӱ��س�):

if "%file%"=="" (
   echo �����Ҫת�����ļ��Ͻ���
   exit /B 1
)

set filePath=
set fileDir=
set fileName=
set fileExt=
set fileFullName=
set filePathNoExt=

for /f "delims=" %%f in ("%file%") do (
    set "filePath=%%f"
    set "fileDir=%%~dpf"
    set "fileName=%%~nf"
    set "fileExt=%%~xf"
    set "fileFullName=%%~nxf"
    set "filePathNoExt=%%~dpnf"
)

if "%fileExt%"==".md" (
   echo ��� ��չ�� �ɹ�
) else (
   echo ��Ч��md�ļ�
   exit /B 1
)

set output= %filePathNoExt%.troff

cd md2troffConvertor

call python md2TroffTool.py -i %file% -o %output%

cd ..

echo �������
echo INPUT : %file%
echo OUTPUT: %output%

goto again