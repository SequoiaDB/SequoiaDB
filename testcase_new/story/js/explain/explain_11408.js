/************************************
*@Description: seqDB-11408: 有统计信息，匹配符走索引
*@author:      chimanzhao
*@createdate:  2020.5.15
*@testlinkCase: seqDB-11408
**************************************/
testConf.clName = COMMCLNAME + "_11408";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;

   dbcl.createIndex( "a", { a: 1 }, true );
   dbcl.createIndex( "b", { b: -1 }, true );

   //分别构造查询字段在mcv中统计的数据类型和不在mcv中统计的数据类型。

   var conds1 = [{ "a": { $mod: [250, 0] } }, { "a": { $ne: 250 } }];
   var conds2 = [{ "$and": [{ a: { $gt: 225 } }, { a: { $lt: 250 } }] }, { "$and": [{ a: { $gte: 0 } }, { a: { $lte: 5 } }] }];

   indexName = ""
   indexName_a = "a";
   scanType = "tbscan";
   scanType1 = "ixscan";

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: -i } )
   }
   dbcl.insert( docs );

   db.analyze( { Collection: fullclName } );

   //不计算IO代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds1, dbcl, indexName, scanType );
   testExplain( conds2, dbcl, indexName_a, scanType1 );

   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );
   dbcl.insert( docs );
   dbcl.insert( docs );

   db.analyze( { Collection: fullclName } );

   //计算IO代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds1, dbcl, indexName_a, scanType1 );
   testExplain( conds2, dbcl, indexName_a, scanType1 );
}

