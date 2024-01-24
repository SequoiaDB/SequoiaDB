testcase=$1
installDir=/opt/sequoiadb
phpApp=${installDir}/tools/server/php/bin/php-bin
echo ${phpApp}
extensionPath=${installDir}/lib/phplib/libsdbphp-5.4.6.so
pharPath=./tools/phpunit.phar
logPath=./log/phptest.log

${phpApp} -d extension=${extensionPath} ${pharPath} --log-junit ${logPath} ${testcase}

echo ${phpApp} -d extension=${extensionPath} ${pharPath} --log-junit ${logPath} ${testcase} 
