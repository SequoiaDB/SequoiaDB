/******************************************************************************
@Description : seqDB-22147: 使用set更新对象为分区键  
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = "cl_22147";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash" };

main( test );

function test ( testPara )
{
   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData[i] } );
      expResult.push( { "a": i, "b": allTypeData[i] } );
      testPara.testCL.update( { "$set": { "a": { "$field": "b" } } }, { "a": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
