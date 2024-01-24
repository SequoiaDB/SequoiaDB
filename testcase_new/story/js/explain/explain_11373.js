/******************************************************************************
*@Description : seqDB-11373:查询带排序，前一阶段查询结果有序的索引选择 
*@author      : Li Yuanyue
*@Date        : 2020.4.27
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11373";
testConf.clOpt = {Compressed: false};

main( test );

function test ( args )
{
   var idxNamea = "index_a_11373";
   var idxNameb = "index_b_11373";
   var idxNamec = "index_c_11373";
   var tbIdx = "";
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var cl = args.testCL;

   cl.createIndex( idxNamea, { a: 1 } );
   cl.createIndex( idxNameb, { b: -1 } );
   cl.createIndex( idxNamec, { c: 1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": { "$gt": 1 } };
   var expIndexName = idxNameb;
   var expScanType = "ixscan";
   var sortCond = { "b": 1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "b": { "$gt": 1 } };
   var expIndexName = idxNamec;
   var expScanType = "ixscan";
   var sortCond = { "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "c": { "$gt": 1 } };
   var expIndexName = idxNamea;
   var expScanType = "ixscan";
   var sortCond = { "a": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var value = rd.getRecords( 20000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": { "$gt": 1 } };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = { "b": 1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "b": { "$gt": 1 } };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = { "c": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var cond = { "c": { "$gt": 1 } };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = { "a": -1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

}
