/******************************************************************************
 * @Description   : seqDB-31756:数据节点2副本异常，启动 Critical 模式的节点异常
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.10.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31756";

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

   // 停止group中2个备节点
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   try
   {
      killNode( db, slaveNode1 );
      killNode( db, slaveNode2 );

      // 剩余一个节点启动Critical模式
      var minKeepTime = 1;
      var maxKeepTime = 2;
      var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );
      var beginTime = new Date();

      // 异常停止启动Critical模式的节点
      killNode( db, masterNode );

      // 等待环境恢复
      rg.start();
      commCheckBusinessStatus( db );

      // 检查Critical模式
      var waitTime = minKeepTime + 2;
      validateWaitTime( beginTime, waitTime );

      // 插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   finally
   {
      rg.start();
      rg.stopCriticalMode();
      commCheckBusinessStatus( db );
   }
}