/******************************************************************************
 * @Description   : seqDB-28715:执行analyze，插入大量数据后，不执行/执行analyze,协调节点调用getCollectionStat()
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.20
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28715";
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   var recsNum = 500000;

   // 执行analyze
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );

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
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );
   isExpired = false;
   // 默认值为200
   sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + testConf.clName, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   totalDataPages = tmpcur.TotalDataPages;
   cursor.close();

   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, undefined );
}