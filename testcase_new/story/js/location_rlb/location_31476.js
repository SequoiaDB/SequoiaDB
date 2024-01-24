/******************************************************************************
 * @Description   : seqDB-31476 :三副本环境停节点查看IsLocationPrimary值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.09
 * @LastEditTime  : 2023.06.09
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31476";
   try
   {
      var group = commGetDataGroupNames( db )[0];
      var rg = db.getRG( group );
      var nodelist = commGetGroupNodes( db, group );
      // 清空节点location
      setLocationForNodes( rg, nodelist, "" );

      // 复制组所有节点设置location
      setLocationForNodes( rg, nodelist, location );
      var masterNode = rg.getMaster();
      var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();
      var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location, 30 );
      // 检查系统快照位置集主节点的IsLocationPrimary值
      var values1 = getSnapshotSystem( db, locationPrimary1 );
      assert.equal( values1[0]["Location"], location );
      assert.equal( values1[0]["IsLocationPrimary"], true );
      // 停一个备节点
      var slaveNodes = getGroupSlaveNodeName( db, group );
      var slaveNode1 = rg.getNode( slaveNodes[0] );
      slaveNode1.stop();
      var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location, 30 );
      // 检查系统快照位置集主节点的IsLocationPrimary值
      var values2 = getSnapshotDatabase( db, locationPrimary2 );
      assert.equal( values2[0]["Location"], location );
      assert.equal( values2[0]["IsLocationPrimary"], true );

      // 停另一个备节点后，没有位置集主节点
      var slaveNode2 = rg.getNode( slaveNodes[1] );
      slaveNode2.stop();
      var isLocationPrimary = waitGetSnapshot( db, masterNodeName, 60 );
      assert.equal( isLocationPrimary, false );

      assert.tryThrow( SDB_CLS_NOT_LOCATION_PRIMARY, function()
      {
         rg.reelectLocation( location );
      } );

      slaveNode1.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );

      var locationPrimary = checkAndGetLocationHasPrimary( db, group, location, 60 );
      var values3 = getSnapshotDatabase( db, locationPrimary );
      assert.equal( values3[0]["Location"], location );
      assert.equal( values3[0]["IsLocationPrimary"], true );

      var primaryNode = getPrimaryNode( db, group );
      assert.equal( values3[0]["NodeID"], primaryNode );
   }
   finally
   {
      slaveNode1.start();
      slaveNode2.start();
      commCheckBusinessStatus( db );
      // 清空节点location
      setLocationForNodes( rg, nodelist, "" );
   }
}
function waitGetSnapshot ( db, nodeName, timeout )
{
   var doTime = 0;
   while( doTime < timeout )
   {
      var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, NodeName: nodeName } );
      var isLocationPrimary = cursor.current().toObj().IsLocationPrimary;
      if( isLocationPrimary == false )
      {
         break;
      }
      cursor.close();
      doTime++;
      sleep( 1000 );
   }
   if( doTime >= timeout )
   {
      throw new Error( "Waiting timeout" );
   }
   return isLocationPrimary;
}