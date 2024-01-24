/******************************************************************************
 * @Description   : seqDB-31760:超过MinKeepTime节点未恢复，手动停止Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31760";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dbcl = args.testCL;

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

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
      var maxKeepTime = 20;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      var beginTime = new Date();

      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 等待超过MinKeepTime
      var waitTime = minKeepTime + 1;
      validateWaitTime( beginTime, waitTime );

      // 停止Critical模式
      rg.stopCriticalMode();

      // 检查Critical模式已经停止
      checkStopCriticalMode( db, srcGroup );

      // 插入数据报错
      assert.tryThrow( SDB_CLS_NODE_NOT_ENOUGH, function()
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
      slaveNode1.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );
   }
}