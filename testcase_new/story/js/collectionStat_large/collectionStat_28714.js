/******************************************************************************
 * @Description   : seqDB-28714:普通表插入数据后，不执行/执行analyze，协调节点调用getCollectionStat()
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.20
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28714";
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var recsNum = 10000;
   var groupName = testPara.srcGroupName;
   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();

   insertData( cl, recsNum );
   var isDefault = true;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 200;
   var totalRecords = 200;
   var totalDataPages = 1;
   var totalDataSize = 80000;

   // 未做analyze,显示集合统计信息的默认值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );
   isDefault = false;

   // analyze后，该字段值是从磁盘获取，快照和接口不能得出准确值，所以在用例中写固定值
   totalDataSize = 290000;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + testConf.clName, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   totalDataPages = tmpcur.TotalDataPages;
   cursor.close();

   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, totalDataSize );
}