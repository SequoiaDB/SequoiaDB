/******************************************************************************
*@Description : seqDB-22406: NullFrac字段测试 
*@author      : Zhao xiaoni
*@Date        : 2020-07-07
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = "cl_22406_22407";

main( test );

function test ( testPara )
{
   var indexName = "index_22406_22407";
   var indexDef = { "a.b.c": 1, "d.e.f": 1, "g.h.i": 1 };
   commCreateIndex( testPara.testCL, indexName, indexDef );

   var records = [];
   for( var i = 0; i < 1000; i++ )
   {
      records.push( { "a": { "b": { "c": null } }, "d": { "e": { "f": null } }, "g": { "h": { "i": null } } } );
      records.push( { "j": i } );
   }
   testPara.testCL.insert( records );
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName, "SampleNum": 2000 } );

   var count = 0;
   var expResult = { "TotalIndexLevels": 1, "DistinctValNum": [1, 1, 1], "NullFrac": 5000, "UndefFrac": 5000, "SampleRecords": 2000, "TotalRecords": 2000 };
   var cursor = db.snapshot( SDB_SNAP_INDEXSTATS, { "Collection": COMMCSNAME + "." + testConf.clName, "Index": indexName } );
   while( cursor.next() )
   {
      count++;
      var statInfo = cursor.current().toObj().StatInfo;
      var group = statInfo[0].Group;
      for( var i = 0; i < group.length; i++ )
      {
         var node = group[i];
         //以下参数不进行比较
         delete ( node.NodeName );
         delete ( node.StatTimestamp );
         delete ( node.MinValue );
         delete ( node.MaxValue );
         delete ( node.TotalIndexPages );
         if( !commCompareObject( expResult, node ) )
         {
            throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( node ) );
         }
      }
   }
   if( count == 0 )
   {
      throw new Error( "count: 0" );
   }
}
