#!/bin/bash

if [ $# != 1 ]; then
   echo "Please input php driver version, example: 5.4.6"
   exit 1
fi

ver=$1

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/type/typeTest.php
if [ $? != 0 ] ; then 
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/sdbTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/csTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/domainTest.php
if [ $? != 0 ] ; then
   exit 1
fi


../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/groupTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/sqlTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/configureTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/procedureTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/taskTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDB/sessionTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCS/csTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCS/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCollection/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCollection/indexTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCollection/recordTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCollection/lobTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaCursor/cursorTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaDomain/domainTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaGroup/groupTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaGroup/nodeTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaNode/nodeTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_testVersions/php_linux_$ver/bin/php -d extension=../build/dd/libsdbphp-$ver.so ../../../testcase_new/tdd/php/tools/phpunit.phar --log-junit phptest.error.txt ../../../testcase_new/tdd/php/SequoiaLob/lobTest.php
if [ $? != 0 ] ; then
   exit 1
fi

echo ------------------ All Successful! ------------------
