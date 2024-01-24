--TEST--
Bug #74589 __DIR__ wrong for unicode character, UTF-8
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) != 'WIN' && PHP_OS != 'Linux') {
    die('skip Linux or Windows only');
}
?>
--INI--
internal_encoding=utf-8
--FILE--
<?php
/*
#vim: set fileencoding=utf-8
#vim: set encoding=utf-8
*/

$item = "bug74589_新建文件夹"; // utf-8 string
$dir = dirname(__FILE__) . DIRECTORY_SEPARATOR . $item;
$test_file = $dir . DIRECTORY_SEPARATOR . "test.php";

mkdir($dir);

file_put_contents($test_file,
"<?php
	var_dump(__DIR__);
	var_dump(__FILE__);
	var_dump(__DIR__ === dirname(__FILE__));");

$php = getenv('TEST_PHP_EXECUTABLE');

echo shell_exec("$php -n $test_file");

?>
===DONE===
--EXPECTF--	
string(%d) "%sbug74589_新建文件夹"
string(%d) "%sbug74589_新建文件夹%etest.php"
bool(true)
===DONE===
--CLEAN--
<?php
	$item = "bug74589_新建文件夹"; // utf-8 string
	$dir = dirname(__FILE__) . DIRECTORY_SEPARATOR . $item;
	$test_file = $dir . DIRECTORY_SEPARATOR . "test.php";
	unlink($test_file);
	rmdir($dir);
?>

