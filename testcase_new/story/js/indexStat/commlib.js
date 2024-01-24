import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}
