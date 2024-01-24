--TEST--
Test get_html_translation_table() function : basic functionality - table as HTML_ENTITIES & diff quote_style
--SKIPIF--
<?php
if( substr(PHP_OS, 0, 3) != "WIN"){  
  die('skip only for Windows');
}

if( !setlocale(LC_ALL, "English_United States.1252") ) {
  die('skip failed to set locale settings to "English_United States.1252"');
}

?>
--FILE--
<?php
/* Prototype  : array get_html_translation_table ( [int $table [, int $quote_style]] )
 * Description: Returns the internal translation table used by htmlspecialchars and htmlentities
 * Source code: ext/standard/html.c
*/

/* Test get_html_translation_table() when table is specified as HTML_ENTITIES */

//set locale 
setlocale(LC_ALL, "English_United States.1252");


echo "*** Testing get_html_translation_table() : basic functionality ***\n";

// Calling get_html_translation_table() with default arguments
echo "-- with default arguments --\n";
var_dump( get_html_translation_table() );

// Calling get_html_translation_table() with all arguments
// $table as HTML_ENTITIES and different quote style
echo "-- with table = HTML_ENTITIES & quote_style = ENT_COMPAT --\n";
$table = HTML_ENTITIES;
$quote_style = ENT_COMPAT;
var_dump( get_html_translation_table($table, $quote_style) );

echo "-- with table = HTML_ENTITIES & quote_style = ENT_QUOTES --\n";
$quote_style = ENT_QUOTES;
var_dump( get_html_translation_table($table, $quote_style) );

echo "-- with table = HTML_ENTITIES & quote_style = ENT_NOQUOTES --\n";
$quote_style = ENT_NOQUOTES;
var_dump( get_html_translation_table($table, $quote_style) );


