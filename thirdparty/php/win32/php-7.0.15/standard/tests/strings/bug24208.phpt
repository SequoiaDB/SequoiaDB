--TEST--
Bug #24208 (parse_str() is not working)
--FILE--
<?php
$a = $b = $c = "oops";
parse_str("a=1&b=2&c=3");
var_dump($a, $b, $c);
?>
--EXPECT--
string(1) "1"
string(1) "2"
string(1) "3"
