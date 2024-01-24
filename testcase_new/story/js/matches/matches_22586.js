/******************************************************************************
*@Description: [seqDB-22586]Using $isnull to updata records. chekout the results
               使用$isnull 更新，更新字段为索引字段
*@Modify list:
   2014-4-10 YiBang Ruan  Init
   2020-08-10 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_22586";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   cl.insert( { a: null, b: 1 } );
   cl.insert( { a: 1, b: 2 } );

   cl.createIndex( "aa", { a: 1 } );

   cl.update( { $set: { b: 2 } }, { a: { $isnull: 1 } }, { "": "aa" } );
   cl.update( { $set: { b: 1 } }, { a: { $isnull: 0 } }, { "": "aa" } );

   record1 = cl.find( { a: { $isnull: 1 } } ).sort( { a: 1 } );
   record2 = cl.find( { a: { $isnull: 0 } } ).sort( { a: 1 } );

   var expectationOne = [{ a: null, b: 2 }];
   var expectationTwo = [{ a: 1, b: 1 }];

   checkRec( record1, expectationOne );
   checkRec( record2, expectationTwo );
}
