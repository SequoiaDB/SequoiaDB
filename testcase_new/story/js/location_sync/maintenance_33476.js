/******************************************************************************
 * @Description   : seqDB-33476:多次以不同方式启动运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_33476";
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
   var slaveNodeName1 = slaveNode1.getHostName() + ":" + slaveNode1.getServiceName();
   var slaveNodeName2 = slaveNode2.getHostName() + ":" + slaveNode2.getServiceName();

   try
   {
      // option指定节点名启动运维模式
      var options = { NodeName: slaveNodeName1, MinKeepTime: 5, MaxKeepTime: 10 };
      group.startMaintenanceMode( options );

      checkGroupNodeInMaintenanceMode( db, groupName, [slaveNodeName1] );

      // option指定location启动运维模式
      var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10 };
      group.startMaintenanceMode( options );

      checkGroupNodeInMaintenanceMode( db, groupName, [slaveNodeName1, slaveNodeName2] );

   } finally
   {
      group.stopMaintenanceMode();

      // 移除group设置的location
      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }

   commCheckBusinessStatus( db );
}