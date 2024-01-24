/***************************************************************************
@Description : seqDB-22694:普通表中获取索引统计信息 
@Modify list : Zhao Xiaoni 2020/8/20
****************************************************************************/
testConf.clName = "cl_22694";

main( test );

function test( testPara )
{
   commCreateIndex ( testPara.testCL, "index_22694", { "a": 1 } );
   var records = [];
   for( var i = 0; i < 200; i++ )
   {
      records.push( { "a": i } );
   }
   testPara.testCL.insert( records );

   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22694" } );
     
   var actResult = testPara.testCL.getIndexStat( "index_22694" ).toObj();
   delete( actResult.StatTimestamp );
   var expResult = { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22694", "Unique": false, "KeyPattern": { "a": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [ 200 ], "MinValue": {"a": 0 }, "MaxValue": {"a": 199 }, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 200, "TotalRecords": 200 }; 
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }  
}

