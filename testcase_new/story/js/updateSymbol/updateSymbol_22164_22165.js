/******************************************************************************
@Description : seqDB-22164: 使用push指定field字段更新对象
               seqDB-22165: 使用push指定field字段更新对象，field指定不同类型值    
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22164_22165";

main( test );

function test ( testPara )
{
   //字段a为非数组
   var expResult = [{ "a": 1, "b": 2 }];
   testPara.testCL.insert( { "a": 1, "b": 2 } );
   testPara.testCL.update( { "$push": { "a": { "$field": "b" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为嵌套对象，$field字段为对象
   expResult = [{ "a": { "child": [1, 2, 1] }, "b": 1 }];
   testPara.testCL.insert( { "a": { "child": [1, 2] }, "b": 1 } );
   testPara.testCL.update( { "$push": { "a.child": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为数组，$field字段覆盖其它所有类型
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": [], "b": allTypeData[i], "c": i } );
      testPara.testCL.update( { "$push": { "a": { "$field": "b" } } }, { "c": i } );
      expResult.push( { "a": [allTypeData[i]], "b": allTypeData[i], "c": i } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
