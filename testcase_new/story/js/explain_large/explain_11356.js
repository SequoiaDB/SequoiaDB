/************************************
*@Description: seqDB-11356: rtnPredicate为[valA, valA]，数据类型为不在mcv中统计的类型，数据均匀情况下的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.8
*@testlinkCase: seqDB-11356
**************************************/
testConf.clName = COMMCLNAME + "_11356";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );

   //设置查询条件
   var conds = [{ a: { age: 1 } }, { a: { $in: [{ age: 1 }] } }, { a: { $all: [{ age: 1 }] } }, { a: { age: { $exists: 0 } } }, { a: { age: { $isnull: 1 } } }];
   indexName = "a";
   scanType = "ixscan";

   //不计算IO代价,构造数据类型不为在mcv中统计的类型，且数据均匀
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: { age: i }, b: { age: -i } } )
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

