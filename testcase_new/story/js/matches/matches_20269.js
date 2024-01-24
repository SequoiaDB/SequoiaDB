/******************************************************************************
@Description: [seqDB-20269] Query by $size with index;
              使用 $size 查询, 走索引查询
@Author: 2020/08/06 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_20269";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_20269";

   var data = [{ a: [4, 1, 9, 1], b: [4, 1, 5, 2] },
   { a: [2, 4, 5, 4], b: [1, 1, 3] },
   { a: [2, 4, 5, 4], b: [1, 1, 0] },
   { a: [2, 4, 5, 4], b: [1, 1, 9] }];

   cl.insert( data );
   cl.createIndex( indexName, { b: 1 } );

   var rc1 = cl.find( {}, { a: { $size: 1 }, b: { $size: 1 } } );
   var rc2 = cl.find( { a: { $size: 1, $et: 4 }, b: { $size: 1, $et: 3 } } ).hint( { "": indexName } );

   var expectationOne = [{ a: 4, b: 4 },
   { a: 4, b: 3 },
   { a: 4, b: 3 },
   { a: 4, b: 3 }];

   var expectationTwo = [{ a: [2, 4, 5, 4], b: [1, 1, 0] },
   { a: [2, 4, 5, 4], b: [1, 1, 3] },
   { a: [2, 4, 5, 4], b: [1, 1, 9] },];

   checkRec( rc1, expectationOne );
   checkRec( rc2, expectationTwo );
}
