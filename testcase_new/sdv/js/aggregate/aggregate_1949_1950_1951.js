/************************************************************************
*@Description: seqDB-1949:$sort+$skip+$limit组合查询
               seqDB-1950:$limit+$skip+$limit+$skip组合查询
               seqDB-1951:$match+$match组合查询
*@Author: 2020-5-12  Zhao Xiaoni
************************************************************************/
testConf.clName = COMMCLNAME + "_1949_1950_1951";

main( test );

function test( testPara )
{
   for( var i = 10; i > 0; i-- )
   {
      testPara.testCL.insert( { "a": i, "b": ( i + 10 ) } );
   }

   var expResult = [ { "a": 2, "b": 12 }, { "a": 3, "b": 13 } ];
   var cursor = testPara.testCL.aggregate( { "$sort": { "a": 1 } }, { "$skip": 1 }, { "$limit": 2 } );
   commCompareResults ( cursor, expResult );

   expResult = [ { "a": 4, "b": 14 } ];
   //按照子操作的前后排序执行
   cursor = testPara.testCL.aggregate( { "$sort": { "a": 1 } }, { "$limit": 4 }, { "$skip": 1 }, { "$limit": 5 }, { "$skip": 2 } );
   commCompareResults ( cursor, expResult );

   expResult = [ { "a": 4, "b": 14 }, { "a": 5, "b": 15 } ];
   //按照子操作的前后排序执行
   cursor = testPara.testCL.aggregate( { "$sort": { "a": 1 } }, { "$skip": 1 },{ "$limit": 4 }, { "$skip": 2 }, { "$limit": 5 } );
   commCompareResults ( cursor, expResult );

   expResult = [ { "a": 6, "b": 16 } ];
   cursor = testPara.testCL.aggregate( { "$match": { "a": 6 } }, { "$match": { "b": 16 } } );
   commCompareResults ( cursor, expResult );
}

