--TEST--
Test usort() function : usage variations - Pass different data types as $cmp_function arg
--FILE--
<?php
/* Prototype  : bool usort(array $array_arg, string $cmp_function)
 * Description: Sort an array by values using a user-defined comparison function 
 * Source code: ext/standard/array.c
 */

/*
 * Pass different data types as $cmp_function argument to usort() to test behaviour
 */

echo "*** Testing usort() : usage variation ***\n";

// Class definition for object variable
class MyClass
{
  public function __toString()
  {
    return 'object';
  }
}

$array_arg = array(0 => 1, 1 => -1, 2 => 3, 3 => 10, 4 => 4, 5 => 2, 6 => 8, 7 => 5);

// Get an unset variable
$unset_var = 10;
unset ($unset_var);

// Get resource variable
$fp = fopen(__FILE__,'r');

// different values for $cmp_function
$inputs = array(

       // int data
/*1*/  0,
       1,
       12345,
       -2345,

       // float data
/*5*/  10.5,
       -10.5,
       10.1234567e8,
       10.7654321E-8,
       .5,

       // array data
/*10*/ array(),
       array(0),
       array(1),
       array(1, 2),
       array('color' => 'red', 'item' => 'pen'),

       // null data
/*15*/ NULL,
       null,

       // boolean data
/*17*/ true,
       false,
       TRUE,
       FALSE,

       // empty data
/*21*/ "",
       '',

       // string data
       "string",
       'string',

       // object data
/*25*/ new MyClass(),

       // resource data
       $fp,

       // undefined data
       @$undefined_var,

       // unset data
/*28*/ @$unset_var,
);

// loop through each element of $inputs to check the behavior of usort()
$iterator = 1;
foreach($inputs as $input) {
  echo "\n-- Iteration $iterator --\n";
  var_dump( usort($array_arg, $input) );
  $iterator++;
};

//closing resource
fclose($fp);
?>
===DONE===
--EXPECTF--
*** Testing usort() : usage variation ***

-- Iteration 1 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 2 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 3 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 4 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 5 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 6 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 7 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 8 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 9 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 10 --

Warning: usort() expects parameter 2 to be a valid callback, array must have exactly two members in %s on line %d
NULL

-- Iteration 11 --

Warning: usort() expects parameter 2 to be a valid callback, array must have exactly two members in %s on line %d
NULL

-- Iteration 12 --

Warning: usort() expects parameter 2 to be a valid callback, array must have exactly two members in %s on line %d
NULL

-- Iteration 13 --

Warning: usort() expects parameter 2 to be a valid callback, first array member is not a valid class name or object in %s on line %d
NULL

-- Iteration 14 --

Warning: usort() expects parameter 2 to be a valid callback, first array member is not a valid class name or object in %s on line %d
NULL

-- Iteration 15 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 16 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 17 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 18 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 19 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 20 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 21 --

Warning: usort() expects parameter 2 to be a valid callback, function '' not found or invalid function name in %s on line %d
NULL

-- Iteration 22 --

Warning: usort() expects parameter 2 to be a valid callback, function '' not found or invalid function name in %s on line %d
NULL

-- Iteration 23 --

Warning: usort() expects parameter 2 to be a valid callback, function 'string' not found or invalid function name in %s on line %d
NULL

-- Iteration 24 --

Warning: usort() expects parameter 2 to be a valid callback, function 'string' not found or invalid function name in %s on line %d
NULL

-- Iteration 25 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 26 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 27 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL

-- Iteration 28 --

Warning: usort() expects parameter 2 to be a valid callback, no array or string given in %s on line %d
NULL
===DONE===