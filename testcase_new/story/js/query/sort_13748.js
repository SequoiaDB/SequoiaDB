/******************************************************************************
@Description : seqDB-13748:查询指定二进制类型字段排序（包含不带索引、带索引）
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13748";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recs = [
      { a: { "$binary": "Ym9vaw==" }, b: 3 },
      { a: { "$binary": "YWdyZWU=" }, b: 2 },
      { a: { "$binary": "ZG9n" }, b: 4 },
      { a: { "$binary": "Y2F0" }, b: 1 }];
   cl.insert( recs );

   // 不走索引
   var query1 = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecs = [
      { a: { "$binary": "Y2F0" }, b: 1 },
      { a: { "$binary": "YWdyZWU=" }, b: 2 },
      { a: { "$binary": "Ym9vaw==" }, b: 3 },
      { a: { "$binary": "ZG9n" }, b: 4 }];
   commCompareResults( query1, expRecs );

   // 索引扫描
   var indexName = "idx";
   cl.createIndex( indexName, { a: -1 } );
   var query2 = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": indexName } ).sort( { a: 1 } );
   commCompareResults( query2, expRecs );
}
