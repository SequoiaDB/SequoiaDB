--TEST--
bug67436: E_STRICT instead of custom error handler

--INI--
error_reporting=-1

--FILE--
<?php

spl_autoload_register(function($classname) {
	if (in_array($classname, array('a','b','c'))) {
		require_once ($classname . '.php');
	}
});

a::staticTest();

$b = new b();
$b->test();

--EXPECTF--
Strict Standards: Declaration of b::test() should be compatible with a::test($arg = c::TESTCONSTANT) in %s/bug67436/b.php on line %d
b::test()
a::test(c::TESTCONSTANT)
