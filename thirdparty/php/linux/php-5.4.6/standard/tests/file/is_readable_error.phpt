--TEST--
Test is_readable() function: error conditions
--FILE--
<?php
/* Prototype: bool is_readable ( string $filename );
   Description: Tells whether the filename is readable
*/

echo "*** Testing is_readable(): error conditions ***\n";
var_dump( is_readable() );  // args < expected
var_dump( is_readable(1, 2) );  // args > expected

echo "\n*** Testing is_readable() on non-existent file ***\n";
var_dump( is_readable(dirname(__FILE__)."/is_readable.tmp") );

echo "Done\n";
?>
--EXPECTF--
*** Testing is_readable(): error conditions ***

Warning: is_readable() expects exactly 1 parameter, 0 given in %s on line %d
NULL

Warning: is_readable() expects exactly 1 parameter, 2 given in %s on line %d
NULL

*** Testing is_readable() on non-existent file ***
bool(false)
Done
