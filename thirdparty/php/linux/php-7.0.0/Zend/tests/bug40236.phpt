--TEST--
Bug #40236 (php -a function allocation eats memory)
--SKIPIF--
<?php
if (php_sapi_name() != "cli") die("skip CLI only");
if (extension_loaded("readline")) die("skip Test doesn't support readline");
?>
--FILE--
<?php
$php = getenv('TEST_PHP_EXECUTABLE');
$cmd = "\"$php\" -n -d memory_limit=4M -a \"".dirname(__FILE__)."\"/bug40236.inc";
echo `$cmd`;
?>
--EXPECTF--
Interactive %s

ok
