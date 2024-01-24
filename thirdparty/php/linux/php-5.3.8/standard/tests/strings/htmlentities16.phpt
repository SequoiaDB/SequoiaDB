--TEST--
htmlentities() test 16 (mbstring / cp1251)
--INI--
output_handler=
--SKIPIF--
<?php
	extension_loaded("mbstring") or die("skip mbstring not available\n");
	if (!@mb_internal_encoding('cp1251') ||
		@htmlentities("\x88\xa9\xd2\xcf\xd3\xcb\xcf\xdb\xce\xd9\xca", ENT_QUOTES, '') == '') {
		die("skip cp1251 character set is not available in this build.\n");
	}
?>
--FILE--
<?php
mb_internal_encoding('cp1251');
$str = "\x88\xa9\xf0\xee\xf1\xea\xee\xf8\xed\xfb\xe9";
var_dump(bin2hex($str), htmlentities($str, ENT_QUOTES, ''));
?>
===DONE===
--EXPECT--
string(22) "88a9f0eef1eaeef8edfbe9"
string(75) "&euro;&copy;&#1088;&#1086;&#1089;&#1082;&#1086;&#1096;&#1085;&#1099;&#1081;"
===DONE===
