/******************************************************************************
@Description : seqDB-13752:查询指定字段排序，且存在记录不包含排序字段（包含不带索引、带索引）
@Modify list :
               2015-01-17 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13752";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recs = [
      { a: 1, b: "string" },
      { b: "max" },
      { a: { "$binary": "aGVsbG8gd29ybGQ=" }, b: "min" },
      { b: "empty" }];
   cl.insert( recs );

   // 不走索引
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecs = [
      { b: "max" },
      { b: "empty" },
      { a: 1, b: "string" },
      { a: { "$binary": "aGVsbG8gd29ybGQ=" }, b: "min" }];
   commCompareResults( cursor, expRecs );

   // 走索引
   var indexName = "idx";
   cl.createIndex( indexName, { a: -1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": indexName } ).sort( { a: 1 } );
   var expRecs = [
      { b: "empty" },
      { b: "max" },
      { a: 1, b: "string" },
      { a: { "$binary": "aGVsbG8gd29ybGQ=" }, b: "min" }];
   commCompareResults( cursor, expRecs );
}
