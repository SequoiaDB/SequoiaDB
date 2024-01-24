/******************************************************************************
 * @Description   : seqDB-28716:普通表插入数据后，不执行/执行analyze，数据节点调用getCollectionStat()
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.20
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28716";
testConf.csName = COMMCSNAME + "_28716";

main( test );
function test ( args )
{
   var cl = args.testCL;
   var csName = testConf.csName;
   var clName = testConf.clName;
   var group = testPara.srcGroupName;
   var recsNum = 10000;

   var dataNode = db.getRG( group ).getMaster();
   var data = dataNode.connect();
   var dataCL = data.getCS( csName ).getCL( clName );
   var nodeName = dataNode.getHostName() + ":" + dataNode.getServiceName();

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
   data.analyze( { "Collection": csName + "." + clName } );
   isDefault = false;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   totalDataPages = tmpcur.TotalDataPages;
   cursor.close();
   checkCollectionStat( dataCL, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, undefined );

   data.close();
}