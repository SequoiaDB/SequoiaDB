/************************************************************************
*@Description:      seqDB-8105:使用$not查询，value取1个值
                    seqDB-8108:使用$not查询，给定值为空（如{$not:[]}）
                    使用$not查询,  结合 {$lt: }& {$gt: }
*@Author:  2016/5/25  xiaoni huang
*@Modifier: 2020/08/05 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "_8105_8108";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var data = [{ a: 0 },
   { a: 2, b: 5 },
   { a: 2, b: null },
   { a: 4, b: 3 },
   { a: 2, b: 4 }];
   cl.insert( data );

   var rc1 = cl.find( { $not: [{ a: 2 }] } );
   var rc2 = cl.find( { $not: [] } );
   var rc3 = cl.find( { $not: [{ a: { $lt: 3 } }, { b: { $gt: 4 } }] } );

   var rc1Expect = [{ a: 0 }, { a: 4, b: 3 }];
   var rc2Expect = data;
   var rc3Expect = [{ a: 0 },
   { a: 2, b: null },
   { a: 4, b: 3 },
   { a: 2, b: 4 }];

   checkRec( rc1, rc1Expect );
   checkRec( rc2, rc2Expect );
   checkRec( rc3, rc3Expect );
}
