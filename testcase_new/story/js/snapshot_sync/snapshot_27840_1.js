/******************************************************************************
 * @Description   : seqDB-27840 跳过回收站，删除CL，CS在group下还存在CL
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27840_1";
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ( testPara )
{
   var dbcs = testPara.testCS;
   var csName = testConf.csName;
   var clName1 = "cl_27840_11";
   var clName2 = "cl_27840_12";
   var clName3 = "cl_27840_13";
   var filePath = WORKDIR + "/lob27840_1/";
   var fileName = "filelob_27840_1";
   var fileSize = 1024 * 1024;
   var lobPageSize = 262144;
   var groupNames = commGetDataGroupNames( db );
   var nodeNum1 = commGetGroupNodes( db, groupNames[0] ).length;
   var nodeNum2 = commGetGroupNodes( db, groupNames[1] ).length;

   var dbcl1 = dbcs.createCL( clName1, { Group: groupNames[0], ReplSize: 0 } );
   var dbcl2 = dbcs.createCL( clName2, { Group: groupNames[0], ReplSize: 0 } );
   var dbcl3 = dbcs.createCL( clName3, { Group: groupNames[1], ReplSize: 0 } );

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   dbcl1.putLob( filePath + fileName );
   dbcl2.putLob( filePath + fileName );
   dbcl3.putLob( filePath + fileName );

   // 获取数据库快照信息，非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   var dataBastInfoRawData = getSnapshotLobStat( cursor, true );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取集合快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: csName + "." + clName1, RawData: true } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData = getSnapshotLobStatToCL( cursor, true )

   // 获取集合getDetail信息
   var cursor = dbcl1.getDetail();
   var clDetail = getSnapshotLobStatToCL( cursor, true );

   // 删除cl,跳过回收站
   dbcs.dropCL( clName2, { SkipRecycleBin: true } );

   // 查看数据库快照并校验结果，聚合结果
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   checkSnapshot( cursor, dataBastInfoRawData, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobPut"] -= 1;
   csInfo[0]["TotalLobWriteSize"] -= fileSize;
   csInfo[0]["TotalLobWrite"] -= lobPages;
   csInfo[0]["TotalLobAddressing"] -= lobPages;
   csInfo[0]["UsedLobSpaceRatio"] = 0;
   csInfo[0]["TotalUsedLobSpace"] = lobPageSize * lobPages * nodeNum1 + lobPageSize * lobPages * nodeNum2;
   csInfo[0]["TotalLobs"] -= 1 * nodeNum1;
   csInfo[0]["TotalLobPages"] -= lobPages * nodeNum1;
   csInfo[0]["TotalLobSize"] -= fileSize * nodeNum1;
   csInfo[0]["TotalValidLobSize"] -= fileSize * nodeNum1;
   csInfo[0]["LobUsageRate"] = csInfo[0]["TotalValidLobSize"] / csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["FreeLobSpace"];
   csInfo[0]["AvgLobSize"] = csInfo[0]["TotalValidLobSize"] / ( nodeNum1 + nodeNum2 );
   csInfo[0]["UsedLobSpaceRatio"] = 0;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: csName + "." + clName1, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   var cursor = dbcl1.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );

   deleteTmpFile( filePath );
}