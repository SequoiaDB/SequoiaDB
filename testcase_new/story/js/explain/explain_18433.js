/******************************************************************************
*@Description : seqDB-18433:查询时多个索引符合候选计划要求的索引选择
*@author      : Li Yuanyue
*@Date        : 2020.5.12
******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_18433";
testConf.clOpt = {Compressed: false};

main( test );

function test ( args )
{
   var idxName1 = "index_a_18433";
   var idxName2 = "index_ab_18433";
   var idxName3 = "index_abc_18433";

   var cl = args.testCL;

   cl.createIndex( idxName1, { a: 1 } );
   cl.createIndex( idxName2, { a: 1, b: 1 } );
   cl.createIndex( idxName3, { a: 1, b: 1, c: 1 } );

   var docs = [];
   for( var i = 0; i < 11000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   // 不计算io代价
   var expNeedEvalIO = false;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1, c: 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var docs = [];
   for( var i = 0; i < 20000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   db.sync();

   // 计算io代价
   var expNeedEvalIO = true;
   checkNeedEvalIO( cl, expNeedEvalIO );

   var cond = { "a": 1 };
   var expIndexName = idxName1;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1 };
   var expIndexName = idxName2;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );

   var cond = { a: 1, b: 1, c: 1 };
   var expIndexName = idxName3;
   var expScanType = "ixscan";
   checkExplain( cl, cond, expIndexName, expScanType );
}