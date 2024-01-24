/******************************************************************************
 * @Description   : seqDB-27850 持续putLob使lobd扩展
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.09.27
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName1 = "cs_27850_1";
   var csName2 = "cs_27850_2";
   var clName1 = "cl_27850_1";
   var clName2 = "cl_27850_2";
   var filePath = WORKDIR + "/lob27850/";
   var fileName = "filelob_27850";
   var fileSize = 1024 * 400;
   var lobPageSize = 524288;

   // coord节点会统计部分负载监控指标，重新连接一个coord确定使用coord NodeName
   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );

   var groupNames = commGetDataGroupNames( db );
   var masterNode1 = sdb.getRG( groupNames[0] ).getMaster();
   var masterNodeName1 = masterNode1.getHostName() + ":" + masterNode1.getServiceName();

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   var dbcs1 = sdb.createCS( csName1, { LobPageSize: 524288 } );
   var dbcl1 = dbcs1.createCL( clName1, { Group: groupNames[0], ReplSize: 0 } );
   var dbcl2 = commCreateCL( db, csName2, clName2, { Group: groupNames[1], ReplSize: 0 } );

   // 每个CL都执行putLob
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   dbcl1.putLob( filePath + fileName );
   dbcl2.putLob( filePath + fileName );

   // 获取数据库快照信息，聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取集合空间CS1快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: csName1, RawData: true } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData1 = getSnapshotLobStat( cursor );

   // 获取集合空间CS2快照信息，聚合结果
   var option = new SdbSnapshotOption().cond( { Name: csName2 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfo2 = getSnapshotLobStat( cursor );

   // 获取集合快照信息，聚合结果,
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName1 + "." + clName1 } );
   var clInfo = getSnapshotLobStatToCL( cursor, false );

   // 获取集合getDetail信息
   var cursor = dbcl2.getDetail();
   var clDetail = getSnapshotLobStatToCL( cursor, true );

   // 持续putLob使lobd扩展一次
   var lobs = 300;
   for( var i = 0; i < lobs; i++ )
   {
      dbcl1.putLob( filePath + fileName );
   }

   commCheckLSN( db, groupNames[0] );
   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   // Number of load monitoring nodes
   var loadMonitorNodeNum = 2;
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   dataBastInfo[0]["TotalLobPut"] += lobs * loadMonitorNodeNum;
   dataBastInfo[0]["TotalLobWriteSize"] += fileSize * lobs * loadMonitorNodeNum;
   dataBastInfo[0]["TotalLobWrite"] += lobs;
   // 元数据寻址此时增加数量等于大对象页增加数量
   dataBastInfo[0]["TotalLobAddressing"] += lobPages * lobs;
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看集合空间cs1快照并校验结果，非聚合结果
   for( var i in csInfoRawData1 )
   {
      if( csInfoRawData1[i]["NodeName"] == masterNodeName1 )
      {
         csInfoRawData1[i]["TotalLobPut"] += lobs;
         csInfoRawData1[i]["TotalLobWriteSize"] += fileSize * lobs;
         csInfoRawData1[i]["TotalLobWrite"] += lobs;
         // 元数据寻址此时增加数量等于大对象页增加数量
         csInfoRawData1[i]["TotalLobAddressing"] += lobPages * lobs;
      }
      csInfoRawData1[i]["TotalLobs"] += lobs;
      csInfoRawData1[i]["TotalLobPages"] += lobPages * lobs;
      csInfoRawData1[i]["TotalUsedLobSpace"] += lobPages * lobs * lobPageSize;
      csInfoRawData1[i]["LobCapacity"] += lobdSize;
      csInfoRawData1[i]["TotalLobSize"] += fileSize * lobs;
      csInfoRawData1[i]["TotalValidLobSize"] += fileSize * lobs;
      csInfoRawData1[i]["MaxLobCapSize"] -= lobdSize;

      csInfoRawData1[i]["FreeLobSpace"] = csInfoRawData1[i]["LobCapacity"] - csInfoRawData1[i]["TotalUsedLobSpace"];
      csInfoRawData1[i]["FreeLobSize"] = csInfoRawData1[i]["FreeLobSpace"];
      csInfoRawData1[i]["UsedLobSpaceRatio"] = csInfoRawData1[i]["TotalUsedLobSpace"] / csInfoRawData1[i]["LobCapacity"];
   }
   var option = new SdbSnapshotOption().cond( { Name: csName1, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData1, lobPages );

   // 查看集合空间cs2快照并校验结果，非聚合结果
   csInfo2[0]["MaxLobCapSize"] -= lobdSize;
   var option = new SdbSnapshotOption().cond( { Name: clName2 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfo2, lobPages );

   // 查看集合cl1快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      if( clInfo[i]["NodeName"] == masterNodeName1 )
      {
         clInfo[i]["TotalLobPut"] += lobs;
         clInfo[i]["TotalLobWriteSize"] += fileSize * lobs;
         clInfo[i]["TotalLobWrite"] += lobs;
         // 元数据寻址此时增加数量等于大对象页增加数量
         clInfo[i]["TotalLobAddressing"] += lobPages * lobs;
      }
      clInfo[i]["TotalLobs"] += lobs;
      clInfo[i]["TotalLobPages"] += lobPages * lobs;
      clInfo[i]["TotalUsedLobSpace"] += lobPages * lobs * lobPageSize;
      clInfo[i]["TotalLobSize"] += fileSize * lobs;
      clInfo[i]["TotalValidLobSize"] += fileSize * lobs;
      clInfo[i]["UsedLobSpaceRatio"] = clInfo[i]["TotalUsedLobSpace"] / ( lobdSize * 2 );
   }
   var option = new SdbSnapshotOption().cond( { Name: csName1 + "." + clName1 } );
   var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 查看集合cl1快照并校验结果，聚合结果
   var cursor = dbcl2.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   sdb.close();
   deleteTmpFile( filePath + fileName )
}