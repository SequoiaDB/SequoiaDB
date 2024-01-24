/************************************
*@Description: seqDB-11362:rtnPredicate为[valA, valB]，非唯一索引下的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.12
*@testlinkCase: seqDB-11362
**************************************/
testConf.clName = COMMCLNAME + "_11362";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 }, true );
   dbcl.createIndex( "b", { b: -1 }, true );
   dbcl.createIndex( "ab", { a: 1, b: 1 }, true );

   //设置查询条件,构造场景(不计算IO代价时,a以5为周期选入mcv中;计算IO代价时，a存入mcv的值为250，506，761)
   //仅valA在mcv中存在统计信息、仅valB在mcv中存在统计信息、valA及valB在mcv中存在统计信息的场景
   var conds = [{ a: { $gt: 250 } }, { a: { $gte: 250 } }, { a: { $lt: 250 } }, { a: { $lte: 250 } }];
   var conds1 = { "$and": [{ a: { $gt: 0 } }, { a: { $lt: 5 } }] }
   var conds2 = { "$and": [{ a: { $gt: 250 } }, { a: { $lt: 506 } }] }
   var indexName = "";
   var scanType = "tbscan";
   var indexName1 = "a";
   var scanType1 = "ixscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: -i } )
   }
   dbcl.insert( docs );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );
   testExplain( conds1, dbcl, indexName1, scanType1 );
   //计算IO代价

   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName1, scanType1 );
   testExplain( conds2, dbcl, indexName1, scanType1 );
}

