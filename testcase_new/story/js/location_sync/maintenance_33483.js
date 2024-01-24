/******************************************************************************
 * @Description   : seqDB-33483:rg.startMaintenanceMode接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_33483";
   var groupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, groupName );

   // 给一个主节点和一个备节点设置相同的location,另一个备节点设置不同的location
   var group = db.getRG( groupName );
   var masterNode = group.getMaster();
   var slaveNode1 = group.getNode( slaveNodes[0] );
   var slaveNode2 = group.getNode( slaveNodes[1] );
   masterNode.setLocation( location );
   slaveNode1.setLocation( location );
   slaveNode2.setLocation( location );

   // 获取一个备节点名
   var slaveNodeName = slaveNode1.getHostName() + ":" + slaveNode1.getServiceName();

   try
   {
      // option指定有效值启动运维模式
      var options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 2 };
      group.startMaintenanceMode( options );
      group.stopMaintenanceMode();

      // option指定无效值启动运维模式
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( slaveNodeName );
      } );

      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         group.startMaintenanceMode( {} );
      } );

      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         group.startMaintenanceMode( { MinKeepTime: 1, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 1 } );
      } );

      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MaxKeepTime: 2 } );
      } );

      // NodeName指定无效值启动运维模式
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: 123, MinKeepTime: 1, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: "test", MinKeepTime: 1, MaxKeepTime: 2 } );
      } );

      // Location指定有效值启动运维模式
      var options = { Location: location, MinKeepTime: 1, MaxKeepTime: 2 };
      group.startMaintenanceMode( options );
      group.stopMaintenanceMode();

      // Location指定无效值启动运维模式
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { Location: 123, MinKeepTime: 1, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { Location: "test", MinKeepTime: 1, MaxKeepTime: 2 } );
      } );

      // MinKeepTime指定有效值启动运维模式
      var options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 1 };
      group.startMaintenanceMode( options );
      group.stopMaintenanceMode();

      var options = { NodeName: slaveNodeName, MinKeepTime: 180, MaxKeepTime: 360 };
      group.startMaintenanceMode( options );
      group.stopMaintenanceMode();

      var options = { NodeName: slaveNodeName, MinKeepTime: 10080, MaxKeepTime: 10080 };
      group.startMaintenanceMode( options );
      group.stopMaintenanceMode();

      // MinKeepTime指定无效值启动运维模式
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 0, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: -1, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 10081, MaxKeepTime: 2 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: "test", MaxKeepTime: 2 } );
      } );

      // MaxKeepTime指定无效值启动运维模式
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 0 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: -1 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 10081 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 360, MaxKeepTime: 180 } );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.startMaintenanceMode( { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: "test" } );
      } );

   } finally
   {
      // 启动节点，恢复集群
      group.start();
      group.stopMaintenanceMode();

      // 移除group设置的location
      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }

   commCheckBusinessStatus( db );
}