/******************************************************************************
 * @Description   : seqDB-27842:CL执行truncate,不跳过回收站
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.27
 * @LastEditTime  : 2023.06.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27842_2";
testConf.csOpt = { LobPageSize: 262144 };
testConf.clName = COMMCLNAME + "_27842_2";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27842_2/";
   var fileName = "filelob_27842_2";
   var lobPageSize = 262144;
   var fileSize = 1024 * 200;

   // putLob
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobs = 100;
   for( var i = 0; i < lobs; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取集合快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: testConf.csName + "." + testConf.clName } );
   var clInfo = getSnapshotLobStatToCL( cursor, false );

   // 获取集合getDetail信息
   var cursor = dbcl.getDetail();
   var clDetail = getSnapshotLobStatToCL( cursor, true );

   // 执行truncate,不跳过回收站
   dbcl.truncate();

   // 查看数据库快照并校验结果，聚合结果
   // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   // 元数据寻址此时增加数量等于大对象页增加数量
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo, lobPages );

   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["TotalLobs"] = 0;
   csInfo[0]["TotalLobPages"] = 0;
   csInfo[0]["TotalLobSize"] = 0;
   csInfo[0]["TotalValidLobSize"] = 0;
   csInfo[0]["LobUsageRate"] = 0;
   csInfo[0]["AvgLobSize"] = 0;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合快照并校验结果，聚合结果
   for( var i in clInfo )
   {
      var lobPages = parseInt( ( fileSize - 1 ) / lobPageSize ) + 1;
      clInfo[i]["TotalLobs"] = 0;
      clInfo[i]["TotalLobPages"] = 0;
      clInfo[i]["TotalUsedLobSpace"] = 0;
      clInfo[i]["UsedLobSpaceRatio"] = 0;
      clInfo[i]["TotalLobSize"] = 0;
      clInfo[i]["TotalValidLobSize"] = 0;
      clInfo[i]["LobUsageRate"] = 0;
      clInfo[i]["AvgLobSize"] = 0;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfo, false, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobs"] = 0;
   clDetail[0]["TotalLobPages"] = 0;
   clDetail[0]["TotalUsedLobSpace"] = 0;
   clDetail[0]["UsedLobSpaceRatio"] = 0;
   clDetail[0]["TotalLobSize"] = 0;
   clDetail[0]["TotalValidLobSize"] = 0;
   clDetail[0]["LobUsageRate"] = 0;
   clDetail[0]["AvgLobSize"] = 0;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );
   deleteTmpFile( filePath );
}