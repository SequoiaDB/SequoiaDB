/******************************************************************************
 * @Description   : seqDB-27847:目标组无lob，源组部分切分至目标组
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.13
 * @LastEditTime  : 2023.09.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27847";
testConf.clName = COMMCLNAME + "_27847";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27847/";
   var fileName = "filelob_27847";
   var fileSize = 1024 * 50;
   var lobPageSize = 262144;
   var expCSName = "ecs_27847";
   var expCLName = "ecl_27847";
   var lobs = 50;
   // 获取cl所在group的主节点，部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];

   var masterNode1 = db.getRG( groupName ).getMaster();
   var data = masterNode1.connect();
   var dataCL = data.getCS( testConf.csName ).getCL( testConf.clName );
   var masterNodeName1 = masterNode1.getHostName() + ":" + masterNode1.getServiceName();
   var dstNodeNum = commGetGroupNodes( db, dstGroupName ).length;

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );

   // 由于目标组节点数量难以预测，直接在目标组创建一个CL用来构造预期数据
   commDropCS( db, expCSName );
   var expcl = commCreateCL( db, expCSName, expCLName, { ShardingKey: { a: 1 }, Group: dstGroupName, ReplSize: 0 } );
   for( var i = 0; i < lobs; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }
   for( var i = 0; i < lobs; i++ )
   {
      expcl.putLob( filePath + fileName );
   }

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 集合快照非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: groupName } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData1 = getSnapshotLobStatToCL( cursor, true );
   var option = new SdbSnapshotOption().cond( { Name: expCSName + "." + expCLName, RawData: true, GroupName: dstGroupName } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData2 = getSnapshotLobStatToCL( cursor, true );

   // 执行部分切分,切分之后，group1和group2都有数据
   dbcl.split( groupName, dstGroupName, 50 );

   // 查看数据库快照并校验结果，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo );

   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   // 查看集合空间快照并校验结果，聚合结果
   csInfo[0]["LobMetaCapacity"] += lobmSize * dstNodeNum;
   csInfo[0]["LobCapacity"] += lobdSize * dstNodeNum;
   csInfo[0]["MaxLobCapacity"] += csInfo[0]["MaxLobCapacity"];
   csInfo[0]["FreeLobSpace"] = csInfo[0]["LobCapacity"] - csInfo[0]["TotalUsedLobSpace"];
   csInfo[0]["FreeLobSize"] = csInfo[0]["FreeLobSpace"];
   // 实际结果小于0.01，显示为0
   csInfo[0]["UsedLobSpaceRatio"] = Math.floor( csInfo[0]["TotalUsedLobSpace"] / csInfo[0]["LobCapacity"] * 100 ) / 100;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo );

   var cursor = dataCL.listLobs();
   var dataLobs = 0;
   while( cursor.next() )
   {
      dataLobs++;
   }
   cursor.close();

   // 校验源组集合快照，非聚合结果
   for( var i in clInfoRawData1 )
   {
      clInfoRawData1[i]["TotalLobs"] = dataLobs;
      clInfoRawData1[i]["TotalLobPages"] = dataLobs;
      clInfoRawData1[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * dataLobs;
      clInfoRawData1[i]["TotalLobSize"] = fileSize * dataLobs;
      clInfoRawData1[i]["TotalValidLobSize"] = fileSize * dataLobs;
      clInfoRawData1[i]["UsedLobSpaceRatio"] = Math.floor( clInfoRawData1[i]["TotalUsedLobSpace"] / clInfoRawData1[i]["LobCapacity"] * 100 ) / 100;
      clInfoRawData1[i]["LobUsageRate"] = clInfoRawData1[i]["TotalLobSize"] / clInfoRawData1[i]["TotalUsedLobSpace"];
      if( clInfoRawData1[i]["NodeName"] == masterNodeName1 )
      {
         clInfoRawData1[i]["TotalLobList"] = 1;
         clInfoRawData1[i]["TotalLobRead"] += lobPages * dataLobs;
         clInfoRawData1[i]["TotalLobAddressing"] += lobPages * dataLobs;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: groupName } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData1, true, lobPages );

   // 校验目标组集合快照，非聚合结果
   for( var i in clInfoRawData2 )
   {
      clInfoRawData2[i]["TotalLobs"] = ( lobs - dataLobs );
      clInfoRawData2[i]["TotalLobPut"] = 0;
      clInfoRawData2[i]["TotalLobAddressing"] = 0;
      clInfoRawData2[i]["TotalLobWriteSize"] = 0;
      clInfoRawData2[i]["TotalLobWrite"] = 0;
      clInfoRawData2[i]["TotalLobPages"] = ( lobs - dataLobs );
      clInfoRawData2[i]["AvgLobSize"] = fileSize / 1;
      clInfoRawData2[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * ( lobs - dataLobs );
      clInfoRawData2[i]["TotalLobSize"] = fileSize * ( lobs - dataLobs );
      clInfoRawData2[i]["TotalValidLobSize"] = fileSize * ( lobs - dataLobs );
      clInfoRawData2[i]["UsedLobSpaceRatio"] = Math.floor( clInfoRawData2[i]["TotalUsedLobSpace"] / clInfoRawData2[i]["LobCapacity"] * 100 ) / 100;
      clInfoRawData2[i]["LobUsageRate"] = clInfoRawData2[i]["TotalLobSize"] / clInfoRawData2[i]["TotalUsedLobSpace"];
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: dstGroupName } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData2, true, lobPages );

   data.close();
   deleteTmpFile( filePath );
   commDropCS( db, expCSName );
}