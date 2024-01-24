/******************************************************************************
 * @Description   : seqDB-27844:目标组有lob，源组100%切分至目标组
 * @Author        : liuli
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27844";
testConf.clName = COMMCLNAME + "_27844";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27844/";
   var fileName = "filelob_27844";
   var fileSize = 1024 * 50;
   var lobs = 30;
   var lobPageSize = 262144;
   var groupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;

   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   // 获取源组节点数量和主节点名称
   var srcNodeNum = commGetGroupNodes( db, groupName ).length;

   // 获取目标组节点数量
   var dstNodeNum = commGetGroupNodes( db, dstGroupNames[0] ).length;

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );

   // putLob后执行split
   for( var i = 0; i < lobs; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }
   dbcl.split( groupName, dstGroupNames[0], 50 );

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取数据库快照信息，非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   var dataBastInfoRawData = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取构造的预期CS非聚合结果，只匹配目标组
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true, Group: dstGroupNames[0] } )
      .sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData = getSnapshotLobStat( cursor );

   // 获取构造的预期CL非聚合结果
   var option = new SdbSnapshotOption().cond( {
      Name: testConf.csName + "." + testConf.clName, RawData: true, Group: dstGroupNames[0]
   } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData = getSnapshotLobStatToCL( cursor, true );

   // 执行100%切分，目标组有数据
   dbcl.split( groupName, dstGroupNames[0], 100 );

   // 查看数据库快照并校验结果，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo );

   // 查看数据库快照并校验结果，非聚合结果
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobs"] = lobs * dstNodeNum;
   csInfo[0]["TotalLobPages"] = lobs * lobPages * dstNodeNum;
   csInfo[0]["TotalLobSize"] = fileSize * lobs * dstNodeNum;
   csInfo[0]["TotalValidLobSize"] = fileSize * lobs * dstNodeNum;
   csInfo[0]["TotalUsedLobSpace"] = lobs * lobPages * lobPageSize * dstNodeNum;
   csInfo[0]["TotalLobPut"] = 0;
   csInfo[0]["TotalLobWriteSize"] = 0;
   csInfo[0]["TotalLobWrite"] = 0;
   csInfo[0]["TotalLobAddressing"] = 0;
   csInfo[0]["LobCapacity"] = csInfo[0]["LobCapacity"] / ( dstNodeNum + srcNodeNum ) * dstNodeNum;
   csInfo[0]["LobMetaCapacity"] = csInfo[0]["LobMetaCapacity"] / ( dstNodeNum + srcNodeNum ) * dstNodeNum;
   csInfo[0]["MaxLobCapacity"] = csInfo[0]["MaxLobCapacity"] / ( dstNodeNum + srcNodeNum ) * dstNodeNum;
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["UsedLobSpaceRatio"] = Math.floor( csInfo[0]["TotalUsedLobSpace"] / csInfo[0]["LobCapacity"] * 100 ) / 100;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo );

   for( var i in csInfoRawData )
   {
      csInfoRawData[i]["TotalLobs"] = lobs;
      csInfoRawData[i]["TotalLobPages"] = lobs * lobPages;
      csInfoRawData[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * lobs;
      csInfoRawData[i]["TotalLobSize"] = fileSize * lobs;
      csInfoRawData[i]["TotalValidLobSize"] = fileSize * lobs;
      csInfoRawData[i]["TotalUsedLobSpace"] = csInfo[0]["TotalUsedLobSpace"] / dstNodeNum;

      csInfoRawData[i]["UsedLobSpaceRatio"] = Math.floor( csInfoRawData[i]["TotalUsedLobSpace"] / csInfoRawData[i]["LobCapacity"] * 100 ) / 100;
      csInfoRawData[i]["LobUsageRate"] = csInfoRawData[i]["TotalLobSize"] / csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSpace"] = csInfoRawData[i]["LobCapacity"] - csInfoRawData[i]["TotalUsedLobSpace"];
      csInfoRawData[i]["FreeLobSize"] = csInfoRawData[i]["FreeLobSpace"];
      csInfoRawData[i]["AvgLobSize"] = fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      clInfoRawData[i]["TotalLobs"] = lobs;
      clInfoRawData[i]["TotalLobPages"] = lobs * lobPages;
      clInfoRawData[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * lobs;
      clInfoRawData[i]["TotalLobSize"] = fileSize * lobs;
      clInfoRawData[i]["TotalValidLobSize"] = fileSize * lobs;
      clInfoRawData[i]["LobUsageRate"] = clInfoRawData[i]["TotalLobSize"] / clInfoRawData[i]["TotalUsedLobSpace"];
      clInfoRawData[i]["UsedLobSpaceRatio"] = Math.floor( clInfoRawData[i]["TotalUsedLobSpace"] / clInfoRawData[i]["LobCapacity"] * 100 ) / 100
      clInfoRawData[i]["AvgLobSize"] = fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true );

   deleteTmpFile( filePath );
}
