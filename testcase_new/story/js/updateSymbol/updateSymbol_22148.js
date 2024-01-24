/******************************************************************************
@Description : seqDB-22148: 匹配不到记录，upsert使用set更新字段  
modify list : 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22148";

main( test );

function test ( testPara )
{
   var expResult = [];
   var allTypeData = [2147483646, 9223372036854775806, 1.7E+30, { "$decimal": "123.456" }, "String", { "$oid": "123abcd00ef12358902300ef" }, true, { "$date": "2012-01-01" }, { "$timestamp": "2012-01-01-13.14.26.124233" }, { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, [1, "string"], { "$minKey": 1 }, { "$maxKey": 1 }, { "parent": { "child": 1 } }, [["child1", "child11"], ["child2", "child22"]]];
   //指定$field为不存在的字段b，匹配条件为不存在的字段c
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": allTypeData[i] } );
      expResult.push( { "a": allTypeData[i] } );
      testPara.testCL.upsert( { "$set": { "a": { "$field": "b" } } }, { "c": i } );
      expResult.push( { "c": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //指定$field为不存在的字段b，匹配条件为不存在的字段b
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i } );
      expResult.push( { "a": i } );
      testPara.testCL.upsert( { "$set": { "a": { "$field": "b" } } }, { "b": allTypeData[i] } );
      expResult.push( { "a": allTypeData[i], "b": allTypeData[i] } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
