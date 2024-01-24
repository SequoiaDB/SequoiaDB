/******************************************************************************
@Description :  seqDB-22150: 使用pop指定field字段更新对象，field指定不同类型值  
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22150";

main( test );

function test ( testPara )
{
   //b字段为0
   var expResult = [{ "a": [1, 2, 3], "b": 0 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": 0 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //b字段为正数
   expResult = [{ "a": [1, 2], "b": 1 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": 1 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //b字段为负数   
   expResult = [{ "a": [2, 3], "b": -1 }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": -1 } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //b字段为嵌套数组
   expResult = [{ "a": [1], "b": [[1, -1], [2, -2]] }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": [[1, -1], [2, -2]] } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b.1.0" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //b字段为嵌套对象
   expResult = [{ "a": [1, 2], "b": { "parent": { "child": 1 } } }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": { "parent": { "child": 1 } } } );
   testPara.testCL.update( { "$pop": { "a": { "$field": "b.parent.child" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
