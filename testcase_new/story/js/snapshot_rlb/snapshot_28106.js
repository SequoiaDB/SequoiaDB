/******************************************************************************
 * @Description   : seqDB-28016:getLob访问备节点 
 * @Author        : liuli
 * @CreateTime    : 2022.09.25
 * @LastEditTime  : 2023.04.23
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_28016";
testConf.clName = COMMCLNAME + "_28016";
testConf.clOpt = { ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var filePath = WORKDIR + "/lob28016/";
   var fileName = "filelob_28016";
   var fileSize = 1024 * 50;

   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );
   var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

   var groupName = testPara.srcGroupName;
   var slaveNode = sdb.getRG( groupName ).getSlave();
   var slaveNodeName = slaveNode.getHostName() + ":" + slaveNode.getServiceName();

   // putLob占多个大对象页
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   try
   {
      var instanceid = 11;
      try
      {
         sdb.updateConf( { instanceid: instanceid }, { NodeName: slaveNodeName } );
      }
      catch( e )
      {
         if( e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
         {
            throw new Error( e );
         }
      }
      slaveNode.stop();
      slaveNode.start();
      commCheckBusinessStatus( db );

      // 设置会话属性，访问对应节点
      sdb.setSessionAttr( { preferredinstance: instanceid } );

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

      // 执行getLob
      dbcl.getLob( lobID, filePath + "checkputlob29106", true );

      // 查看数据库快照并校验结果，聚合结果
      // dataBastInfo聚合结果只返回一条，对应结果变更，对预期结果进行修改
      dataBastInfo[0]["TotalLobGet"] += 1 * 2;
      // 元数据寻址此时增加数量等于大对象页增加数量
      var lobPages = parseInt( ( fileSize + 1023 ) / 262144 ) + 1;
      dataBastInfo[0]["TotalLobAddressing"] += lobPages;
      dataBastInfo[0]["TotalLobReadSize"] += fileSize * 2;
      dataBastInfo[0]["TotalLobRead"] += lobPages;
      var cursor = sdb.snapshot( SDB_SNAP_DATABASE );
      checkSnapshot( cursor, dataBastInfo, lobPages );

      // 查看数据库快照并校验结果，非聚合结果
      for( var i in dataBastInfoRawData )
      {
         // 非聚合操作中只有cl所在group主节点返回结果发生变更
         if( dataBastInfoRawData[i]["NodeName"] == slaveNodeName )
         {
            dataBastInfoRawData[i]["TotalLobGet"] += 1;
            dataBastInfoRawData[i]["TotalLobRead"] += lobPages;
            // 元数据寻址此时增加数量等于大对象页增加数量
            dataBastInfoRawData[i]["TotalLobAddressing"] += lobPages;
            dataBastInfoRawData[i]["TotalLobReadSize"] += fileSize;
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
      csInfo[0]["TotalLobRead"] += lobPages;
      csInfo[0]["TotalLobAddressing"] += lobPages;
      csInfo[0]["TotalLobReadSize"] += fileSize;
      var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: testConf.csName } );
      checkSnapshot( cursor, csInfo, lobPages );

      // 查看集合空间快照并校验结果，非聚合结果
      for( var i in csInfoRawData )
      {
         if( csInfoRawData[i]["NodeName"] == slaveNodeName )
         {
            csInfoRawData[i]["TotalLobGet"] += 1;
            csInfoRawData[i]["TotalLobRead"] += lobPages;
            csInfoRawData[i]["TotalLobAddressing"] += lobPages;
            csInfoRawData[i]["TotalLobReadSize"] += fileSize;
         }
      }
      var option = new SdbSnapshotOption().cond( { Name: testConf.csName, RawData: true } ).sort( { NodeName: 1 } );
      var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONSPACES, option );
      checkSnapshot( cursor, csInfoRawData, lobPages );

      // 查看集合快照并校验结果，聚合结果
      for( var i in clInfoRawData )
      {
         if( clInfoRawData[i]["NodeName"] == slaveNodeName )
         {
            clInfo[i]["TotalLobGet"] += 1;
            clInfo[i]["TotalLobRead"] += lobPages;
            clInfo[i]["TotalLobAddressing"] += lobPages;
            clInfo[i]["TotalLobReadSize"] += fileSize;
         }
      }
      var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName } );
      var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
      checkSnapshotToCL( cursor, clInfo, false, lobPages );

      // 查看集合快照并校验结果，非聚合结果
      for( var i in clInfoRawData )
      {
         if( clInfoRawData[i]["NodeName"] == slaveNodeName )
         {
            clInfoRawData[i]["TotalLobGet"] += 1;
            clInfoRawData[i]["TotalLobRead"] += lobPages;
            clInfoRawData[i]["TotalLobAddressing"] += lobPages;
            clInfoRawData[i]["TotalLobReadSize"] += fileSize;
         }
      }
      var option = new SdbSnapshotOption().cond( { Name: testConf.csName + "." + testConf.clName, RawData: true } )
         .sort( { "Details.NodeName": 1 } );
      var cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, option );
      checkSnapshotToCL( cursor, clInfoRawData, true, lobPages );

      // 校验集合getDetail信息
      clDetail[0]["TotalLobGet"] += 1;
      clDetail[0]["TotalLobRead"] += lobPages;
      clDetail[0]["TotalLobAddressing"] += lobPages;
      clDetail[0]["TotalLobReadSize"] += fileSize;
      var cursor = dbcl.getDetail();
      checkSnapshotToCL( cursor, clDetail, true, lobPages );

      deleteTmpFile( filePath );
   }
   finally
   {
      try
      {
         sdb.deleteConf( { instanceid: 1 } );
      } catch( e )
      {
         if( e != SDB_COORD_NOT_ALL_DONE )
         {
            throw new Error( e );
         }
      }
      slaveNode.stop();
      slaveNode.start();
      commCheckBusinessStatus( sdb );
   }

   sdb.close();
}