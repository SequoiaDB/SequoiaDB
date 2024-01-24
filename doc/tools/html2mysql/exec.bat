@echo off
%~dp0\..\..\..\tools\server\php_win\php.exe -d extension=php_mysqli.dll -dextension=php_openssl.dll -f "%~dp0\main.php"