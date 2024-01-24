--TEST--
Bug #68335: rmdir doesnt work with file:// stream wrapper
--FILE--
<?php
$dir = 'file://' . dirname(__FILE__) . '/testDir';
mkdir($dir);
var_dump(is_dir($dir));
rmdir($dir);
var_dump(is_dir($dir));
?>
--EXPECT--
bool(true)
bool(false)
