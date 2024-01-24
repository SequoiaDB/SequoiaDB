/******************************************************************************
 * @Description   : seqDB-27846:目标组无lob，源组100%切分至目标组 
 * @Author        : liuli
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.09.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27846";
testConf.clName = COMMCLNAME + "_27846";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27846/";
   var fileName = "filelob_27846";
   var fileSize = 1024 * 50;
   var expCSName = "ecs_27846";
   var expCLName = "ecl_27846";
   var lobs = 10;
   var groupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;

   // 获取源组节点数量
   var srcNodeNum = commGetGroupNodes( db, groupName ).length;
   // 获取目标组节点数量
   var dstNodeNum = commGetGroupNodes( db, dstGroupNames[0] ).length;

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );

   // 由于目标组节点数量难以预测，直接在目标组创建一个CL用来构造预期数据
   commDropCS( db, expCSName );
   var expcl = commCreateCL( db, expCSName, expCLName, { Group: dstGroupNames[0], ReplSize: 0 } );
   for( var i = 0; i < lobs; i++ )
   {
      expcl.putLob( filePath + fileName );
   }

   for( var i = 0; i < lobs; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取数据库快照信息，非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true } );
   var dataBastInfoRawData = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取构造的预期CS非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: expCSName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   var csInfoRawData = getSnapshotLobStat( cursor );

   // 获取构造的预期CL非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: expCSName + "." + expCLName, RawData: true } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData = getSnapshotLobStatToCL( cursor, true );

   // 执行100%切分
   dbcl.split( groupName, dstGroupNames[0], 100 );

   // 查看数据库快照并校验结果，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo );

   // 查看数据库快照并校验结果，非聚合结果
   var option = new SdbSnapshotOption().cond( { RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, option );
   checkSnapshot( cursor, dataBastInfoRawData );

   // 查看集合空间快照并校验结果，聚合结果
   var keys = Object.keys( csInfo[0] );
   for( var i in keys )
   {
      if( keys[i] == "TotalLobPut" || keys[i] == "TotalLobAddressing" || keys[i] == "TotalLobWriteSize" ||
         keys[i] == "TotalLobWrite" )
      {
         csInfo[0][keys[i]] = 0;
      }
      else if( keys[i] != "UsedLobSpaceRatio" && keys[i] != "LobUsageRate" && keys[i] != "AvgLobSize" )
      {
         if( csInfo[0][keys[i]] != undefined )
         {
            csInfo[0][keys[i]] = csInfo[0][keys[i]] / srcNodeNum * dstNodeNum;
         }
      }
   }
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo );

   // 查看集合空间快照并校验结果，非聚合结果
   for( var i in csInfoRawData )
   {
      csInfoRawData[i]["TotalLobPut"] = 0;
      csInfoRawData[i]["TotalLobAddressing"] = 0;
      csInfoRawData[i]["TotalLobWriteSize"] = 0;
      csInfoRawData[i]["TotalLobWrite"] = 0;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
   checkSnapshot( cursor, csInfoRawData );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      clInfoRawData[i]["TotalLobPut"] = 0;
      clInfoRawData[i]["TotalLobAddressing"] = 0;
      clInfoRawData[i]["TotalLobWriteSize"] = 0;
      clInfoRawData[i]["TotalLobWrite"] = 0;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true );

   commDropCS( db, expCSName );
   deleteTmpFile( filePath );
}
