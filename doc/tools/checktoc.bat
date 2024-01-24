@echo off
%~dp0\..\..\tools\server\php_win\php.exe -d extension=php_mysqli.dll -f "%~dp0\checktoc.php"
pause