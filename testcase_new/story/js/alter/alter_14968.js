/* *****************************************************************************
@discretion: cl enable compression, then insert and query data
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclcompression_14968";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Compressed: false } );

   //enable compression
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      dbcl.enableCompression();
   } );
   //checkAlterResult( clName, "AttributeDesc", "Compressed" );
   //checkAlterResult( clName, "CompressionTypeDesc", "lzw" );

   //insert data and query data
   insertAndQueryRecs( dbcl );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}

function insertAndQueryRecs ( cl )
{
   var expRecs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var rec = { a: i, b: i, c: i };
      expRecs.push( rec );
   }
   cl.insert( expRecs );

   var rc = cl.find().sort( { a: 1 } );
   checkRec( rc, expRecs );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}
