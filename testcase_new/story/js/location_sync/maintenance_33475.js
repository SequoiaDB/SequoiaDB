/******************************************************************************
 * @Description   : seqDB-33475:已启动运维模式的复制组启动Critical模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var groupNames = commGetDataGroupNames( db );
   var groupName1 = groupNames[0];
   var groupName2 = groupNames[1];
   var group1 = db.getRG( groupName1 );
   var group2 = db.getRG( groupName2 );

   // 获取一个备节点名
   var slaveNodeNames = getGroupSlaveNodeName( db, groupName1 );
   var slaveNode1 = group1.getNode( slaveNodeNames[0] );
   var slaveNode2 = group1.getNode( slaveNodeNames[1] );
   var slaveNode3 = group2.getSlave();

   try
   {
      // 指定备节点启动运维模式
      var slaveNodeName1 = slaveNode1.getHostName() + ":" + slaveNode1.getServiceName();
      var options = { NodeName: slaveNodeName1, MinKeepTime: 5, MaxKeepTime: 10 };
      group1.startMaintenanceMode( options );

      // 指定相同节点启动critical 模式
      assert.tryThrow( SDB_OPERATION_CONFLICT, function()
      {
         group1.startCriticalMode( options );
      } );

      // 指定不同节点启动critical 模式
      var slaveNodeName2 = slaveNode2.getHostName() + ":" + slaveNode2.getServiceName();
      var options = { NodeName: slaveNodeName2, MinKeepTime: 5, MaxKeepTime: 10 };
      assert.tryThrow( SDB_OPERATION_CONFLICT, function()
      {
         group1.startCriticalMode( options );
      } );

      // 指定不同组节点启动critical 模式
      var slaveNodeName3 = slaveNode3.getHostName() + ":" + slaveNode3.getServiceName();
      var options = { NodeName: slaveNodeName3, MinKeepTime: 5, MaxKeepTime: 10 };
      group2.startCriticalMode( options );

   } finally
   {
      // 恢复集群
      group1.stopMaintenanceMode();
      group2.stopCriticalMode();
   }

   commCheckBusinessStatus( db );
}