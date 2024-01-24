--TEST--
Test array_reduce() function : variation - invalid parameters 
--FILE--
<?php
/* Prototype  : mixed array_reduce(array input, mixed callback [, int initial])
 * Description: Iteratively reduce the array to a single value via the callback. 
 * Source code: ext/standard/array.c
 * Alias to functions: 
 */

echo "*** Testing array_reduce() : variation - invalid parameters ***\n";


$array = array(1);

var_dump(array_reduce($array, "bogusbogus"));

var_dump(array_reduce("bogusarray", "max"));

var_dump(array_reduce(new stdClass(), "max"));

?>
===DONE===
--EXPECTF--
*** Testing array_reduce() : variation - invalid parameters ***

Warning: array_reduce() expects parameter 2 to be a valid callback, function 'bogusbogus' not found or invalid function name in %sarray_reduce_variation2.php on line %d
NULL

Warning: array_reduce() expects parameter 1 to be array, string given in %sarray_reduce_variation2.php on line %d
NULL

Warning: array_reduce() expects parameter 1 to be array, object given in %sarray_reduce_variation2.php on line %d
NULL
===DONE===