echo "Done\n";
?>
--EXPECTF--
*** Testing get_html_translation_table() : basic functionality ***
-- with default arguments --
array(4) {
  ["""]=>
  string(6) "&quot;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
  ["&"]=>
  string(5) "&amp;"
}
-- with table = HTML_ENTITIES & quote_style = ENT_COMPAT --
array(100) {
  ["�"]=>
  string(6) "&nbsp;"
  ["�"]=>
  string(7) "&iexcl;"
  ["�"]=>
  string(6) "&cent;"
  ["�"]=>
  string(7) "&pound;"
  ["�"]=>
  string(8) "&curren;"
  ["�"]=>
  string(5) "&yen;"
  ["�"]=>
  string(8) "&brvbar;"
  ["�"]=>
  string(6) "&sect;"
  ["�"]=>
  string(5) "&uml;"
  ["�"]=>
  string(6) "&copy;"
  ["�"]=>
  string(6) "&ordf;"
  ["�"]=>
  string(7) "&laquo;"
  ["�"]=>
  string(5) "&not;"
  ["�"]=>
  string(5) "&shy;"
  ["�"]=>
  string(5) "&reg;"
  ["�"]=>
  string(6) "&macr;"
  ["�"]=>
  string(5) "&deg;"
  ["�"]=>
  string(8) "&plusmn;"
  ["�"]=>
  string(6) "&sup2;"
  ["�"]=>
  string(6) "&sup3;"
  ["�"]=>
  string(7) "&acute;"
  ["�"]=>
  string(7) "&micro;"
  ["�"]=>
  string(6) "&para;"
  ["�"]=>
  string(8) "&middot;"
  ["�"]=>
  string(7) "&cedil;"
  ["�"]=>
  string(6) "&sup1;"
  ["�"]=>
  string(6) "&ordm;"
  ["�"]=>
  string(7) "&raquo;"
  ["�"]=>
  string(8) "&frac14;"
  ["�"]=>
  string(8) "&frac12;"
  ["�"]=>
  string(8) "&frac34;"
  ["�"]=>
  string(8) "&iquest;"
  ["�"]=>
  string(8) "&Agrave;"
  ["�"]=>
  string(8) "&Aacute;"
  ["�"]=>
  string(7) "&Acirc;"
  ["�"]=>
  string(8) "&Atilde;"
  ["�"]=>
  string(6) "&Auml;"
  ["�"]=>
  string(7) "&Aring;"
  ["�"]=>
  string(7) "&AElig;"
  ["�"]=>
  string(8) "&Ccedil;"
  ["�"]=>
  string(8) "&Egrave;"
  ["�"]=>
  string(8) "&Eacute;"
  ["�"]=>
  string(7) "&Ecirc;"
  ["�"]=>
  string(6) "&Euml;"
  ["�"]=>
  string(8) "&Igrave;"
  ["�"]=>
  string(8) "&Iacute;"
  ["�"]=>
  string(7) "&Icirc;"
  ["�"]=>
  string(6) "&Iuml;"
  ["�"]=>
  string(5) "&ETH;"
  ["�"]=>
  string(8) "&Ntilde;"
  ["�"]=>
  string(8) "&Ograve;"
  ["�"]=>
  string(8) "&Oacute;"
  ["�"]=>
  string(7) "&Ocirc;"
  ["�"]=>
  string(8) "&Otilde;"
  ["�"]=>
  string(6) "&Ouml;"
  ["�"]=>
  string(7) "&times;"
  ["�"]=>
  string(8) "&Oslash;"
  ["�"]=>
  string(8) "&Ugrave;"
  ["�"]=>
  string(8) "&Uacute;"
  ["�"]=>
  string(7) "&Ucirc;"
  ["�"]=>
  string(6) "&Uuml;"
  ["�"]=>
  string(8) "&Yacute;"
  ["�"]=>
  string(7) "&THORN;"
  ["�"]=>
  string(7) "&szlig;"
  ["�"]=>
  string(8) "&agrave;"
  ["�"]=>
  string(8) "&aacute;"
  ["�"]=>
  string(7) "&acirc;"
  ["�"]=>
  string(8) "&atilde;"
  ["�"]=>
  string(6) "&auml;"
  ["�"]=>
  string(7) "&aring;"
  ["�"]=>
  string(7) "&aelig;"
  ["�"]=>
  string(8) "&ccedil;"
  ["�"]=>
  string(8) "&egrave;"
  ["�"]=>
  string(8) "&eacute;"
  ["�"]=>
  string(7) "&ecirc;"
  ["�"]=>
  string(6) "&euml;"
  ["�"]=>
  string(8) "&igrave;"
  ["�"]=>
  string(8) "&iacute;"
  ["�"]=>
  string(7) "&icirc;"
  ["�"]=>
  string(6) "&iuml;"
  ["�"]=>
  string(5) "&eth;"
  ["�"]=>
  string(8) "&ntilde;"
  ["�"]=>
  string(8) "&ograve;"
  ["�"]=>
  string(8) "&oacute;"
  ["�"]=>
  string(7) "&ocirc;"
  ["�"]=>
  string(8) "&otilde;"
  ["�"]=>
  string(6) "&ouml;"
  ["�"]=>
  string(8) "&divide;"
  ["�"]=>
  string(8) "&oslash;"
  ["�"]=>
  string(8) "&ugrave;"
  ["�"]=>
  string(8) "&uacute;"
  ["�"]=>
  string(7) "&ucirc;"
  ["�"]=>
  string(6) "&uuml;"
  ["�"]=>
  string(8) "&yacute;"
  ["�"]=>
  string(7) "&thorn;"
  ["�"]=>
  string(6) "&yuml;"
  ["""]=>
  string(6) "&quot;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
  ["&"]=>
  string(5) "&amp;"
}
-- with table = HTML_ENTITIES & quote_style = ENT_QUOTES --
array(101) {
  ["�"]=>
  string(6) "&nbsp;"
  ["�"]=>
  string(7) "&iexcl;"
  ["�"]=>
  string(6) "&cent;"
  ["�"]=>
  string(7) "&pound;"
  ["�"]=>
  string(8) "&curren;"
  ["�"]=>
  string(5) "&yen;"
  ["�"]=>
  string(8) "&brvbar;"
  ["�"]=>
  string(6) "&sect;"
  ["�"]=>
  string(5) "&uml;"
  ["�"]=>
  string(6) "&copy;"
  ["�"]=>
  string(6) "&ordf;"
  ["�"]=>
  string(7) "&laquo;"
  ["�"]=>
  string(5) "&not;"
  ["�"]=>
  string(5) "&shy;"
  ["�"]=>
  string(5) "&reg;"
  ["�"]=>
  string(6) "&macr;"
  ["�"]=>
  string(5) "&deg;"
  ["�"]=>
  string(8) "&plusmn;"
  ["�"]=>
  string(6) "&sup2;"
  ["�"]=>
  string(6) "&sup3;"
  ["�"]=>
  string(7) "&acute;"
  ["�"]=>
  string(7) "&micro;"
  ["�"]=>
  string(6) "&para;"
  ["�"]=>
  string(8) "&middot;"
  ["�"]=>
  string(7) "&cedil;"
  ["�"]=>
  string(6) "&sup1;"
  ["�"]=>
  string(6) "&ordm;"
  ["�"]=>
  string(7) "&raquo;"
  ["�"]=>
  string(8) "&frac14;"
  ["�"]=>
  string(8) "&frac12;"
  ["�"]=>
  string(8) "&frac34;"
  ["�"]=>
  string(8) "&iquest;"
  ["�"]=>
  string(8) "&Agrave;"
  ["�"]=>
  string(8) "&Aacute;"
  ["�"]=>
  string(7) "&Acirc;"
  ["�"]=>
  string(8) "&Atilde;"
  ["�"]=>
  string(6) "&Auml;"
  ["�"]=>
  string(7) "&Aring;"
  ["�"]=>
  string(7) "&AElig;"
  ["�"]=>
  string(8) "&Ccedil;"
  ["�"]=>
  string(8) "&Egrave;"
  ["�"]=>
  string(8) "&Eacute;"
  ["�"]=>
  string(7) "&Ecirc;"
  ["�"]=>
  string(6) "&Euml;"
  ["�"]=>
  string(8) "&Igrave;"
  ["�"]=>
  string(8) "&Iacute;"
  ["�"]=>
  string(7) "&Icirc;"
  ["�"]=>
  string(6) "&Iuml;"
  ["�"]=>
  string(5) "&ETH;"
  ["�"]=>
  string(8) "&Ntilde;"
  ["�"]=>
  string(8) "&Ograve;"
  ["�"]=>
  string(8) "&Oacute;"
  ["�"]=>
  string(7) "&Ocirc;"
  ["�"]=>
  string(8) "&Otilde;"
  ["�"]=>
  string(6) "&Ouml;"
  ["�"]=>
  string(7) "&times;"
  ["�"]=>
  string(8) "&Oslash;"
  ["�"]=>
  string(8) "&Ugrave;"
  ["�"]=>
  string(8) "&Uacute;"
  ["�"]=>
  string(7) "&Ucirc;"
  ["�"]=>
  string(6) "&Uuml;"
  ["�"]=>
  string(8) "&Yacute;"
  ["�"]=>
  string(7) "&THORN;"
  ["�"]=>
  string(7) "&szlig;"
  ["�"]=>
  string(8) "&agrave;"
  ["�"]=>
  string(8) "&aacute;"
  ["�"]=>
  string(7) "&acirc;"
  ["�"]=>
  string(8) "&atilde;"
  ["�"]=>
  string(6) "&auml;"
  ["�"]=>
  string(7) "&aring;"
  ["�"]=>
  string(7) "&aelig;"
  ["�"]=>
  string(8) "&ccedil;"
  ["�"]=>
  string(8) "&egrave;"
  ["�"]=>
  string(8) "&eacute;"
  ["�"]=>
  string(7) "&ecirc;"
  ["�"]=>
  string(6) "&euml;"
  ["�"]=>
  string(8) "&igrave;"
  ["�"]=>
  string(8) "&iacute;"
  ["�"]=>
  string(7) "&icirc;"
  ["�"]=>
  string(6) "&iuml;"
  ["�"]=>
  string(5) "&eth;"
  ["�"]=>
  string(8) "&ntilde;"
  ["�"]=>
  string(8) "&ograve;"
  ["�"]=>
  string(8) "&oacute;"
  ["�"]=>
  string(7) "&ocirc;"
  ["�"]=>
  string(8) "&otilde;"
  ["�"]=>
  string(6) "&ouml;"
  ["�"]=>
  string(8) "&divide;"
  ["�"]=>
  string(8) "&oslash;"
  ["�"]=>
  string(8) "&ugrave;"
  ["�"]=>
  string(8) "&uacute;"
  ["�"]=>
  string(7) "&ucirc;"
  ["�"]=>
  string(6) "&uuml;"
  ["�"]=>
  string(8) "&yacute;"
  ["�"]=>
  string(7) "&thorn;"
  ["�"]=>
  string(6) "&yuml;"
  ["""]=>
  string(6) "&quot;"
  ["'"]=>
  string(5) "&#39;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
  ["&"]=>
  string(5) "&amp;"
}
-- with table = HTML_ENTITIES & quote_style = ENT_NOQUOTES --
array(99) {
  ["�"]=>
  string(6) "&nbsp;"
  ["�"]=>
  string(7) "&iexcl;"
  ["�"]=>
  string(6) "&cent;"
  ["�"]=>
  string(7) "&pound;"
  ["�"]=>
  string(8) "&curren;"
  ["�"]=>
  string(5) "&yen;"
  ["�"]=>
  string(8) "&brvbar;"
  ["�"]=>
  string(6) "&sect;"
  ["�"]=>
  string(5) "&uml;"
  ["�"]=>
  string(6) "&copy;"
  ["�"]=>
  string(6) "&ordf;"
  ["�"]=>
  string(7) "&laquo;"
  ["�"]=>
  string(5) "&not;"
  ["�"]=>
  string(5) "&shy;"
  ["�"]=>
  string(5) "&reg;"
  ["�"]=>
  string(6) "&macr;"
  ["�"]=>
  string(5) "&deg;"
  ["�"]=>
  string(8) "&plusmn;"
  ["�"]=>
  string(6) "&sup2;"
  ["�"]=>
  string(6) "&sup3;"
  ["�"]=>
  string(7) "&acute;"
  ["�"]=>
  string(7) "&micro;"
  ["�"]=>
  string(6) "&para;"
  ["�"]=>
  string(8) "&middot;"
  ["�"]=>
  string(7) "&cedil;"
  ["�"]=>
  string(6) "&sup1;"
  ["�"]=>
  string(6) "&ordm;"
  ["�"]=>
  string(7) "&raquo;"
  ["�"]=>
  string(8) "&frac14;"
  ["�"]=>
  string(8) "&frac12;"
  ["�"]=>
  string(8) "&frac34;"
  ["�"]=>
  string(8) "&iquest;"
  ["�"]=>
  string(8) "&Agrave;"
  ["�"]=>
  string(8) "&Aacute;"
  ["�"]=>
  string(7) "&Acirc;"
  ["�"]=>
  string(8) "&Atilde;"
  ["�"]=>
  string(6) "&Auml;"
  ["�"]=>
  string(7) "&Aring;"
  ["�"]=>
  string(7) "&AElig;"
  ["�"]=>
  string(8) "&Ccedil;"
  ["�"]=>
  string(8) "&Egrave;"
  ["�"]=>
  string(8) "&Eacute;"
  ["�"]=>
  string(7) "&Ecirc;"
  ["�"]=>
  string(6) "&Euml;"
  ["�"]=>
  string(8) "&Igrave;"
  ["�"]=>
  string(8) "&Iacute;"
  ["�"]=>
  string(7) "&Icirc;"
  ["�"]=>
  string(6) "&Iuml;"
  ["�"]=>
  string(5) "&ETH;"
  ["�"]=>
  string(8) "&Ntilde;"
  ["�"]=>
  string(8) "&Ograve;"
  ["�"]=>
  string(8) "&Oacute;"
  ["�"]=>
  string(7) "&Ocirc;"
  ["�"]=>
  string(8) "&Otilde;"
  ["�"]=>
  string(6) "&Ouml;"
  ["�"]=>
  string(7) "&times;"
  ["�"]=>
  string(8) "&Oslash;"
  ["�"]=>
  string(8) "&Ugrave;"
  ["�"]=>
  string(8) "&Uacute;"
  ["�"]=>
  string(7) "&Ucirc;"
  ["�"]=>
  string(6) "&Uuml;"
  ["�"]=>
  string(8) "&Yacute;"
  ["�"]=>
  string(7) "&THORN;"
  ["�"]=>
  string(7) "&szlig;"
  ["�"]=>
  string(8) "&agrave;"
  ["�"]=>
  string(8) "&aacute;"
  ["�"]=>
  string(7) "&acirc;"
  ["�"]=>
  string(8) "&atilde;"
  ["�"]=>
  string(6) "&auml;"
  ["�"]=>
  string(7) "&aring;"
  ["�"]=>
  string(7) "&aelig;"
  ["�"]=>
  string(8) "&ccedil;"
  ["�"]=>
  string(8) "&egrave;"
  ["�"]=>
  string(8) "&eacute;"
  ["�"]=>
  string(7) "&ecirc;"
  ["�"]=>
  string(6) "&euml;"
  ["�"]=>
  string(8) "&igrave;"
  ["�"]=>
  string(8) "&iacute;"
  ["�"]=>
  string(7) "&icirc;"
  ["�"]=>
  string(6) "&iuml;"
  ["�"]=>
  string(5) "&eth;"
  ["�"]=>
  string(8) "&ntilde;"
  ["�"]=>
  string(8) "&ograve;"
  ["�"]=>
  string(8) "&oacute;"
  ["�"]=>
  string(7) "&ocirc;"
  ["�"]=>
  string(8) "&otilde;"
  ["�"]=>
  string(6) "&ouml;"
  ["�"]=>
  string(8) "&divide;"
  ["�"]=>
  string(8) "&oslash;"
  ["�"]=>
  string(8) "&ugrave;"
  ["�"]=>
  string(8) "&uacute;"
  ["�"]=>
  string(7) "&ucirc;"
  ["�"]=>
  string(6) "&uuml;"
  ["�"]=>
  string(8) "&yacute;"
  ["�"]=>
  string(7) "&thorn;"
  ["�"]=>
  string(6) "&yuml;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
  ["&"]=>
  string(5) "&amp;"
}
Done
