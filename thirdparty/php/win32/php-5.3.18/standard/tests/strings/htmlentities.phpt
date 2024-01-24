--TEST--
HTML entities
--INI--
output_handler=
--FILE--
<?php 
setlocale (LC_CTYPE, "C");
$sc_encoded = htmlspecialchars ("<>\"&��\n");
echo $sc_encoded;
$ent_encoded = htmlentities ("<>\"&��\n");
echo $ent_encoded;
echo html_entity_decode($sc_encoded);
echo html_entity_decode($ent_encoded);
?>
--EXPECT--
&lt;&gt;&quot;&amp;��
&lt;&gt;&quot;&amp;&aring;&Auml;
<>"&��
<>"&��
