/******************************************************************************
 * @Description   : seqDB-31755:数据节点2副本异常，重复启动 Critical 模式
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.10.31
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31755";

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取sharingbreak
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { GroupName: srcGroup }, { sharingbreak: 1 } );
   var actSharingbreak = cursor.current().toObj().sharingbreak;
   println( "actSharingbreak: " + actSharingbreak );

   // 修改sharingbreak为默认值
   db.deleteConf( { sharingbreak: 1 } );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 停止group中2个备节点，先异常停止再正常停止，让节点停止之后不启动
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   try
   {
      killNode( db, slaveNode1 );
      killNode( db, slaveNode2 );

      // 剩余一个节点启动Critical模式
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      var properties1 = getCriticalStatus( db, srcGroup );

      // 重复启动Critical模式
      var minKeepTime = 10;
      var maxKeepTime = 20;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );
      var properties2 = getCriticalStatus( db, srcGroup );

      assert.notEqual( properties1, properties2 );

      // 检查Critical模式
      var properties = { NodeName: masterNodeName };
      checkStartCriticalMode( db, srcGroup, properties );


      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 启动节点
      slaveNode1.start();
      slaveNode2.start();

      // 等待LSN一致
      commCheckBusinessStatus( db );

      // 停止Critical模式
      rg.stopCriticalMode();

      // 检查Critical模式停止
      checkStopCriticalMode( db, srcGroup );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
      // 恢复actSharingbreak配置
      db.updateConf( { sharingbreak: actSharingbreak } );
   }
}

function getCriticalStatus ( db, groupName )
{
   var groupMode = "critical";
   var actProperties;

   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   while( cursor.next() )
   {
      var groupModeInfo = cursor.current().toObj();
      var actGroupMode = groupModeInfo["GroupMode"];
      actProperties = groupModeInfo["Properties"][0];
      delete actProperties.UpdateTime;

      assert.equal( actGroupMode, groupMode );
   }
   cursor.close();
   return actProperties;
}