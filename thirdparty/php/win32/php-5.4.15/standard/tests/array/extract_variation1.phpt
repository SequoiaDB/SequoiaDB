--TEST--
Test extract() function (variation 1) 
--FILE--
<?php

$val = 4;
$str = "John";

debug_zval_dump($val);
debug_zval_dump($str);

/* Extracting Global Variables */
var_dump(extract($GLOBALS, EXTR_REFS));
debug_zval_dump($val);
debug_zval_dump($str);

echo "\nDone";
?>

--EXPECTF--
long(4) refcount(2)
string(4) "John" refcount(2)
int(%d)
long(4) refcount(2)
string(4) "John" refcount(2)

Done
