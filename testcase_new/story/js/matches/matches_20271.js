/******************************************************************************
@Description: [seqDB-20271] Query by $isnull, if target field are not exists, query it with index.
              使用 $isnull 查询, 如果目标字段不存在, 走索引查询
@Author: 2020/08/05 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_20271";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_20271";

   var data = [];
   for( var x = 1; x < 4; x++ )
   {
      var y = ( 4 - x );
      data.push( { a: null, b: y } );
      data.push( { a: x, b: y } );
      data.push( { b: y } );
   }
   cl.insert( data );

   cl.createIndex( indexName, { a: 1 } );

   var rc1 = cl.find( { a: { $isnull: 0 } } );
   var rc2 = cl.find( { a: { $isnull: 1 } } ).hint( { "": indexName } );

   var expectationOne = [{ a: 1, b: 3 },
   { a: 2, b: 2 },
   { a: 3, b: 1 }];

   var expectationTwo = [{ b: 3 }, { b: 2 }, { b: 1 },
   { a: null, b: 3 },
   { a: null, b: 2 },
   { a: null, b: 1 }];

   checkRec( rc1, expectationOne );
   checkRec( rc2, expectationTwo );
}
