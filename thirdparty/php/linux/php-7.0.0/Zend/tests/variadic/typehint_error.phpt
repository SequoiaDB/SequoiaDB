--TEST--
Variadic arguments enforce typehints
--FILE--
<?php

function test(array... $args) {
    var_dump($args);
}

test();
test([0], [1], [2]);
test([0], [1], 2);

?>
--EXPECTF--
array(0) {
}
array(3) {
  [0]=>
  array(1) {
    [0]=>
    int(0)
  }
  [1]=>
  array(1) {
    [0]=>
    int(1)
  }
  [2]=>
  array(1) {
    [0]=>
    int(2)
  }
}

Fatal error: Uncaught TypeError: Argument 3 passed to test() must be of the type array, integer given, called in %s:%d
Stack trace:
#0 %s(%d): test(Array, Array, 2)
#1 {main}
  thrown in %s on line %d
