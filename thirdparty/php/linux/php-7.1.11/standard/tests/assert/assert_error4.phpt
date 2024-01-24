--TEST--
assert() - basic - Test recoverable error
--INI--
assert.active = 1
assert.warning = 1
assert.callback = f1
assert.quiet_eval = 0
assert.bail = 0
error_reporting = -1
display_errors = 1
--FILE--
<?php
$sa = "0 $ 0";
var_dump($r2 = assert($sa, "Describing what was asserted"));
--EXPECTF--

Fatal error: Uncaught ParseError: syntax error, unexpected '$', expecting ';' in %s(%d) : assert code:1
Stack trace:
#0 %s(%d): assert('0 $ 0', 'Describing what...')
#1 {main}

Next Error: Failure evaluating code: 
Describing what was asserted:"0 $ 0" in %s:%d
Stack trace:
#0 %s(%d): assert('0 $ 0', 'Describing what...')
#1 {main}
  thrown in %s on line %d
