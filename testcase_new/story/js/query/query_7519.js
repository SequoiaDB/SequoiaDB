/******************************************************************************
@Description : [seqDB-7519] index scan query when there's multiple predicates
               查询条件包含索引条件，且索引量不超过一个数据页
@Modify list :
               2014-08-07 pusheng Ding  Init
               2020-08-13 Zixian Yan Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_7519";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "Index_7519";
   var data = [{ a: 1, b: 10 }, { a: 2, b: 20 }];
   cl.insert( data );
   cl.createIndex( indexName, { a: 1, b: 1 } );

   var record = cl.find( { a: { $gte: 0 }, b: 10 } ).hint( { "": indexName } );
   var expectation = [{ a: 1, b: 10 }];
   checkRec( record, expectation );
}
