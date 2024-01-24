--TEST--
Test get_html_translation_table() function : basic functionality - charset WINDOWS-1252
--FILE--
<?php
/* Prototype  : array get_html_translation_table ( [int $table [, int $quote_style [, string charset_hint]]] )
 * Description: Returns the internal translation table used by htmlspecialchars and htmlentities
 * Source code: ext/standard/html.c
*/


echo "*** Testing get_html_translation_table() : basic functionality/Windows-1252 ***\n";

echo "-- with table = HTML_ENTITIES --\n";
$table = HTML_ENTITIES;
var_dump( get_html_translation_table($table, ENT_COMPAT, "WINDOWS-1252") );

echo "-- with table = HTML_SPECIALCHARS --\n";
$table = HTML_SPECIALCHARS; 
var_dump( get_html_translation_table($table, ENT_COMPAT, "WINDOWS-1252") );

echo "Done\n";
?>
--EXPECTF--
*** Testing get_html_translation_table() : basic functionality/Windows-1252 ***
-- with table = HTML_ENTITIES --
array(125) {
  ["�"]=>
  string(6) "&euro;"
  ["�"]=>
  string(7) "&sbquo;"
  ["�"]=>
  string(6) "&fnof;"
  ["�"]=>
  string(7) "&bdquo;"
  ["�"]=>
  string(8) "&hellip;"
  ["�"]=>
  string(8) "&dagger;"
  ["�"]=>
  string(8) "&Dagger;"
  ["�"]=>
  string(6) "&circ;"
  ["�"]=>
  string(8) "&permil;"
  ["�"]=>
  string(8) "&Scaron;"
  ["�"]=>
  string(8) "&lsaquo;"
  ["�"]=>
  string(7) "&OElig;"
  ["�"]=>
  string(7) "&lsquo;"
  ["�"]=>
  string(7) "&rsquo;"
  ["�"]=>
  string(7) "&ldquo;"
  ["�"]=>
  string(7) "&rdquo;"
  ["�"]=>
  string(6) "&bull;"
  ["�"]=>
  string(7) "&ndash;"
  ["�"]=>
  string(7) "&mdash;"
  ["�"]=>
  string(7) "&tilde;"
  ["�"]=>
  string(7) "&trade;"
  ["�"]=>
  string(8) "&scaron;"
  ["�"]=>
  string(8) "&rsaquo;"
  ["�"]=>
  string(7) "&oelig;"
  ["�"]=>
  string(6) "&Yuml;"
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
  ["&"]=>
  string(5) "&amp;"
  ["""]=>
  string(6) "&quot;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
}
-- with table = HTML_SPECIALCHARS --
array(4) {
  ["&"]=>
  string(5) "&amp;"
  ["""]=>
  string(6) "&quot;"
  ["<"]=>
  string(4) "&lt;"
  [">"]=>
  string(4) "&gt;"
}
Done
