/************************************
*@Description: seqDB-11355:rtnPredicate为[valA, valA]，ValA落在mcv中的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.7
*@testlinkCase: seqDB-11355
**************************************/
testConf.clName = COMMCLNAME + "_11355";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );

   //设置查询条件,构造valA在mcv中存在统计信息的场景
   //(不计算IO代价时,a以5为周期选入mcv中;计算IO代价时，a存入mcv的值为250，506，761,ab存入mcv的值为260，516，771)
   var conds = [{ a: { $et: 250 } }, { a: { $in: [250] } }, { a: { $all: [250] } }, { a: { $exists: 0 } }, { a: { $isnull: 1 } }];
   var conds1 = [conds[3], conds[4]];
   var conds2 = [conds[0], conds[1], conds[2]];
   var conds3 = [{ a: { $et: 260 } }, { a: { $in: [260] } }, { a: { $all: [260] } }];
   indexName = "a";
   indexName1 = "";
   indexName2 = "ab";
   scanType = "ixscan";
   scanType1 = "tbscan";
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

   testExplain( conds1, dbcl, indexName1, scanType1 );
   testExplain( conds2, dbcl, indexName, scanType );
   testExplain( conds3, dbcl, indexName, scanType );
}

