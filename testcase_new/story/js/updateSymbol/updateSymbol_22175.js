/******************************************************************************
@Description : seqDB-22175:使用set更新符，$field指定字段名/字段值和更新字段一致 
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22175";

main( test );

function test ( testPara )
{
   //指定$field字段和更新字段相同
   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": allTypeData[i], "b": allTypeData[i], "c": i } );
      expResult.push( { "a": allTypeData[i], "b": allTypeData[i], "c": i } );
      testPara.testCL.update( { "$set": { "a": { "$field": "a" } } }, { "c": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );

   //指定$field字段值和更新字段值相同，字段名不同
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.update( { "$set": { "a": { "$field": "b" } } }, { "c": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
