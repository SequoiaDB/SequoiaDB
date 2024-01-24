/******************************************************************************
*@Description : seqDB-22458: 数据组中构造大量索引统计信息，逐步删除的同时获取索引统计信息快照  
*@author      : Zhao xiaoni
*@Date        : 2020-07-07
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var clNames = [];
   for( var i = 0; i < 10; i++ )
   {
      var clName = "cl_22458_" + i;
      clNames.push( clName );
      var indexName = "index_22458_" + i;
      var indexDef = { "a.b.c": 1, "d.e.f": 1, "g.h.i": 1 };
      commDropCL( db, COMMCSNAME, clName );
      var cl = commCreateCL( db, COMMCSNAME, clName );
      commCreateIndex( cl, indexName, indexDef );

      var records = [];
      for( var j = 0; j < 1000; j++ )
      {
         records.push( { "a": { "b": { "c": j } }, "d": { "e": { "f": j } }, "g": { "h": { "i": j } } } );
      }
      cl.insert( records );
      db.analyze( { "Collection": COMMCSNAME + "." + clName, "SampleNum": 200 } );
   }

   var expResult = [].concat( clNames );
   for( var i = 0; i < 10; i++ )
   {
      var actResult = [];
      commDropCL( db, COMMCSNAME, clNames[i] );

      var cursor = db.snapshot( SDB_SNAP_INDEXSTATS, { "NodeSelect": "master" } );
      while( cursor.next() )
      {
         var collection = cursor.current().toObj().Collection;
         var clName = collection.split( "." )[1].toString();
         var index = cursor.current().toObj().Index.toString();
         if( clNames.indexOf( clName ) !== -1 && index !== "$id" )
         {
            actResult.push( clName );
         }
      }

      expResult.shift();
      if( actResult.length !== expResult.length )
      {
         throw new Error( "expResult: " + expResult + ", actResult: " + actResult );
      }
   }
}
