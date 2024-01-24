/******************************************************************************
*@Description : seqDB-22405: IndexLevels/IndexPages/DistinctValNum字段测试  
*@author      : Zhao xiaoni
*@Date        : 2020-07-07
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = "cl_22405";

main( test );

function test ( testPara )
{
   var indexName = "index_22405";
   var indexDef = { "a.b.c": 1, "d.e.f": 1, "g.h.i": 1 };
   commCreateIndex( testPara.testCL, indexName, indexDef );

   var array = new Array( 1024 );
   array = array.join( "a" );
   var records = [];
   for( var i = 0; i < 1000; i++ )
   {
      records.push( { "a": { "b": { "c": array + i } }, "d": { "e": { "f": array + i } }, "g": { "h": { "i": array + i } } } );
   }
   testPara.testCL.insert( records );
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName, "SampleNum": 200 } );

   var count = 0;
   var expResult = { "TotalIndexLevels": 3, "DistinctValNum": [200, 200, 200], "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 200, "TotalRecords": 1000 };
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
