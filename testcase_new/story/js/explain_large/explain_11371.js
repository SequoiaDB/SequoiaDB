/************************************
*@Description: seqDB-11371:rtnPredicate为[valA, valB]，valA与valB类型不同情况下的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.12
*@testlinkCase: seqDB-11371
**************************************/
testConf.clName = COMMCLNAME + "_11371";
testConf.clOpt = {Compressed: false};

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );
   var fullclName = COMMCSNAME + "." + testConf.clName;

   //设置查询条件,构造valA及valB在mcv中存在统计信息且跨类型的场景 
   var conds = [{ $and: [{ a: { $gt: 250 } }, { a: { $lt: true } }] }, { $and: [{ a: { $gte: 250 } }, { a: { $lte: false } }] }];
   indexName = "a";
   scanType = "ixscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      if( i > 500 )
      {
         if( i % 2 )
         {
            docs.push( { a: true, b: i } )
         }
         else
         {
            docs.push( { a: false, b: i } )
         }
      }
      else
      {
         docs.push( { a: i, b: i } )
      }
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

   testExplain( conds, dbcl, indexName, scanType );
}
