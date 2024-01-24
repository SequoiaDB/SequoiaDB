/******************************************************************************
 * @Description   : seqDB-31814:集群正常catalog启动Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.05.30
 * @LastEditTime  : 2023.05.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31814_1";
   var location2 = "location_31814_2";

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, CATALOG_GROUPNAME );

   // 2个备节点设置相同的location，主节点设置不同的location
   var rg = db.getCataRG();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location1 );
   var masterNode = rg.getMaster();
   masterNode.setLocation( location2 );
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 指定一个catalog备节点启动Critical模式
   var minKeepTime = 1;
   var maxKeepTime = 20;
   var options = { NodeName: slaveNodes[0], MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      rg.startCriticalMode( options );
   } );

   // 指定Location启动Critical模式，Location中全部为备节点
   var options = { Location: location1, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      rg.startCriticalMode( options );
   } );

   // 指定Location强制启动Critical模式，Location中全部为备节点
   var options = { Location: location1, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime, Enforced: true };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      rg.startCriticalMode( options );
   } );

   // 指定主节点启动Critical模式
   var options = { NodeName: masterNodeName, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
   rg.startCriticalMode( options );

   // 校验Critical模式启动成功
   checkStartCriticalMode( db, CATALOG_GROUPNAME, options );

   // 指定Location启动Critical模式，Location中有主节点
   var options = { Location: location2, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
   rg.startCriticalMode( options );

   // 校验Critical模式启动成功
   checkStartCriticalMode( db, CATALOG_GROUPNAME, options );

   // 停止Critical模式
   rg.stopCriticalMode();

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}