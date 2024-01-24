/***************************************************************************
@Description : seqDB-22649:集合中无数据时，执行analyze后获取索引统计信息
@Modify list : Zhao Xiaoni 2020/8/20
****************************************************************************/
testConf.clName = "cl_22649";

main( test );

function test( testPara )
{
   commCreateIndex ( testPara.testCL, "index_22649", { "a": 1 } );
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22649" } );
     
   var actResult = testPara.testCL.getIndexStat( "index_22649" ).toObj();
   delete( actResult.StatTimestamp );
   var expResult = { "Collection": COMMCSNAME + "." + testConf.clName, "Index": "index_22649", "Unique": false, "KeyPattern": { "a": 1 }, "TotalIndexLevels": 1, "TotalIndexPages": 1, "DistinctValNum": [ 0 ], "MinValue": null, "MaxValue": null, "NullFrac": 0, "UndefFrac": 0, "SampleRecords": 0, "TotalRecords": 0 }; 
   if( !commCompareObject( expResult, actResult ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }  
}

