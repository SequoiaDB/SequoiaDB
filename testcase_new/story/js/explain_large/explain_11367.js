/************************************
*@Description: seqDB-11367: rtnPredicate为[valA, valB]，数据均匀情况下的索引选择 
*@author:      chimanzhao
*@createdate:  2020.5.12
*@testlinkCase: seqDB-11367
**************************************/
testConf.clName = COMMCLNAME + "_11367";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );
   dbcl.createIndex( "c", { c: 1 } );
   dbcl.createIndex( "d", { d: -1 } );
   dbcl.createIndex( "cd", { c: 1, d: 1 } );

   //设置查询条件,构造场景(不计算IO代价时,a以5为周期选入mcv中;计算IO代价时，a存入mcv的值为250，506，761)
   //分别构造查询字段在mcv中统计的数据类型和不在mcv中统计的数据类型。
   var conds = [{ a: { $gt: 250 } }, { a: { $gte: 250 } }, { a: { $lt: 250 } }, { a: { $lte: 250 } }];
   var conds1 = [{ c: { $gt: { age: 1 } } }, { c: { $gte: { age: 1 } } }, { c: { $lt: { age: 1 } } }, { c: { $lte: { age: 1 } } }];

   indexName = ""
   indexName_a = "a";
   indexName_c = "c";
   scanType = "tbscan";
   scanType1 = "ixscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: { age: i }, d: { age: -i } } )
   }
   dbcl.insert( docs );

   testExplain( conds, dbcl, indexName, scanType );
   testExplain( conds1, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );
   testExplain( conds1, dbcl, indexName, scanType );

   //计算IO代价

   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );

   testExplain( conds, dbcl, indexName, scanType );
   testExplain( conds1, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName_a, scanType1 );
   testExplain( conds1, dbcl, indexName_c, scanType1 );
}

