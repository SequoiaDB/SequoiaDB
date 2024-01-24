/******************************************************************************
@Description : seqDB-13746:查询指定时间戳类型字段排序（包含不带索引、带索引）
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13746";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recs = [
      { a: { "$timestamp": "2000-01-01-01.01.01.100000" }, b: 2 },
      { a: { "$timestamp": "2000-01-01-01.01.01.000001" }, b: 1 },
      { a: { "$timestamp": "2011-11-30-17.04.01.123456" }, b: 4 },
      { a: { "$timestamp": "2010-12-31-17.04.01.123456" }, b: 3 }];
   cl.insert( recs );

   // 不走索引
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecs = [
      { a: { "$timestamp": "2000-01-01-01.01.01.000001" }, b: 1 },
      { a: { "$timestamp": "2000-01-01-01.01.01.100000" }, b: 2 },
      { a: { "$timestamp": "2010-12-31-17.04.01.123456" }, b: 3 },
      { a: { "$timestamp": "2011-11-30-17.04.01.123456" }, b: 4 }];
   commCompareResults( cursor, expRecs );

   // 索引扫描
   var indexName = "idx";
   cl.createIndex( indexName, { a: -1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": indexName } ).sort( { a: 1 } );
   commCompareResults( cursor, expRecs );
}
