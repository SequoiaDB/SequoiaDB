--TEST--
assert() - error - give assert nonsense string with quiet_eval on then off
--INI--
assert.active = 1
assert.warning = 1
assert.callback = f1
assert.bail = 0
assert.quiet_eval = 0
--FILE--
<?php
function f1() 
{
	echo "f1 called\n";
}

$sa = "threemeninaboat";

var_dump($r2=assert($sa));

var_dump($ra0 = assert_options(ASSERT_QUIET_EVAL, 1));

var_dump($r2=assert($sa));
--EXPECTF--
Notice: Use of undefined constant threemeninaboat - assumed 'threemeninaboat' in %s(9) : assert code on line 1
bool(true)
int(0)
bool(true)
