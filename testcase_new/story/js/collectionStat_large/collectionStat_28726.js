/******************************************************************************
 * @Description   : seqDB-28726:集合没有插入数据，不执行/执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28726";
testConf.csName = COMMCSNAME + "_28726";
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var csName = testConf.csName;
   var clName = testConf.clName;
   var groupName = testPara.srcGroupName;
   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();

   // 未做analyze,显示集合统计信息的默认值
   var isDefault = true;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 200;
   var totalRecords = 200;
   var totalDataPages = 1;
   var totalDataSize = 80000;
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": csName + "." + clName } );

   isDefault = false;
   sampleRecords = 0;
   totalRecords = 0;
   totalDataSize = 0;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   totalDataPages = tmpcur.TotalDataPages;
   cursor.close();

   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );
}