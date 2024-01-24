@echo off

if "%1"=="" (
   echo 请输入PHP驱动版本号，如 5.4.15
   goto ERR
)

set ver=%1%

echo Test PHP %ver% driver!

:START

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\type\typeTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\sdbTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\csTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\clTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\domainTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\groupTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\sqlTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\configureTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\authenticateTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\procedureTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\transactionTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\backupoffline.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\taskTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDB\sessionTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCS\csTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCS\clTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCollection\clTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCollection\indexTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCollection\recordTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCollection\lobTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaCursor\cursorTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaDomain\domainTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaGroup\groupTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaGroup\nodeTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaNode\nodeTest.php
if %errorlevel% NEQ 0 goto ERR

..\..\..\tools\server\php_testVersions\php_win_%ver%\php.exe -d extension=../build/dd/libsdbphp-%ver%.dll ..\..\test\php\tools\phpunit.phar --log-junit phptest.error.txt ..\..\test\php\SequoiaLob\lobTest.php
if %errorlevel% NEQ 0 goto ERR

goto SUCCESS

:ERR
echo ------------------ An Error! ------------------
goto END

:SUCCESS
echo ------------------ All Successful! ------------------

:END