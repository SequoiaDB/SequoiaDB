/******************************************************************************
 * @Description   : seqDB-27849:putLob/getLob后查看快照，lob占多个大对象页
 *                : seqDB-27851:deleteLob，lob占一个/多个大对象页(占多个大对象页部分)
 * @Author        : liuli
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27849";
testConf.csOpt = { LobPageSize: 8192 };
testConf.clName = COMMCLNAME + "_27849";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27849/";
   var fileName = "filelob_27849";
   var fileSize = 1024 * 1024;
   var lobPageSize = 8192;

   // coord节点会统计部分负载监控指标，重新连接一个coord确定使用coord NodeName
   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );
   var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

   // 获取cl所在的group的主节点,部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var masterNode = sdb.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();
   var nodeNum = commGetGroupNodes( sdb, groupName ).length;

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

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   // Number of load monitoring nodes
   var loadMonitorNodeNum = 2;
   dataBastInfo[0]["TotalLobPut"] += 1 * loadMonitorNodeNum;
   dataBastInfo[0]["TotalLobWriteSize"] += fileSize * loadMonitorNodeNum;
   // 元数据寻址此时增加数量等于大对象页增加数量
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   dataBastInfo[0]["TotalLobAddressing"] += lobPages;
   dataBastInfo[0]["TotalLobWrite"] += lobPages;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看数据库快照并校验结果，非聚合结果
   for( var i in dataBastInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( dataBastInfoRawData[i]["NodeName"] == masterNodeName )
      {
         dataBastInfoRawData[i]["TotalLobPut"] += 1;
         dataBastInfoRawData[i]["TotalLobWriteSize"] += fileSize;
         dataBastInfoRawData[i]["TotalLobWrite"] += lobPages;
         // 元数据寻址此时增加数量等于大对象页增加数量
         dataBastInfoRawData[i]["TotalLobAddressing"] += lobPages;
      }
      else if( dataBastInfoRawData[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfoRawData[i]["TotalLobPut"] += 1;
         dataBastInfoRawData[i]["TotalLobWriteSize"] += fileSize;
      }
   }
   // 查看快照进行排序，此前的预期结果已进行过排序
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobPut"] += 1;
   csInfo[0]["TotalLobWriteSize"] += fileSize;
   csInfo[0]["TotalLobWrite"] += lobPages;
   csInfo[0]["TotalLobAddressing"] += lobPages;
   csInfo[0]["LobCapacity"] += lobdSize * nodeNum;
   csInfo[0]["LobMetaCapacity"] += lobmSize * nodeNum;
   csInfo[0]["TotalUsedLobSpace"] += lobPageSize * lobPages * nodeNum;
   csInfo[0]["TotalLobs"] += 1 * nodeNum;
   csInfo[0]["TotalLobPages"] += lobPages * nodeNum;
   csInfo[0]["TotalLobSize"] += fileSize * nodeNum;
   csInfo[0]["TotalValidLobSize"] += fileSize * nodeNum;

   csInfo[0]["LobUsageRate"] = csInfo[0]["TotalValidLobSize"] / csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["FreeLobSpace"];
   csInfo[0]["AvgLobSize"] = fileSize / 1;
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      if( csInfoRawData[i]["NodeName"] == masterNodeName )
      {
         csInfoRawData[i]["TotalLobPut"] += 1;
         csInfoRawData[i]["TotalLobWriteSize"] += fileSize;
         csInfoRawData[i]["TotalLobAddressing"] += lobPages;
         csInfoRawData[i]["TotalLobWrite"] += lobPages;
      }
      csInfoRawData[i]["LobCapacity"] += lobdSize;
      csInfoRawData[i]["LobMetaCapacity"] += lobmSize;
      csInfoRawData[i]["TotalUsedLobSpace"] += lobPageSize * lobPages;
      csInfoRawData[i]["TotalLobs"] += 1;
      csInfoRawData[i]["TotalLobPages"] += lobPages;
      csInfoRawData[i]["TotalLobSize"] += fileSize;
      csInfoRawData[i]["TotalValidLobSize"] += fileSize;

      csInfoRawData[i]["LobUsageRate"] = csInfoRawData[i]["TotalLobSize"] / csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSpace"] = csInfoRawData[i]["LobCapacity"] - csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSize"] = csInfoRawData[i]["FreeLobSpace"];
      csInfoRawData[i]["AvgLobSize"] = fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      if( clInfo[i]["NodeName"] == masterNodeName )
      {
         clInfo[i]["TotalLobPut"] += 1;
         clInfo[i]["TotalLobWriteSize"] += fileSize;
         clInfo[i]["TotalLobAddressing"] += lobPages;
         clInfo[i]["TotalLobWrite"] += lobPages;
      }
      clInfo[i]["TotalLobSize"] += fileSize;
      clInfo[i]["TotalValidLobSize"] += fileSize;
      clInfo[i]["TotalLobs"] += 1;
      clInfo[i]["TotalLobPages"] += lobPages;
      clInfo[i]["TotalUsedLobSpace"] += lobPageSize * lobPages;
      clInfo[i]["LobUsageRate"] += clInfo[i]["TotalLobSize"] / clInfo[i]["TotalUsedLobSpace"];
      clInfo[i]["AvgLobSize"] += fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      if( clInfoRawData[i]["NodeName"] == masterNodeName )
      {
         clInfoRawData[i]["TotalLobPut"] += 1;
         clInfoRawData[i]["TotalLobWriteSize"] += fileSize;
         clInfoRawData[i]["TotalLobAddressing"] += lobPages;
         clInfoRawData[i]["TotalLobWrite"] += lobPages;
      }
      clInfoRawData[i]["TotalLobSize"] += fileSize;
      clInfoRawData[i]["TotalValidLobSize"] += fileSize;
      clInfoRawData[i]["TotalLobs"] += 1;
      clInfoRawData[i]["TotalLobPages"] += lobPages;
      clInfoRawData[i]["TotalUsedLobSpace"] += lobPageSize * lobPages;
      clInfoRawData[i]["LobUsageRate"] += clInfo[i]["TotalLobSize"] / clInfo[i]["TotalUsedLobSpace"];
      clInfoRawData[i]["AvgLobSize"] += fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobPut"] += 1;
   clDetail[0]["TotalLobWriteSize"] += fileSize;
   clDetail[0]["TotalLobAddressing"] += lobPages;
   clDetail[0]["TotalLobWrite"] += lobPages;
   clDetail[0]["TotalLobSize"] += fileSize;
   clDetail[0]["TotalValidLobSize"] += fileSize;
   clDetail[0]["TotalLobs"] += 1;
   clDetail[0]["TotalLobPages"] += lobPages;
   clDetail[0]["TotalUsedLobSpace"] += lobPageSize * lobPages;
   clDetail[0]["LobUsageRate"] += clInfo[i]["TotalLobSize"] / clInfo[i]["TotalUsedLobSpace"];
   clDetail[0]["AvgLobSize"] += fileSize / 1;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   // 执行getLob操作
   dbcl.getLob( lobID, filePath + "checkputlob27849", true );

   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   dataBastInfo[0]["TotalLobGet"] += 1 * 2;
   dataBastInfo[0]["TotalLobReadSize"] += fileSize * 2;
   dataBastInfo[0]["TotalLobRead"] += lobPages;
   dataBastInfo[0]["TotalLobAddressing"] += lobPages;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看数据库快照并校验结果，非聚合结果
   for( var i in dataBastInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( dataBastInfoRawData[i]["NodeName"] == masterNodeName )
      {
         dataBastInfoRawData[i]["TotalLobGet"] += 1;
         dataBastInfoRawData[i]["TotalLobReadSize"] += fileSize;
         // 元数据寻址此时增加数量等于大对象页增加数量
         dataBastInfoRawData[i]["TotalLobRead"] += lobPages;
         dataBastInfoRawData[i]["TotalLobAddressing"] += lobPages;
      }
      else if( dataBastInfoRawData[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfoRawData[i]["TotalLobGet"] += 1;
         dataBastInfoRawData[i]["TotalLobReadSize"] += fileSize;
      }
   }
   // 查看快照进行排序，此前的预期结果已进行过排序
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobGet"] += 1;
   csInfo[0]["TotalLobReadSize"] += fileSize;
   csInfo[0]["TotalLobAddressing"] += lobPages;
   csInfo[0]["TotalLobRead"] += lobPages;
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      if( csInfoRawData[i]["NodeName"] == masterNodeName )
      {
         csInfoRawData[i]["TotalLobGet"] += 1;
         csInfoRawData[i]["TotalLobReadSize"] += fileSize;
         csInfoRawData[i]["TotalLobAddressing"] += lobPages;
         csInfoRawData[i]["TotalLobRead"] += lobPages;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      if( clInfo[i]["NodeName"] == masterNodeName )
      {
         clInfo[i]["TotalLobGet"] += 1;
         clInfo[i]["TotalLobReadSize"] += fileSize;
         clInfo[i]["TotalLobAddressing"] += lobPages;
         clInfo[i]["TotalLobRead"] += lobPages;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      if( clInfoRawData[i]["NodeName"] == masterNodeName )
      {
         clInfoRawData[i]["TotalLobGet"] += 1;
         clInfoRawData[i]["TotalLobReadSize"] += fileSize;
         clInfoRawData[i]["TotalLobAddressing"] += lobPages;
         clInfoRawData[i]["TotalLobRead"] += lobPages;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobGet"] += 1;
   clDetail[0]["TotalLobReadSize"] += fileSize;
   clDetail[0]["TotalLobAddressing"] += lobPages;
   clDetail[0]["TotalLobRead"] += lobPages;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   // 删除lob，占多个大对象页
   dbcl.deleteLob( lobID );

   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   dataBastInfo[0]["TotalLobDelete"] += 1 * 2;
   // 元数据寻址此时增加数量等于大对象页增加数量
   dataBastInfo[0]["TotalLobAddressing"] += lobPages;
   dataBastInfo[0]["TotalLobRead"] += lobPages + 1;
   dataBastInfo[0]["TotalLobTruncate"] += lobPages;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看数据库快照并校验结果，非聚合结果
   for( var i in dataBastInfoRawData )
   {
      // 非聚合操作中只有cl所在group主节点返回结果发生变更
      if( dataBastInfoRawData[i]["NodeName"] == masterNodeName )
      {
         dataBastInfoRawData[i]["TotalLobDelete"] += 1;
         dataBastInfoRawData[i]["TotalLobRead"] += lobPages + 1;
         dataBastInfoRawData[i]["TotalLobTruncate"] += lobPages;
         dataBastInfoRawData[i]["TotalLobAddressing"] += lobPages;
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
   csInfo[0]["TotalLobAddressing"] += lobPages;
   csInfo[0]["TotalLobRead"] += lobPages + 1;
   csInfo[0]["TotalLobTruncate"] += lobPages;
   csInfo[0]["TotalUsedLobSpace"] -= lobPageSize * lobPages * nodeNum;
   csInfo[0]["TotalLobs"] -= 1 * nodeNum;
   csInfo[0]["TotalLobPages"] -= lobPages * nodeNum;
   csInfo[0]["TotalLobSize"] -= fileSize * nodeNum;
   csInfo[0]["TotalValidLobSize"] -= fileSize * nodeNum;

   csInfo[0]["LobUsageRate"] = csInfo[0]["TotalLobSize"] / csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["FreeLobSpace"];
   csInfo[0]["AvgLobSize"] = csInfo[0]["TotalLobSize"] / 1;
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      if( csInfoRawData[i]["NodeName"] == masterNodeName )
      {
         csInfoRawData[i]["TotalLobDelete"] += 1;
         csInfoRawData[i]["TotalLobAddressing"] += lobPages;
         // 查看集合空间快照并校验结果，聚合结果
         csInfoRawData[i]["TotalLobRead"] += lobPages + 1;
         csInfoRawData[i]["TotalLobTruncate"] += lobPages;
      }
      csInfoRawData[i]["TotalUsedLobSpace"] -= lobPageSize * lobPages;
      csInfoRawData[i]["TotalLobs"] -= 1;
      csInfoRawData[i]["TotalLobPages"] -= lobPages;
      csInfoRawData[i]["TotalLobSize"] -= fileSize;
      csInfoRawData[i]["TotalValidLobSize"] -= fileSize;

      csInfoRawData[i]["LobUsageRate"] = csInfoRawData[i]["TotalLobSize"] / csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSpace"] = csInfoRawData[i]["LobCapacity"] - csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSize"] = csInfoRawData[i]["FreeLobSpace"];
      csInfoRawData[i]["AvgLobSize"] = csInfoRawData[i]["TotalLobSize"] / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData, lobPages, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      if( clInfo[i]["NodeName"] == masterNodeName )
      {
         clInfo[i]["TotalLobDelete"] += 1;
         clInfo[i]["TotalLobAddressing"] += lobPages;
         clInfo[i]["TotalLobRead"] += lobPages + 1;
         clInfo[i]["TotalLobTruncate"] += lobPages;
      }
      clInfo[i]["TotalLobs"] -= 1;
      clInfo[i]["TotalLobSize"] -= fileSize;
      clInfo[i]["TotalValidLobSize"] -= fileSize;
      clInfo[i]["TotalLobPages"] -= lobPages;
      clInfo[i]["TotalUsedLobSpace"] -= lobPageSize * lobPages;
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
         clInfoRawData[i]["TotalLobAddressing"] += lobPages;
         clInfoRawData[i]["TotalLobRead"] += lobPages + 1;
         clInfoRawData[i]["TotalLobTruncate"] += lobPages;
      }
      clInfoRawData[i]["TotalLobs"] -= 1;
      clInfoRawData[i]["TotalLobSize"] -= fileSize;
      clInfoRawData[i]["TotalValidLobSize"] -= fileSize;
      clInfoRawData[i]["TotalLobPages"] -= lobPages;
      clInfoRawData[i]["TotalUsedLobSpace"] -= lobPageSize * lobPages;
      clInfoRawData[i]["LobUsageRate"] = clInfoRawData[i]["TotalLobSize"] / clInfoRawData[i]["TotalUsedLobSpace"];
      clInfoRawData[i]["AvgLobSize"] = clInfoRawData[i]["TotalValidLobSize"] / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobDelete"] += 1;
   clDetail[0]["TotalLobAddressing"] += lobPages;
   clDetail[0]["TotalLobRead"] += lobPages + 1;
   clDetail[0]["TotalLobTruncate"] += lobPages;
   clDetail[0]["TotalLobs"] -= 1;
   clDetail[0]["TotalLobSize"] -= fileSize;
   clDetail[0]["TotalValidLobSize"] -= fileSize;
   clDetail[0]["TotalLobPages"] -= lobPages;
   clDetail[0]["TotalUsedLobSpace"] -= lobPageSize * lobPages;
   clDetail[0]["LobUsageRate"] = clDetail[0]["TotalLobSize"] / clDetail[0]["TotalUsedLobSpace"];
   clDetail[0]["AvgLobSize"] = clDetail[0]["TotalValidLobSize"] / 1;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   deleteTmpFile( filePath );

   sdb.close();
}