/******************************************************************************
*@Description : seqDB-11374:查询并排序，且排序为内排的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.5.6
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11374";
testConf.clOpt = {Compressed: false};

main( test );

function test ( args )
{
   var idxName1 = "index_abc_11374";
   var tbIdx = "";
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var cl = args.testCL;

   cl.createIndex( idxName1, { a: 1, b: -1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 不计算 IO 代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": { "$gt": 1 } };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = { "a": 1, "b": -1, "c": 1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );

   var value = rd.getRecords( 20000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.analyze( { Collection: fullclName } );

   // 计算 IO 代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": { "$gt": 1 } };
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   var sortCond = { "a": 1, "b": -1, "c": 1 };
   checkExplain( cl, cond, expIndexName, expScanType, sortCond );
}
