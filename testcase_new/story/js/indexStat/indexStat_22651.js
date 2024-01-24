/***************************************************************************
@Description : seqDB-22651:分区表中获取索引统计信息 
@Modify list : Zhao Xiaoni 2020/8/20
****************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = "cl_22651";
testConf.clOpt = { "ShardingKey": { "b": 1 }, "ShardingType": "hash", "AutoSplit": true };

main( test );

function test( testPara )
{
   var groups = commGetGroups ( db );
   commCreateIndex ( testPara.testCL, "index_22651", { "a": 1 } );
   var records = [];
   for( var i = 0; i < 200; i++ )
   {
      records.push( { "a": i } );
   }
   testPara.testCL.insert( records );

   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22651" } );
     
   var actResult = testPara.testCL.getIndexStat( "index_22651" ).toObj();
   delete( actResult.StatTimestamp );
   var expResult = { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22651", "Unique": false, "KeyPattern": { "a": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": groups.length, "DistinctValNum": [ 200 ], "MinValue": {"a": 0 }, "MaxValue": {"a": 199 }, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 200, "TotalRecords": 200 }; 
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }  
}


