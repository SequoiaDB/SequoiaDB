/******************************************************************************
 * @Description   : seqDB-31830:通过复制组设置ActiveLocation
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.05.24
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31830_1";
   var location2 = "location_31830_2";
   var location3 = "location_31830_3";
   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );

   // 给一个主节点和一个备节点设置相同的location,另一个备节点设置不同的location
   var rg = db.getRG( dataGroupName );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );

   // 设置ActiveLocation为一个已存在的location
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // 设置ActiveLocation为一个不存在的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( location3 );
   } );
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );

   // 获取catalog group 中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, CATALOG_GROUPNAME );

   // 给一个主节点和一个备节点设置相同的location,另一个备节点设置不同的location
   var rg = db.getCataRG();
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );

   // 设置ActiveLocation为为一个已存在的location
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location1 );

   // 设置ActiveLocation为一个不存在的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( location3 );
   } );
   checkGroupActiveLocation( db, CATALOG_GROUPNAME, location1 );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}