--TEST--
money_format test
--SKIPIF--
<?php
	if (!function_exists('money_format') || !function_exists('setlocale')) {
		die("SKIP money_format - not supported\n");
	}

if (setlocale(LC_MONETARY, 'en_US') === false) {
	die('skip en_US locale not available');
}
?>
--FILE--
<?php
setlocale(LC_MONETARY, 'en_US');
var_dump( money_format("X%nY", 3.1415));
var_dump(money_format("AAAAA%n%n%n%n", NULL));
?>
--EXPECTF--
string(7) "X$3.14Y"

Warning: money_format(): Only a single %ci or %cn token can be used in %s on line %d
bool(false)
