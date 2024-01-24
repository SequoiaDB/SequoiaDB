--TEST--
Test prev() function : usage variation - Pass different data types as $array_arg
--FILE--
<?php
/* Prototype  : mixed prev(array $array_arg)
 * Description: Move array argument's internal pointer to the previous element and return it 
 * Source code: ext/standard/array.c
 */

/*
 * Pass different data types as $array_arg argument to prev() to test behaviour
 */

echo "*** Testing prev() : variation ***\n";

//get an unset variable
$unset_var = 10;
unset ($unset_var);

// get a class
class classA
{
  public function __toString() {
    return "Class A object";
  }
}

// heredoc string
$heredoc = <<<EOT
hello world
EOT;

// get a resource variable
$fp = fopen(__FILE__, "r");

// unexpected values to be passed to $array_arg argument
$inputs = array(

       // int data
/*1*/  0,
       1,
       12345,
       -2345,

       // float data
/*5*/  10.5,
       -10.5,
       12.3456789000e10,
       12.3456789000E-10,
       .5,

       // null data
/*10*/ NULL,
       null,

       // boolean data
/*12*/ true,
       false,
       TRUE,
       FALSE,
       
       // empty data
/*16*/ "",
       '',
       array(),

       // string data
/*19*/ "string",
       'string',
       $heredoc,
       
       // object data
/*22*/ new classA(),

       // undefined data
/*23*/ @$undefined_var,

       // unset data
/*24*/ @$unset_var,

       // resource variable
/*25*/ $fp
);

// loop through each element of $inputs to check the behavior of prev()
$iterator = 1;
foreach($inputs as $input) {
  echo "\n-- Iteration $iterator --\n";
  var_dump( prev($input) );
  $iterator++;
};

fclose($fp);
?>
===DONE===
--EXPECTF--
*** Testing prev() : variation ***

-- Iteration 1 --

Warning: prev() expects parameter 1 to be array, integer given in %s on line %d
NULL

-- Iteration 2 --

Warning: prev() expects parameter 1 to be array, integer given in %s on line %d
NULL

-- Iteration 3 --

Warning: prev() expects parameter 1 to be array, integer given in %s on line %d
NULL

-- Iteration 4 --

Warning: prev() expects parameter 1 to be array, integer given in %s on line %d
NULL

-- Iteration 5 --

Warning: prev() expects parameter 1 to be array, double given in %s on line %d
NULL

-- Iteration 6 --

Warning: prev() expects parameter 1 to be array, double given in %s on line %d
NULL

-- Iteration 7 --

Warning: prev() expects parameter 1 to be array, double given in %s on line %d
NULL

-- Iteration 8 --

Warning: prev() expects parameter 1 to be array, double given in %s on line %d
NULL

-- Iteration 9 --

Warning: prev() expects parameter 1 to be array, double given in %s on line %d
NULL

-- Iteration 10 --

Warning: prev() expects parameter 1 to be array, null given in %s on line %d
NULL

-- Iteration 11 --

Warning: prev() expects parameter 1 to be array, null given in %s on line %d
NULL

-- Iteration 12 --

Warning: prev() expects parameter 1 to be array, boolean given in %s on line %d
NULL

-- Iteration 13 --

Warning: prev() expects parameter 1 to be array, boolean given in %s on line %d
NULL

-- Iteration 14 --

Warning: prev() expects parameter 1 to be array, boolean given in %s on line %d
NULL

-- Iteration 15 --

Warning: prev() expects parameter 1 to be array, boolean given in %s on line %d
NULL

-- Iteration 16 --

Warning: prev() expects parameter 1 to be array, string given in %s on line %d
NULL

-- Iteration 17 --

Warning: prev() expects parameter 1 to be array, string given in %s on line %d
NULL

-- Iteration 18 --
bool(false)

-- Iteration 19 --

Warning: prev() expects parameter 1 to be array, string given in %s on line %d
NULL

-- Iteration 20 --

Warning: prev() expects parameter 1 to be array, string given in %s on line %d
NULL

-- Iteration 21 --

Warning: prev() expects parameter 1 to be array, string given in %s on line %d
NULL

-- Iteration 22 --
bool(false)

-- Iteration 23 --

Warning: prev() expects parameter 1 to be array, null given in %s on line %d
NULL

-- Iteration 24 --

Warning: prev() expects parameter 1 to be array, null given in %s on line %d
NULL

-- Iteration 25 --

Warning: prev() expects parameter 1 to be array, resource given in %s on line %d
NULL
===DONE===
