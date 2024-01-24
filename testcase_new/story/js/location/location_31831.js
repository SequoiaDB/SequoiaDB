/******************************************************************************
 * @Description   : seqDB-31831:通过复制组修改ActiveLocation
 * @Author        : liuli
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31831_1";
   var location2 = "location_31831_2";
   var location3 = "location_31831_3";
   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );

   // 给一个主节点和一个备节点设置相同的location
   var rg = db.getRG( dataGroupName );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );

   // 另一个备节点设置不同的location
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode2.setLocation( location2 );

   // 设置ActiveLocation为location1
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // 修改ActiveLocation为location1
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // 修改ActiveLocation为location2
   rg.setActiveLocation( location2 );
   checkGroupActiveLocation( db, dataGroupName, location2 );

   // 修改ActiveLocation为一个不存在的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( location3 );
   } );
   checkGroupActiveLocation( db, dataGroupName, location2 );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );

   // 获取catalog group 中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, CATALOG_GROUPNAME );

   // 给一个主节点和一个备节点设置相同的location
   var rg = db.getCataRG();
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );

   // 另一个备节点设置不同的location
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode2.setLocation( location2 );

   // 设置ActiveLocation为location1
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location1 );

   // 修改ActiveLocation为location1
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location1 );

   // 修改ActiveLocation为location2
   rg.setActiveLocation( location2 );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location2 );

   // 修改ActiveLocation为一个不存在的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( location3 );
   } );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location2 );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}