/******************************************************************************
@Description: seqDB-22146: 使用set更新多个对象
              seqDB-22153: 使用pop更新多个对象
              seqDB-22158: 使用pull更新多个对象 
              seqDB-22163: 使用pull_by更新多个对象 
              seqDB-22168: 使用push更新多个对象 
modify list: 2020-5-14  Zhao Xiaoni 
******************************************************************************/
testConf.clName = "cl_22146_22153_22158_22163_22168";

main( test );

function test ( testPara )
{
   //使用set更新多个对象
   var expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": i, "c": i, "d": allTypeData[i] } );
      expResult.push( { "a": i, "b": allTypeData[i], "c": allTypeData[i], "d": allTypeData[i] } );
      testPara.testCL.update( { "$set": { "b": { "$field": "d" }, "c": { "$field": "d" } } }, { "a": i } );
   }

   var cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //使用pop更新多个对象
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData, "c": allTypeData, "d": i } );
      testPara.testCL.update( { "$pop": { "b": { "$field": "d" }, "c": { "$field": "d" } } }, { "a": i } );
      var data = [].concat( allTypeData );
      data.splice( allTypeData.length - i, i );
      expResult.push( { "a": i, "b": data, "c": data, "d": i } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //使用pull更新多个对象
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData, "c": allTypeData, "d": allTypeData[i] } );
      testPara.testCL.update( { "$pull": { "b": { "$field": "d" }, "c": { "$field": "d" } } }, { "a": i } );
      var data = [].concat( allTypeData );
      data.splice( i, 1 );
      expResult.push( { "a": i, "b": data, "c": data, "d": allTypeData[i] } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //使用pull_by更新多个对象
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": allTypeData, "c": allTypeData, "d": allTypeData[i] } );
      testPara.testCL.update( { "$pull_by": { "b": { "$field": "d" }, "c": { "$field": "d" } } }, { "a": i } );
      var data = [].concat( allTypeData );
      data.splice( i, 1 );
      expResult.push( { "a": i, "b": data, "c": data, "d": allTypeData[i] } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
   testPara.testCL.remove();

   //使用push更新多个对象
   expResult = [];
   for( var i = 0; i < allTypeData.length; i++ )
   {
      testPara.testCL.insert( { "a": i, "b": [], "c": [], "d": allTypeData[i] } );
      testPara.testCL.update( { "$push": { "b": { "$field": "d" }, "c": { "$field": "d" } } }, { "a": i } );
      expResult.push( { "a": i, "b": [allTypeData[i]], "c": [allTypeData[i]], "d": allTypeData[i] } );
   }

   cursor = testPara.testCL.find();
   commCompareResults( cursor, expResult );
}
