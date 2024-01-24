/******************************************************************************
 * @Description   : seqDB-27853:truncateLob，truncate部分占多个大对象页
 * @Author        : liuli
 * @CreateTime    : 2022.09.30
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27853";
testConf.csOpt = { LobPageSize: 32768 };
testConf.clName = COMMCLNAME + "_27853";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var lobPageSize = 32768;
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27853/";
   var fileName = "filelob_27853";
   var fileSize = 1024 * 1024;
   var size = 1024 * 50;

   // coord节点会统计部分负载监控指标，重新连接一个coord确定使用coord NodeName
   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );
   var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

   // 获取cl所在的group的主节点,部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var masterNode = sdb.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();
   var nodeNum = commGetGroupNodes( sdb, groupName ).length;

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

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

   // 执行truncateLob操作，truncateLob部分多个大对象页
   dbcl.truncateLob( lobID, size );

   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   // Number of load monitoring nodes
   var loadMonitorNodeNum = 2;
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   dataBastInfo[0]["TotalLobDelete"] += 1 * loadMonitorNodeNum;
   dataBastInfo[0]["TotalLobWrite"] += 1;
   // 元数据寻址此时增加数量等于大对象页增加数量
   dataBastInfo[0]["TotalLobAddressing"] += lobPages;
   dataBastInfo[0]["TotalLobTruncate"] += lobPages - 2;
   dataBastInfo[0]["TotalLobRead"] += lobPages;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看数据库快照并校验结果，非聚合结果
   for( var i in dataBastInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( dataBastInfoRawData[i]["NodeName"] == masterNodeName )
      {
         dataBastInfoRawData[i]["TotalLobDelete"] += 1;
         dataBastInfoRawData[i]["TotalLobWrite"] += 1;
         // 元数据寻址此时增加数量等于大对象页增加数量
         dataBastInfoRawData[i]["TotalLobAddressing"] += lobPages;
         dataBastInfoRawData[i]["TotalLobTruncate"] += lobPages - 2;
         dataBastInfoRawData[i]["TotalLobRead"] += lobPages;
      }
      else if( dataBastInfoRawData[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfoRawData[i]["TotalLobDelete"] += 1;
      }
   }
   // 查看快照进行排序，此前的预期结果已进行过排序
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobDelete"] += 1;
   csInfo[0]["TotalLobWrite"] += 1;
   // 元数据寻址此时增加数量等于大对象页增加数量
   csInfo[0]["TotalLobAddressing"] += lobPages - 1;
   csInfo[0]["TotalLobTruncate"] += lobPages - 2;
   csInfo[0]["TotalLobRead"] += lobPages;

   csInfo[0]["TotalLobPages"] -= ( lobPages - 2 ) * nodeNum;
   csInfo[0]["TotalUsedLobSpace"] = lobPageSize * 2 * nodeNum;
   csInfo[0]["TotalLobSize"] = ( lobPageSize * 2 - 1024 ) * nodeNum;
   csInfo[0]["TotalValidLobSize"] -= ( fileSize - size ) * nodeNum;
   csInfo[0]["LobUsageRate"] = csInfo[0]["TotalValidLobSize"] / csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["FreeLobSpace"];
   csInfo[0]["AvgLobSize"] = csInfo[0]["TotalValidLobSize"] / 1 / nodeNum;
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      if( csInfoRawData[i]["NodeName"] == masterNodeName )
      {
         csInfoRawData[i]["TotalLobDelete"] += 1;
         csInfoRawData[i]["TotalLobWrite"] += 1;
         // 元数据寻址此时增加数量等于大对象页增加数量
         csInfoRawData[i]["TotalLobAddressing"] += lobPages - 1;
         csInfoRawData[i]["TotalLobTruncate"] += lobPages - 2;
         csInfoRawData[i]["TotalLobRead"] += lobPages;
      }
      csInfoRawData[i]["TotalLobPages"] -= ( lobPages - 2 );
      csInfoRawData[i]["TotalUsedLobSpace"] = lobPageSize * 2;
      csInfoRawData[i]["TotalLobSize"] = ( lobPageSize * 2 - 1024 );
      csInfoRawData[i]["TotalValidLobSize"] -= ( fileSize - size );
      csInfoRawData[i]["LobUsageRate"] = csInfoRawData[i]["TotalLobSize"] / csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSpace"] = csInfoRawData[i]["LobCapacity"] - csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSize"] = csInfoRawData[i]["FreeLobSpace"];
      csInfoRawData[i]["AvgLobSize"] = csInfoRawData[i]["TotalValidLobSize"] / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      if( clInfo[i]["NodeName"] == masterNodeName )
      {
         clInfo[i]["TotalLobDelete"] += 1;
         clInfo[i]["TotalLobWrite"] += 1;
         // 元数据寻址此时增加数量等于大对象页增加数量
         clInfo[i]["TotalLobAddressing"] += lobPages - 1;
         clInfo[i]["TotalLobTruncate"] += lobPages - 2;
         clInfo[i]["TotalLobRead"] += lobPages;
      }
      clInfo[i]["TotalLobPages"] -= ( lobPages - 2 );
      clInfo[i]["TotalUsedLobSpace"] = lobPageSize * 2;
      clInfo[i]["TotalLobSize"] = ( lobPageSize * 2 - 1024 );
      clInfo[i]["TotalValidLobSize"] -= ( fileSize - size );
      clInfo[i]["LobUsageRate"] = clInfo[i]["TotalLobSize"] / clInfo[i]["TotalUsedLobSpace"];
      clInfo[i]["AvgLobSize"] = clInfo[i]["TotalValidLobSize"] / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      if( clInfoRawData[i]["NodeName"] == masterNodeName )
      {
         clInfoRawData[i]["TotalLobDelete"] += 1;
         clInfoRawData[i]["TotalLobWrite"] += 1;
         // 元数据寻址此时增加数量等于大对象页增加数量
         clInfoRawData[i]["TotalLobAddressing"] += lobPages - 1;
         clInfoRawData[i]["TotalLobTruncate"] += lobPages - 2;
         clInfoRawData[i]["TotalLobRead"] += lobPages;
      }
      clInfoRawData[i]["TotalLobPages"] -= ( lobPages - 2 );
      clInfoRawData[i]["TotalUsedLobSpace"] = lobPageSize * 2;
      clInfoRawData[i]["TotalLobSize"] = ( lobPageSize * 2 - 1024 );
      clInfoRawData[i]["TotalValidLobSize"] -= ( fileSize - size );
      clInfoRawData[i]["LobUsageRate"] = clInfoRawData[i]["TotalLobSize"] / clInfoRawData[i]["TotalUsedLobSpace"];
      clInfoRawData[i]["AvgLobSize"] = clInfoRawData[i]["TotalValidLobSize"] / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobDelete"] += 1;
   clDetail[0]["TotalLobWrite"] += 1;
   // 元数据寻址此时增加数量等于大对象页增加数量
   clDetail[0]["TotalLobAddressing"] += lobPages - 1;
   clDetail[0]["TotalLobTruncate"] += lobPages - 2;
   clDetail[0]["TotalLobRead"] += lobPages;
   clDetail[0]["TotalLobPages"] -= ( lobPages - 2 );
   clDetail[0]["TotalUsedLobSpace"] = lobPageSize * 2;
   clDetail[0]["TotalLobSize"] = ( lobPageSize * 2 - 1024 );
   clDetail[0]["TotalValidLobSize"] -= ( fileSize - size );
   clDetail[0]["LobUsageRate"] = clDetail[0]["TotalLobSize"] / clDetail[0]["TotalUsedLobSpace"];
   clDetail[0]["AvgLobSize"] = clDetail[0]["TotalValidLobSize"] / 1;

   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   deleteTmpFile( filePath );

   sdb.close();
}