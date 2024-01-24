/******************************************************************************
 * @Description   : seqDB-31833:coord复制组设置ActiveLocation
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
   var location1 = "location_31833_1";
   var coordGroupName = COORD_GROUPNAME;

   // 获取CoordGroup中的节点
   var coordNodes = getGroupSlaveNodeName( db, coordGroupName );

   // 给一个节点设置location
   var rg = db.getRG( coordGroupName );
   var slaveNode = rg.getNode( coordNodes[0] );
   slaveNode.setLocation( location1 );

   // 设置ActiveLocation
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( location1 );
   } );

   // 移除group设置的location
   slaveNode.setLocation( "" );

}