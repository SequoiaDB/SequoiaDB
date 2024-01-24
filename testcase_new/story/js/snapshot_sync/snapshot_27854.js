/******************************************************************************
 * @Description   : seqDB-27854:执行listLobs后查看快照 
 * @Author        : liuli
 * @CreateTime    : 2022.09.30
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27854";
testConf.clName = COMMCLNAME + "_27854";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var filePath = WORKDIR + "/lob27849/";
   var fileName = "filelob_27849";
   var fileSize = 1024 * 200;
   var lobPageSize = 262144;

   // coord节点会统计部分负载监控指标，重新连接一个coord确定使用coord NodeName
   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );
   var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

   // 获取cl所在的group的主节点,部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var masterNode = sdb.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;

   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobs = 100;
   for( var j = 0; j < lobs; j++ )
   {
      dbcl.putLob( filePath + fileName );
   }

   // 获取数据库快照信息，聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取数据库快照信息，非聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   var dataBastInfoRawData = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData = getSnapshotLobStat( cursor );

   // 获取集合快照信息，聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, { Name: testConf.csName + "." + testConf.clName } );
   var clInfo = getSnapshotLobStatToCL( cursor, false );

   // 获取集合快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData = getSnapshotLobStatToCL( cursor, true );

   // 获取集合getDetail信息
   var cursor = dbcl.getDetail();
   var clDetail = getSnapshotLobStatToCL( cursor, true );

   dbcl.listLobs().toArray();

   // 查看数据库快照并校验结果，聚合结果
   // Number of load monitoring nodes
   var loadMonitorNodeNum = 2;
   dataBastInfo[0]["TotalLobList"] += 1 * loadMonitorNodeNum;
   dataBastInfo[0]["TotalLobRead"] += lobs;
   dataBastInfo[0]["TotalLobAddressing"] += lobs;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看数据库快照并校验结果，非聚合结果
   for( var i in dataBastInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( dataBastInfoRawData[i]["NodeName"] == masterNodeName )
      {
         dataBastInfoRawData[i]["TotalLobList"] += 1;
         dataBastInfoRawData[i]["TotalLobRead"] += lobs;
         dataBastInfoRawData[i]["TotalLobAddressing"] += lobs;
      }
      else if( dataBastInfoRawData[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfoRawData[i]["TotalLobList"] += 1;
      }
   }
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobList"] += 1;
   csInfo[0]["TotalLobRead"] += lobs;
   csInfo[0]["TotalLobAddressing"] += lobs;
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( csInfoRawData[i]["NodeName"] == masterNodeName )
      {
         csInfoRawData[i]["TotalLobList"] += 1;
         csInfoRawData[i]["TotalLobRead"] += lobs;
         csInfoRawData[i]["TotalLobAddressing"] += lobs;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( clInfo[i]["NodeName"] == masterNodeName )
      {
         clInfo[i]["TotalLobList"] += 1;
         clInfo[i]["TotalLobRead"] += lobs;
         clInfo[i]["TotalLobAddressing"] += lobs;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( clInfoRawData[i]["NodeName"] == masterNodeName )
      {
         clInfoRawData[i]["TotalLobList"] += 1;
         clInfoRawData[i]["TotalLobRead"] += lobs;
         clInfoRawData[i]["TotalLobAddressing"] += lobs;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobList"] += 1;
   clDetail[0]["TotalLobRead"] += lobs;
   clDetail[0]["TotalLobAddressing"] += lobs;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   sdb.close();
   deleteTmpFile( filePath + fileName )
}