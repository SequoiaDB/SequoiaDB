import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function getBulkData ( dataNum, idStartData, aStartData )
{
   if( idStartData === undefined ) { idStartData = 0; }
   if( aStartData === undefined ) { aStartData = 0; }

   var doc = [];
   for( var i = 0; i < dataNum; i++ )
   {
      doc.push( { "_id": idStartData + i, "a": aStartData + i } );
   }

   return doc;
}

