--TEST--
Test get_html_translation_table() function : usage variations - unexpected table values
--SKIPIF--
<?php
if( substr(PHP_OS, 0, 3) == "WIN"){  
  die('skip Not for Windows');
}

if( !setlocale(LC_ALL, "en_US.UTF-8") ) {
  die('skip failed to set locale settings to "en-US.UTF-8"');
}
?>
--FILE--
<?php
/* Prototype  : array get_html_translation_table ( [int $table [, int $quote_style]] )
 * Description: Returns the internal translation table used by htmlspecialchars and htmlentities
 * Source code: ext/standard/html.c
*/

/*
 * test get_html_translation_table() with unexpected value for argument $table 
*/

//set locale to en_US.UTF-8
setlocale(LC_ALL, "en_US.UTF-8");

echo "*** Testing get_html_translation_table() : usage variations ***\n";
// initialize all required variables
$quote_style = ENT_COMPAT;

// get an unset variable
$unset_var = 10;
unset($unset_var);

// a resource variable 
$fp = fopen(__FILE__, "r");

// array with different values
$values =  array (

  // array values
  array(),
  array(0),
  array(1),
  array(1, 2),
  array('color' => 'red', 'item' => 'pen'),

  // boolean values
  true,
  false,
  TRUE,
  FALSE,

  // string values
  "string",
  'string',

  // objects
  new stdclass(),

  // empty string
  "",
  '',

  // null vlaues
  NULL,
  null,

  // resource var
  $fp,

  // undefined variable
  @$undefined_var,

  // unset variable
  @$unset_var
);


// loop through each element of the array and check the working of get_html_translation_table()
// when $table arugment is supplied with different values
echo "\n--- Testing get_html_translation_table() by supplying different values for 'table' argument ---\n";
$counter = 1;
for($index = 0; $index < count($values); $index ++) {
  echo "-- Iteration $counter --\n";
  $table = $values [$index];

  var_dump( get_html_translation_table($table) );
  var_dump( get_html_translation_table($table, $quote_style) );

  $counter ++;
}

// close resource
fclose($fp);

echo "Done\n";
?>
--EXPECTF--
*** Testing get_html_translation_table() : usage variations ***

--- Testing get_html_translation_table() by supplying different values for 'table' argument ---
-- Iteration 1 --

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL
-- Iteration 2 --

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL
-- Iteration 3 --

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL
-- Iteration 4 --

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL
-- Iteration 5 --

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, array given in %s on line %s
NULL
-- Iteration 6 --
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
-- Iteration 7 --
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
-- Iteration 8 --
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
-- Iteration 9 --
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
-- Iteration 10 --

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL
-- Iteration 11 --

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL
-- Iteration 12 --

Warning: get_html_translation_table() expects parameter 1 to be long, object given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, object given in %s on line %s
NULL
-- Iteration 13 --

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL
-- Iteration 14 --

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, string given in %s on line %s
NULL
-- Iteration 15 --
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
-- Iteration 16 --
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
-- Iteration 17 --

Warning: get_html_translation_table() expects parameter 1 to be long, resource given in %s on line %s
NULL

Warning: get_html_translation_table() expects parameter 1 to be long, resource given in %s on line %s
NULL
-- Iteration 18 --
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
-- Iteration 19 --
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
Done
