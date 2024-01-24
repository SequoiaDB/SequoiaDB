/******************************************************************************
 * @Description   : seqDB-28725:执行analyze后，分区表插入大量数据，使部分复制组过期执行analyze/使全部复制组过期执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.22
 * @LastEditTime  : 2023.01.19
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28725";
testConf.csName = COMMCSNAME + "_28725";
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
   var node = db.getRG( srcGroupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   var node1 = db.getRG( dstGroupName1 ).getMaster();
   var nodeName1 = node1.getHostName() + ":" + node1.getServiceName();
   var node2 = db.getRG( dstGroupName2 ).getMaster();
   var nodeName2 = node2.getHostName() + ":" + node2.getServiceName();

   cl.split( srcGroupName, dstGroupName1, { "a": 200000 }, { "a": { "$minKey": 1 } } );
   cl.split( srcGroupName, dstGroupName2, { "a": 300000 }, { "a": 200000 } );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": csName + "." + clName } );

   // 插入大量起数据在一个group上
   var recsNum1 = 200000;
   insertData( cl, recsNum1 );
   db.analyze( { "Collection": csName + "." + clName } );

   var isDefault = false;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName1 } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages1 = tmpcur.TotalDataPages;
   cursor.close();
   // 返回过期值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, recsNum1, totalDataPages1, undefined );

   // 插入大量数据在每个group上
   var recsNum2 = 600000;
   var docs = [];
   for( var i = 200000; i < recsNum2; i++ )
   {
      docs.push( { a: i } );
   }
   cl.insert( docs );
   db.analyze( { "Collection": csName + "." + clName } );

   sampleRecords = 600;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName2 } );
   tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages2 = tmpcur.TotalDataPages;
   cursor.close();

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName } );
   tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages3 = tmpcur.TotalDataPages;
   var totalDataPages = totalDataPages1 + totalDataPages2 + totalDataPages3
   cursor.close();
   // 返回过期值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, recsNum2, totalDataPages, undefined );
}