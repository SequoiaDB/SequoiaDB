/******************************************************************************
 * @Description   : seqDB-31477: 三副本环境修改主节点的location
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.06.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31477_1";
   var location2 = "location_31477_2";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );

   // 将复制组1中的所有节点设置location1
   setLocationForNodes( rg, nodelist, location1 );

   assert.tryThrow( SDB_CLS_NOT_LOCATION_PRIMARY, function()
   {
      rg.reelectLocation( location1 );
   } );
   // 将主节点的location设置为location2
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();
   masterNode.setLocation( location2 );

   var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location1, 10 );
   var values1 = getSnapshotSystem( db, locationPrimary1 );
   assert.equal( values1[0]["Location"], location1 );
   assert.equal( values1[0]["IsLocationPrimary"], true );
   var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location2, 10 );
   var values2 = getSnapshotDatabase( db, locationPrimary2 );
   assert.equal( values2[0]["Location"], location2 );
   assert.equal( values2[0]["IsLocationPrimary"], true );

   // 主节点设置location为""
   masterNode.setLocation( "" );

   // 备节点的位置集主节点IsLocationPrimary为true
   var locationPrimary3 = checkAndGetLocationHasPrimary( db, group, location1, 10 );
   var values3 = getSnapshotDatabase( db, locationPrimary3 );
   assert.equal( values3[0]["Location"], location1 );
   assert.equal( values3[0]["IsLocationPrimary"], true );

   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, GroupName: group, NodeName: masterNodeName } );
   while( cursor.next() )
   {
      var isLocationPrimary = cursor.current().toObj().IsLocationPrimary;
   }
   assert.equal( isLocationPrimary, false );
   cursor.close();

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );
}