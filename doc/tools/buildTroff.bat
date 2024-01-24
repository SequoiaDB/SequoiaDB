@echo off

set pandocFile="C:\Program Files (x86)\Pandoc\pandoc.exe"
set convertFile="md2troffConvertor\sundown-master\bin\md2TroffTool.exe"

if exist %pandocFile% (
   echo 检查 Pandoc 成功.
) else (
   echo %pandocFile% 不存在
   echo 请去安装 共享盘:\soft\pandoc\pandoc-1.18-windows.msi
)

if exist %convertFile% (
   echo 检查 ConvertFile 成功.
) else (
   echo %convertFile% 不存在
   cd md2troffConvertor\sundown-master
   call scons
   cd ..\..\

   echo %convertFile% 编译完成
)

:again

set file=

@echo.
@echo.

set/p file=请把要转换的文件拖进来(退出直接按回车):

if "%file%"=="" (
   echo 必须把要转换的文件拖进来
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
   echo 检查 扩展名 成功
) else (
   echo 无效的md文件
   exit /B 1
)

set output= %filePathNoExt%.troff

cd md2troffConvertor

call python md2TroffTool.py -i %file% -o %output%

cd ..

echo 编译完成
echo INPUT : %file%
echo OUTPUT: %output%

goto again