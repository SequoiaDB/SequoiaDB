/******************************************************************************
@Description : seqDB-13750:查询指定字段排序，排序字段为不同数据类型的排序（包含不带索引、带索引）
@Modify list :
               2015-01-17 pusheng Ding  Init
               2020-08-14 Zixian Yan    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13750";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recs = [
      { a: 1, b: 2147483647, type: "int" },
      { a: 2, b: 9223372036854775807, type: "longInt" },
      { a: 3, b: 1.1234e-12, type: "float" },
      { a: 4, b: '1234abcd', type: "string" },
      { a: 5, b: { "$oid": "123abcd00ef12358902300ef" }, type: "oid" },
      { a: 6, b: true, type: "boolean" },
      { a: 7, b: { "$date": "2015-01-17" }, type: "date" },
      { a: 8, b: { "$timestamp": "2015-01-17-10.59.30.124233" }, type: "timestamp" },
      { a: 9, b: { "$regex": "^张", "$options": "1" }, type: "regex" },
      { a: 10, b: { "subobj": "value" }, type: "object" },
      { a: 11, b: ["abc", 100, "def"], type: "array" },
      { a: 12, b: null, type: "null" },
      { a: 13, type: "empty" }];
   cl.insert( recs );

   // 不走索引
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { b: 1 } );
   var expRecs = [
      { a: 13, type: "empty" },
      { a: 12, b: null, type: "null" },
      { a: 3, b: 1.1234e-12, type: "float" },
      { a: 11, b: ["abc", 100, "def"], type: "array" },
      { a: 1, b: 2147483647, type: "int" },
      { a: 2, b: 9223372036854775807, type: "longInt" },
      { a: 4, b: '1234abcd', type: "string" },
      { a: 10, b: { "subobj": "value" }, type: "object" },
      { a: 5, b: { "$oid": "123abcd00ef12358902300ef" }, type: "oid" },
      { a: 6, b: true, type: "boolean" },
      { a: 7, b: { "$date": "2015-01-17" }, type: "date" },
      { a: 8, b: { "$timestamp": "2015-01-17-10.59.30.124233" }, type: "timestamp" },
      { a: 9, b: { "$regex": "^张", "$options": "1" }, type: "regex" }];
   commCompareResults( cursor, expRecs );

   // 走索引排序
   var indexName = "idx";
   cl.createIndex( indexName, { b: -1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": indexName } ).sort( { b: 1 } );
   commCompareResults( cursor, expRecs );
}
