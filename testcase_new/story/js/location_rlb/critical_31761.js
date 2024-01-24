/******************************************************************************
 * @Description   : seqDB-31761:超过MaxKeepTime节点未恢复，自动停止Critical模式
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.10.31
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31761";
testConf.clOpt = { ReplSize: 0 };

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
      var minKeepTime = 1;
      var maxKeepTime = 3;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      var beginTime = new Date();

      // 等待超过MinKeepTime
      var waitTime = minKeepTime + 1;
      validateWaitTime( beginTime, waitTime );
      var properties = { NodeName: masterNodeName };
      checkStartCriticalMode( db, srcGroup, properties );

      // 等待超过MaxKeepTime
      var waitTime = maxKeepTime + 1;
      validateWaitTime( beginTime, waitTime );
      checkStopCriticalMode( db, srcGroup );

      // 插入数据报错
      assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
      {
         dbcl.insert( { a: 1 } );
      } );

      // 启动节点
      slaveNode1.start();
      slaveNode2.start();
      // 等待LSN一致
      commCheckBusinessStatus( db );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
      db.updateConf( { sharingbreak: actSharingbreak } );
   }
}