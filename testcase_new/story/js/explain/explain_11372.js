/******************************************************************************
*@Description : seqDB-11372:匹配组合索引的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.4.25
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11372";
testConf.clOpt = {Compressed: false};

main( test );

function test ( args )
{
   var idxName1 = "index_abc_11372";
   var idxName2 = "index_ab_11372";
   var idxName3 = "index_a_11372";
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var cl = args.testCL;

   cl.createIndex( idxName1, { a: 1, b: -1, c: 1 } );
   cl.createIndex( idxName2, { a: -1, b: 1 } );
   cl.createIndex( idxName3, { a: 1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var value = rd.getRecords( 20000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1, "b": 1, "c": -1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1, "b": 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { "a": 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );
}
