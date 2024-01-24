/******************************************************************************
 * @Description   : seqDB-31754:数据节点2副本异常，剩余节点设置 Critical 模式
 * @Author        : liuli
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31754";

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
      var minKeepTime = 10;
      var maxKeepTime = 20;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      // 检查Critical模式
      checkStartCriticalMode( db, srcGroup, options );

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
      slaveNode1.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );
   }
}