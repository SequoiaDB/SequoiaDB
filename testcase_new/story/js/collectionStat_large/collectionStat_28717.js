/******************************************************************************
 * @Description   : seqDB-28717:执行analyze，插入大量数据后，不执行/执行analyze,数据节点调用getCollectionStat()
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.20
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28717";
testConf.csName = COMMCSNAME + "_28717";

main( test );
function test ( args )
{
   var cl = args.testCL;
   var csName = testConf.csName;
   var clName = testConf.clName;
   var group = testPara.srcGroupName;
   var recsNum = 500000;

   var dataNode = db.getRG( group ).getMaster();
   var data = dataNode.connect();
   var dataCL = data.getCS( csName ).getCL( clName );
   var nodeName = dataNode.getHostName() + ":" + dataNode.getServiceName();

   // 执行analyze
   data.analyze( { "Collection": csName + "." + clName } );

   // 插入大量数据，使缓存过期
   insertData( cl, recsNum );
   var isDefault = false;
   var isExpired = true;
   var avgNumFields = 10;
   var sampleRecords = 0;
   var totalRecords = 0;
   var totalDataPages = 0;
   var totalDataSize = 0;
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   // analyze，更新缓存中的集合统计信息
   data.analyze( { "Collection": csName + "." + clName } );

   isExpired = false;
   sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   totalDataPages = tmpcur.TotalDataPages;
   cursor.close();

   // analyze后，该字段值是从磁盘获取，快照和接口不能得出准确值，所以在用例中写固定值
   totalDataSize = 14500000;
   checkCollectionStat( dataCL, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, totalDataSize );

   data.close();
}