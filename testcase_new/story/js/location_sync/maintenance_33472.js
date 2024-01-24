/******************************************************************************
 * @Description   : seqDB-33472:group启动运维模式，同时指定NodeName和Location
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_33472";
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

   try
   {
      // option同时指定节点名与location启动运维模式
      var options = { Location: location, NodeName: slaveNodes[0], MinKeepTime: 5, MaxKeepTime: 10 };
      group.startMaintenanceMode( options );

      checkGroupNodeInMaintenanceMode( db, groupName, [slaveNodes[0]] );

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