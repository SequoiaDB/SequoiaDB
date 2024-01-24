/************************************
*@Description: seqDB-11354:rtnPredicate为[valA, valA]，唯一索引下的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.6
*@testlinkCase: seqDB-11354
**************************************/
testConf.clName = COMMCLNAME + "_11354";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 }, true );
   dbcl.createIndex( "b", { b: -1 }, true );
   dbcl.createIndex( "ab", { a: 1, b: 1 }, true );

   //设置查询条件,构造valA在mcv中存在统计信息的场景(不计算IO代价时,a以5为周期选入mcv中;计算IO代价时，a存入mcv的值为250，506，761)
   var conds = [{ a: { $et: 250 } }, { a: { $in: [250] } }, { a: { $all: [506] } }, { a: { $exists: 0 } }, { a: { $isnull: 1 } }];
   indexName = "a";
   scanType = "ixscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: -i } )
   }
   dbcl.insert( docs );
   testExplain( conds, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );

   //计算IO代价
   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );
   testExplain( conds, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );
}

