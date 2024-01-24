/******************************************************************************
@Description : seqDB-22143: 使用set更新符，$field指定字段更新 
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22143";

main( test );

function test ( testPara )
{
   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData[i] } );
      expResult.push( { "a": allTypeData[i], "b": allTypeData[i] } );
      testPara.testCL.update( { "$set": { "a": { "$field": "b" } } }, { "a": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
