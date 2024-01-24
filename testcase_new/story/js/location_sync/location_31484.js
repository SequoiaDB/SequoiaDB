/******************************************************************************
 * @Description   : seqDB-31484:使用reelectLocation()同时指定NodeID、hostName、serviceName重新选举位置集主节点
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.06.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31484";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );
   var nodeNames = getGroupSlaveNodeName( db, group );

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );

   // 将复制组1中的所有节点设置location
   setLocationForNodes( rg, nodelist, location );
   // 将主节点的location设置为""
   var masterNode = rg.getMaster();
   masterNode.setLocation( "" );
   checkAndGetLocationHasPrimary( db, group, location, 10 );

   // 指定存在的NodeID、hostNmae、svcName
   var nodeId1 = getNodeId( db, nodeNames[0] );
   rg.reelectLocation( location, { NodeID: nodeId1, HostName: nodelist[0]["HostName"], ServiceName: nodelist[0]["svcname"] } );
   var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values1 = getSnapshotSystem( db, locationPrimary1 );
   assert.equal( values1[0]["Location"], location );
   assert.equal( values1[0]["IsLocationPrimary"], true );
   assert.equal( values1[0]["NodeID"], nodeId1 );
   var primaryNode = getPrimaryNode( db, group );
   assert.equal( values1[0]["NodeID"], primaryNode );

   // 指定存在的NodeID、不存在的hostNmae
   var nodeId2 = getNodeId( db, nodeNames[1] );
   rg.reelectLocation( location, { NodeID: nodeId2, HostName: "nodehostname-31484" } );
   var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values2 = getSnapshotDatabase( db, locationPrimary2 );
   assert.equal( values2[0]["Location"], location );
   assert.equal( values2[0]["IsLocationPrimary"], true );
   assert.equal( values2[0]["NodeID"], nodeId2 );
   var primaryNode = getPrimaryNode( db, group );
   assert.equal( values2[0]["NodeID"], primaryNode );

   // 指定存在的NodeID、hostNmae，不存在的svcName
   var nodeId3 = getNodeId( db, nodeNames[0] );
   rg.reelectLocation( location, { NodeID: nodeId3, HostName: nodelist[0]["HostName"], ServiceName: "20000" } );
   var locationPrimary3 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values3 = getSnapshotSystem( db, locationPrimary3 );
   assert.equal( values3[0]["Location"], location );
   assert.equal( values3[0]["IsLocationPrimary"], true );
   assert.equal( values3[0]["NodeID"], nodeId3 );
   var primaryNode = getPrimaryNode( db, group );
   assert.equal( values3[0]["NodeID"], primaryNode );

   // 指定不存在的NodeID，存在的hostNmae、svcName
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      rg.reelectLocation( location, { NodeID: 20000, HostName: nodelist[1]["HostName"], ServiceName: nodelist[1]["svcname"] } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}