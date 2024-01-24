/************************************
*@Description: seqDB-11358:rtnPredicate为[valA, valA]，数据类型为非bool型，数据均匀情况下的索引选择 
*@author:      chimanzhao
*@createdate:  2020.5.12
*@testlinkCase: seqDB-11358
**************************************/
testConf.clName = COMMCLNAME + "_11358";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );

   var conds = [{ a: { $et: 1 } }, { a: { $in: [1] } }, { a: { $all: [1] } }, { a: { $exists: 0 } }, { a: { $isnull: 1 } }];
   var conds1 = [conds[0], conds[1], conds[2]];
   var conds2 = [conds[3], conds[4]];
   var indexName = "a";
   var scanType = "ixscan";
   var indexName1 = "";
   var scanType1 = "tbscan";
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
   //添加数据使数据页数大于optestcachesize（20）
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

   testExplain( conds1, dbcl, indexName, scanType );
   testExplain( conds2, dbcl, indexName1, scanType1 );
}

