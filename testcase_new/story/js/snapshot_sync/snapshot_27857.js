/******************************************************************************
 * @Description   : seqDB-27857:getLob操作失败，查看统计指标
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.13
 * @LastEditTime  : 2023.04.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_27857";
testConf.clName = COMMCLNAME + "_27857";
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob27857/";
   var fileName = "filelob_27857";
   var fileSize = 1024 * 50;
   var lobPageSize = 262144;

   // 获取cl所在的group的主节点,部分操作只在主节点统计
   var groupName = testPara.srcGroupName;
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 执行多次putLob
   deleteTmpFile( filePath + fileName )
   makeTmpFile( filePath, fileName, fileSize );
   for( var i = 0; i < 20; i++ )
   {
      dbcl.putLob( filePath + fileName );
   }
   var lobID = dbcl.putLob( filePath + fileName );

   // 获取集合空间快照信息，聚合结果
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   var csInfo = getSnapshotLobStat( cursor );

   // 获取集合快照信息，非聚合结果
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   var clInfoRawData = getSnapshotLobStatToCL( cursor, true );

   // 获取集合getDetail信息
   var cursor = dbcl.getDetail();
   var clDetail = getSnapshotLobStatToCL( cursor, true );

   assert.tryThrow( SDB_FE, function()
   {
      dbcl.getLob( lobID, filePath + fileName );
   } );

   // 查看集合空间快照并校验结果，聚合结果
   var lobPages = parseInt( ( fileSize + 1023 ) / lobPageSize ) + 1;
   csInfo[0]["TotalLobGet"] += 1;
   csInfo[0]["TotalLobReadSize"] += fileSize;
   csInfo[0]["TotalLobRead"] += lobPages;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
   checkSnapshot( cursor, csInfo, lobPages );

   // 查看集合快照并校验结果，非聚合结果
   for( var i in clInfoRawData )
   {
      if( clInfoRawData[i]["NodeName"] == masterNodeName )
      {
         clInfoRawData[i]["TotalLobGet"] += 1;
         clInfoRawData[i]["TotalLobReadSize"] += fileSize;
         clInfoRawData[i]["TotalLobRead"] += lobPages;
      }
   }
   var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
      .sort( { "Details.NodeName": 1 } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, option );
   checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

   // 校验集合getDetail信息
   clDetail[0]["TotalLobGet"] += 1;
   clDetail[0]["TotalLobReadSize"] += fileSize;
   clDetail[0]["TotalLobRead"] += lobPages;
   var cursor = dbcl.getDetail();
   checkSnapshotToCL( cursor, clDetail, true, lobPages );
   deleteTmpFile( filePath + fileName )
}