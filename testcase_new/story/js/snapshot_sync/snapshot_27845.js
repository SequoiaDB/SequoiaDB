/******************************************************************************
 * @Description   : seqDB-27845:目标组有lob，源组部分切分至目标组
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.15
 * @LastEditTime  : 2022.10.25
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27845";
testConf.clName = COMMCLNAME + "_27845";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27845/";
   var fileName = "filelob_27845";
   var fileSize = 1024 * 50;
   var lobs = 30;
   var lobPageSize = 262144;
   var groupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;

   var masterNode1 = db.getRG( groupName ).getMaster();
   var data = masterNode1.connect();
   var dataCL = data.getCS( testConf.csName ).getCL( testConf.clName );
   var masterNodeName1 = masterNode1.getHostName() + ":" + masterNode1.getServiceName();
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;

   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );

   // putLob占多个大对象页
   for( var i = 0; i < lobs; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }

   // putLob后执行split
   dbcl.split( groupName, dstGroupNames[0], 50 );

   // 获取数据库快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   var dataBastInfo = getSnapshotLobStat( cursor );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取集合快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: groupName } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData1 = getSnapshotLobStatToCL( cursor, true );

   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: dstGroupNames[0] } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData2 = getSnapshotLobStatToCL( cursor, true );
   // 执行部分切分
   dbcl.split( groupName, dstGroupNames[0], 50 );

   // 查看数据库快照并校验结果，聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE );
   checkSnapshot( cursor, dataBastInfo );

   // 查看集合空间快照并校验结果，聚合结果
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
      clInfoRawData1[i]["TotalLobPages"] = dataLobs * lobPages;
      clInfoRawData1[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * dataLobs;
      clInfoRawData1[i]["TotalLobSize"] = fileSize * dataLobs;
      clInfoRawData1[i]["TotalValidLobSize"] = fileSize * dataLobs;
      clInfoRawData1[i]["LobUsageRate"] = clInfoRawData1[i]["TotalLobSize"] / clInfoRawData1[i]["TotalUsedLobSpace"];
      clInfoRawData1[i]["UsedLobSpaceRatio"] = Math.floor( clInfoRawData1[i]["TotalUsedLobSpace"] / clInfoRawData1[i]["LobCapacity"] * 100 ) / 100
      clInfoRawData1[i]["AvgLobSize"] = fileSize / 1;
      if( clInfoRawData1[i]["NodeName"] == masterNodeName1 )
      {
         clInfoRawData1[i]["TotalLobList"] = 1;
         clInfoRawData1[i]["TotalLobRead"] += lobPages * dataLobs;
         clInfoRawData1[i]["TotalLobAddressing"] += lobPages * dataLobs;
      }
   }

   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: groupName } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData1, true );

   // 校验目标组集合快照，非聚合结果
   for( var i in clInfoRawData2 )
   {
      clInfoRawData2[i]["TotalLobs"] = ( lobs - dataLobs );
      clInfoRawData2[i]["TotalLobPages"] = ( lobs - dataLobs ) * lobPages;
      clInfoRawData2[i]["TotalUsedLobSpace"] = lobPageSize * lobPages * ( lobs - dataLobs );
      clInfoRawData2[i]["TotalLobSize"] = fileSize * ( lobs - dataLobs );
      clInfoRawData2[i]["TotalValidLobSize"] = fileSize * ( lobs - dataLobs );
      clInfoRawData2[i]["LobUsageRate"] = clInfoRawData2[i]["TotalLobSize"] / clInfoRawData2[i]["TotalUsedLobSpace"];
      clInfoRawData2[i]["UsedLobSpaceRatio"] = Math.floor( clInfoRawData2[i]["TotalUsedLobSpace"] / clInfoRawData2[i]["LobCapacity"] * 100 ) / 100
      clInfoRawData2[i]["AvgLobSize"] = fileSize / 1;
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true, GroupName: dstGroupNames[0] } ).sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData2, true );

   data.close();
   deleteTmpFile( filePath );
}