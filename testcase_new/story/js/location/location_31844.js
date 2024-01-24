/******************************************************************************
 * @Description   : seqDB-31844:SdbReplicaGroup.setActiveLocation(<location>)参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.23
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31844_1";
   var location2 = "location_31844_2";
   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );

   // 给节点设置location
   var rg = db.getRG( dataGroupName );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );


   // 设置ActiveLocation为参数为空
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      rg.setActiveLocation();
   } );

   // 设置ActiveLocation为参数为非字符串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.setActiveLocation( 100 );
   } );

   // 设置ActiveLocation为参数为超长字符串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var test = new Array( 257 );
      var longStr = test.join( "a" );
      rg.setActiveLocation( longStr );
   } );

   // 修改ActiveLocation为location1
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}