/******************************************************************************
 * @Description   : seqDB-33474:已启动Critical模式的复制组启动运维模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipGroupLessThanThree = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var location = "location_33474";
   var groupName = commGetDataGroupNames( db )[0];
   var group = db.getRG( groupName );
   // group1指定主节点和一个备节点设置Location后启动Critical模式
   var slaveNodeNames = getGroupSlaveNodeName( db, groupName );
   var masterNodeName = getGroupMasterNodeName( db, groupName )[0];
   var slaveNode = group.getNode( slaveNodeNames[0] );
   slaveNode.setLocation( location );
   var masterNode = group.getNode( masterNodeName )
   masterNode.setLocation( location );
   var options = { Location: location, MinKeepTime: 10, MaxKeepTime: 20 };
   group.startCriticalMode( options );

   checkStartCriticalMode( db, groupName, options );

   // 指定已经启动Critical模式的备节点启动运维模式
   options = { NodeName: slaveNodeNames[0], MinKeepTime: 10, MaxKeepTime: 20 };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      group.startMaintenanceMode( options );
   } )

   // 指定没有启动Critical模式的备节点启动运维模式
   options = { NodeName: slaveNodeNames[1], MinKeepTime: 10, MaxKeepTime: 20 };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      group.startMaintenanceMode( options );
   } )

   // 指定group2中的一个备节点启动运维模式
   var groupName2 = commGetDataGroupNames( db )[1];
   var group2 = db.getRG( groupName2 );
   var slaveNodeNames2 = getGroupSlaveNodeName( db, groupName2 );
   options = { NodeName: slaveNodeNames2[0], MinKeepTime: 10, MaxKeepTime: 20 };
   group2.startMaintenanceMode( options );

   // 启动运维模式成功
   var mode = "maintenance";
   checkGroupNodeNameMode( db, groupName2, slaveNodeNames2[0], mode );

   group.stopCriticalMode();
   group2.stopMaintenanceMode();
   slaveNode.setLocation( "" );
   masterNode.setLocation( "" );
}