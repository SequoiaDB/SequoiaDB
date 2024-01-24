import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function insertRecs ( dbcl, recsNum )
{
   if( typeof ( recsNum ) == "undefined" ) { recsNum = 100; }
   var doc = [];
   for( var i = 0; i < recsNum; i++ )
   {
      doc.push( { a: i, no: i, b: i, c: "test" + i } );
   }
   dbcl.insert( doc );
}
