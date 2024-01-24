/******************************************************************************
*@Description : seqDB-11407: 无统计信息，匹配符不走索引
*@author      : Li Yuanyue
*@Date        : 2020.5.12
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_11407";
testConf.clOpt = {Compressed: false};

main( test );

function test ( args )
{
   var clName = CHANGEDPREFIX + "_11407";
   var idxName1 = "index_abc_11407";
   var idxName2 = "index_ab_11407";
   var idxName3 = "index_a_11407";
   var tbIdx = "";

   var cl = args.testCL;

   cl.createIndex( idxName1, { a: 1, b: -1, c: 1 } );
   cl.createIndex( idxName2, { a: -1, b: 1 } );
   cl.createIndex( idxName3, { a: 1 } );

   // 生成随机数
   var rd = new commDataGenerator();
   var value = rd.getRecords( 11000, "int", ["a", "b", "c"] );
   cl.insert( value );

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = [{ "a": { "$mod": [100, 2] } }, { "b": { "$ne": 5 } }
      , { "c": { "$gt": 2000 } }, { "a": { "$gte": 1000 } }
      , { "b": { "$lt": 500 } }, { "c": { "$lte": 666 } }, { "a": { "$exists": 1 } }];
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   testExplain( cl, cond, expIndexName, expScanType );

   var value = rd.getRecords( 20000, "int", ["a", "b", "c"] );
   cl.insert( value );

   db.sync()

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = [{ "a": { "$mod": [100, 2] } }, { "b": { "$ne": 5 } }
      , { "c": { "$gt": 2000 } }, { "a": { "$gte": 1000 } }
      , { "b": { "$lt": 500 } }, { "c": { "$lte": 666 } }, { "a": { "$exists": 1 } }];
   var expIndexName = tbIdx;
   var expScanType = "tbscan";
   testExplain( cl, cond, expIndexName, expScanType );
}