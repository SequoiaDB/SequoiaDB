/******************************************************************************
 * @Description   : seqDB-28723:执行analyze后，分区表插入大量数据，使部分复制组过期，不执行analyze
 *                  seqDB-28724:执行analyze后，分区表插入大量数据，使全部复制组过期，不执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.17
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28723_28724";
testConf.csName = COMMCSNAME + "_28723_28724";
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipGroupLessThanThree = true;
testConf.clOpt = { "ShardingKey": { "a": -1 }, "ShardingType": "range" }

main( test );
function test ( args )
{
   var cl = args.testCL;
   var csName = testConf.csName;
   var clName = testConf.clName;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName1 = testPara.dstGroupNames[0];
   var dstGroupName2 = testPara.dstGroupNames[1];

   cl.split( srcGroupName, dstGroupName1, { "a": 200000 }, { "a": { "$minKey": 1 } } );
   cl.split( srcGroupName, dstGroupName2, { "a": 400000 }, { "a": 200000 } );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": csName + "." + clName } );

   // 插入大量数据在一个group上
   var recsNum1 = 200000;
   insertData( cl, recsNum1 );
   var isDefault = false;
   var isExpired = true;
   var avgNumFields = 10;
   var sampleRecords = 0;
   var totalRecords = 0;
   var totalDataPages = 0;
   var totalDataSize = 0;

   // 返回过期值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   // 插入大量数据在每个group上
   var recsNum2 = 600000;
   var docs = [];
   for( var i = 200000; i < recsNum2; i++ )
   {
      docs.push( { a: i } );
   }
   cl.insert( docs );

   // 返回过期值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );
}