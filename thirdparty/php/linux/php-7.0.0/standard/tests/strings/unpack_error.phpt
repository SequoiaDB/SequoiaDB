--TEST--
Test unpack() function : error conditions 
--FILE--
<?php

/* Prototype  : array unpack  ( string $format  , string $data  )
 * Description: Unpack data from binary string
 * Source code: ext/standard/pack.c
*/

echo "*** Testing unpack() : error conditions ***\n";

echo "\n-- Testing unpack() function with no arguments --\n";
var_dump( unpack() );

echo "\n-- Testing unpack() function with more than expected no. of arguments --\n";
$extra_arg = 10;
var_dump(unpack("I", pack("I", 65534), $extra_arg));

echo "\n-- Testing unpack() function with invalid format character --\n";
$extra_arg = 10;
var_dump(unpack("G", pack("I", 65534)));
?>
===DONE===
--EXPECTF--
*** Testing unpack() : error conditions ***

-- Testing unpack() function with no arguments --

Warning: unpack() expects exactly 2 parameters, 0 given in %s on line %d
NULL

-- Testing unpack() function with more than expected no. of arguments --

Warning: unpack() expects exactly 2 parameters, 3 given in %s on line %d
NULL

-- Testing unpack() function with invalid format character --

Warning: unpack(): Invalid format type G in %s on line %d
bool(false)
===DONE===
