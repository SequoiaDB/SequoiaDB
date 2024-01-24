/******************************************************************************
@Description : seqDB-22145: 使用set更新不存在的对象 
               seqDB-22152: 使用pop更新不存在的对象 
               seqDB-22157: 使用pull更新不存在的对象
               seqDB-22162: 使用pull_by更新不存在的对象 
               seqDB-22167: 使用push更新不存在的对象 
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22145_22152_22157_22162_22167";

main( test );

function test ( testPara )
{
   //使用set更新不存在的对象
   var expResult = [{ "a": 1, "b": 1 }];
   testPara.testCL.insert( { "a": 1 } );
   testPara.testCL.update( { "$set": { "b": { "$field": "a" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );

   //使用pop更新不存在对象
   testPara.testCL.update( { "$pop": { "c": { "$field": "a" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );

   //使用pull更新不存在的对象
   testPara.testCL.update( { "$pull": { "c": { "$field": "a" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );

   //使用pull_by更新不存在的对象
   testPara.testCL.update( { "$pull_by": { "c": { "$field": "a" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );

   //使用push更新不存在的对象
   expResult = [{ "a": 1, "b": 1, "c": [1] }];
   testPara.testCL.update( { "$push": { "c": { "$field": "a" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
