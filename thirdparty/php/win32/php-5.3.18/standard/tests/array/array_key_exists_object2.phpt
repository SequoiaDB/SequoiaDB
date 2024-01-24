--TEST--
Test array_key_exists() function : object functionality - different visibilities
--FILE--
<?php
/* Prototype  : bool array_key_exists(mixed $key, array $search)
 * Description: Checks if the given key or index exists in the array 
 * Source code: ext/standard/array.c
 * Alias to functions: key_exists
 */

/*
 * Pass array_key_exists() an object with private and protected properties
 */

echo "*** Testing array_key_exists() : object functionality ***\n";

class myClass {
	public $var1;
	protected $var2;
	private $var3;
	
	function __construct($a, $b, $c = null) {
		$this->var1 = $a;
		$this->var2 = $b;
		if (!is_null($c)) {
			$this->var3 = $c;
		}
 	}
}

echo "\n-- Do not assign a value to \$class1->var3 --\n";
$class1 = new myClass ('a', 'b');
echo "\$key = var1:\n";
var_dump(array_key_exists('var1', $class1));
echo "\$key = var2:\n";
var_dump(array_key_exists('var2', $class1));
echo "\$key = var3:\n";
var_dump(array_key_exists('var3', $class1));
echo "\$class1:\n";
var_dump($class1);

echo "\n-- Assign a value to \$class2->var3 --\n";
$class2 = new myClass('x', 'y', 'z');
echo "\$key = var3:\n";
var_dump(array_key_exists('var3', $class2));
echo "\$class2:\n";
var_dump($class2);

echo "Done";
?>

--EXPECTF--
*** Testing array_key_exists() : object functionality ***

-- Do not assign a value to $class1->var3 --
$key = var1:
bool(true)
$key = var2:
bool(false)
$key = var3:
bool(false)
$class1:
object(myClass)#1 (3) {
  [%b|u%"var1"]=>
  %unicode|string%(1) "a"
  [%b|u%"var2":protected]=>
  %unicode|string%(1) "b"
  [%b|u%"var3":%b|u%"myClass":private]=>
  NULL
}

-- Assign a value to $class2->var3 --
$key = var3:
bool(false)
$class2:
object(myClass)#2 (3) {
  [%b|u%"var1"]=>
  %unicode|string%(1) "x"
  [%b|u%"var2":protected]=>
  %unicode|string%(1) "y"
  [%b|u%"var3":%b|u%"myClass":private]=>
  %unicode|string%(1) "z"
}
Done
