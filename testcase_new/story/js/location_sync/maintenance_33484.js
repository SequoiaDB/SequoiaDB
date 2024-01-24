/******************************************************************************
 * @Description   : seqDB-33484:rg.stopMaintenanceMode接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_33484";
   var groupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, groupName );

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
      // option指定有效值
      var options = { NodeName: slaveNodeName };
      group.stopMaintenanceMode( options );

      var options = { Location: location };
      group.stopMaintenanceMode( options );

      // option指定无效值
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         group.stopMaintenanceMode( slaveNodeName );
      } );

      // NodeName指定无效值
      group.stopMaintenanceMode( { NodeName: "node1" } );

      group.stopMaintenanceMode( { NodeName: 123 } );

      // Location指定无效值
      group.stopMaintenanceMode( { Location: "location1" } );

      group.stopMaintenanceMode( { Location: 123 } );

   } finally
   {
      // 移除group设置的location
      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }

   commCheckBusinessStatus( db );
}
