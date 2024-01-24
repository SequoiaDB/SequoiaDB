/******************************************************************************
@Description : seqDB-13747:查询指定日期类型字段排序（包含不带索引、带索引）
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
testConf.clName = COMMCLNAME + "_13747";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recs = [
      { a: { "$date": "2000-04-03" }, b: 2 },
      { a: { "$date": "2000-04-01" }, b: 1 },
      { a: { "$date": "2011-01-01" }, b: 4 },
      { a: { "$date": "2000-05-01" }, b: 3 }];
   cl.insert( recs );

   // 不走索引
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecs = [
      { a: { "$date": "2000-04-01" }, b: 1 },
      { a: { "$date": "2000-04-03" }, b: 2 },
      { a: { "$date": "2000-05-01" }, b: 3 },
      { a: { "$date": "2011-01-01" }, b: 4 }];
   commCompareResults( cursor, expRecs );

   // 索引扫描
   var indexName = "idx";
   cl.createIndex( indexName, { a: -1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": indexName } ).sort( { a: 1 } );
   commCompareResults( cursor, expRecs )
}
