/************************************************************************
*@Description:   seqDB-8086:使用$isnull:1查询，目标字段存在且为null，不走索引查询
                 seqDB-8088:使用$isnull:1查询，目标字段存在且不为null，不走索引查询
                 seqDB-8090:使用$isnull:1查询，目标字段不存在
*@Author:  2016/5/20  xiaoni huang
*@Modifier: 2020/08/11 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "_8086_8088_8090";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   var data = [{ a: null, b: 1 },
   { b: 2 },
   { a: 2, b: 3 }];

   cl.insert( data );

   var record1 = cl.find( { a: { $isnull: 1 } } );
   var record2 = cl.find( { a: { $isnull: 0 } } );

   var expectForOne = [{ a: null, b: 1 }, { b: 2 }];
   var expectForTwo = [{ a: 2, b: 3 }];

   checkRec( record1, expectForOne );
   checkRec( record2, expectForTwo );
}
