/************************************************************************
*@Description:   seqDB-8081:使用$isnull:0查询，目标字段存在且为null，不走索引查询
                 seqDB-8083:使用$isnull:0查询，目标字段存在且不为null
                 seqDB-8085:使用$isnull:0查询，目标字段不存在
*@Author:  2016/5/20  xiaoni huang
*@Modifier： 2020/08/11 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "_8081_8083_8085";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var data = [{ a: 0 },
   { a: 1, b: null },
   { a: 2, b: "" }]

   cl.insert( data );
   var expectation = [{ a: 2, b: "" }];
   var record = cl.find( { b: { $isnull: 0 } } ).sort( { a: 1 } );
   checkRec( record, expectation );

   cl.createIndex( "index_8081_8083_8085", { b: 1 } );

   cl.update( { $set: { a: 3 } }, { b: { $isnull: 1 } } );
   cl.update( { $set: { a: 1 } }, { b: { $isnull: 0 } } );

   var record1 = cl.find( { b: { $isnull: 1 } } ).sort( { a: 1 } );
   var record2 = cl.find( { b: { $isnull: 0 } } ).sort( { a: 1 } );

   var expectForOne = [{ a: 3 }, { a: 3, b: null }];
   var expectForTwo = [{ a: 1, b: "" }];

   checkRec( record1, expectForOne );
   checkRec( record2, expectForTwo );
}
