/******************************************************************************
@Description :  seqDB-22154: 使用pull指定field字段更新对象
                seqDB-22155: 使用pull指定field字段更新对象，field指定不同类型值   
@Modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22154_22155";

main( test );

function test ( testPara )
{
   //字段a为非数组
   var expResult = [{ "a": { "parent": { "child": 1 } }, "b": { "child": 1 } }];
   testPara.testCL.insert( { "a": { "parent": { "child": 1 } }, "b": { "child": 1 } } );
   testPara.testCL.update( { "$pull": { "a": { "$field": "b" } } } );

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为数组，$field字段为对象中的值
   expResult = [{ "a": [2, 3], "b": { "field": 1 } }];
   testPara.testCL.insert( { "a": [1, 2, 3], "b": { "field": 1 } } );
   testPara.testCL.update( { "$pull": { "a": { "$field": "b.field" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为对象数组，$field字段为嵌套对象中的对象
   expResult = [{ "a": [{ "key1": "value1", "key2": "value2" }], "b": { "field": { "key1": "value1" } } }];
   testPara.testCL.insert( { "a": [{ "key1": "value1" }, { "key1": "value1", "key2": "value2" }], "b": { "field": { "key1": "value1" } } } );
   testPara.testCL.update( { "$pull": { "a": { "$field": "b.field" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为对象数组，$field字段为数组对象中的对象
   expResult = [{ "a": [{ "key1": "value1", "key2": "value2" }], "b": [{ "key1": "value1" }] }];
   testPara.testCL.insert( { "a": [{ "key1": "value1" }, { "key1": "value1", "key2": "value2" }], "b": [{ "key1": "value1" }] } );
   testPara.testCL.update( { "$pull": { "a": { "$field": "b.0" } } } );

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //字段a为数组，$field字段覆盖其它所有类型
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": allTypeData, "b": allTypeData[i], "c": i } );
      testPara.testCL.update( { "$pull": { "a": { "$field": "b" } } }, { "c": i } );
      var data = [].concat( allTypeData );
      data.splice( i, 1 );
      expResult.push( { "a": data, "b": allTypeData[i], "c": i } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
