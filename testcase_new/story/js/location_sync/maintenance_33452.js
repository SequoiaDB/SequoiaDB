/******************************************************************************
 * @Description   : seqDB-33452:主节点所在Location启动运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_33452_1";
   var location2 = "location_33452_2";
   var groupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, groupName );

   var group = db.getRG( groupName );
   var masterNode = group.getMaster();
   var slaveNode1 = group.getNode( slaveNodes[0] );
   var slaveNode2 = group.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );

   try
   {
      // option指定节点名启动运维模式
      var options = { Location: location1, MinKeepTime: 5, MaxKeepTime: 10 };
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