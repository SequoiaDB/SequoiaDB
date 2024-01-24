/******************************************************************************
 * @Description   : seqDB-31758:节点已经恢复，超过MinKeepTime自动停止 Critical 模式
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31758";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;
   var clName = COMMCLNAME + "_31758_2";

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
      var minKeepTime = 2;
      var maxKeepTime = 10;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      // 检查Critical模式
      var properties = { NodeName: masterNodeName };
      checkStartCriticalMode( db, srcGroup, properties );

      var beginTime = new Date();

      // 插入数据并校验
      db.transBegin();
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
      db.transCommit();

      // 启动节点，等待节点恢复
      slaveNode1.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );

      checkStartCriticalMode( db, srcGroup, properties );

      // 等待超过MinKeepTime
      var waitTime = minKeepTime + 1;
      validateWaitTime( beginTime, waitTime );

      // 检查Critical模式已经停止
      checkStopCriticalMode( db, srcGroup );

      // 插入数据并校验
      var options = { ReplSize: 0, Group: srcGroup };
      var dbcl2 = args.testCS.createCL( clName, options );
      var docs2 = insertBulkData( dbcl2, 1000 );
      var cursor = dbcl2.find().sort( { a: 1 } );
      commCompareResults( cursor, docs2 );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
      commDropCL( db, testConf.csName, clName, true, true );
      db.updateConf( { sharingbreak: actSharingbreak } );
   }
}