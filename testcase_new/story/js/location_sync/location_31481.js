/******************************************************************************
 * @Description   : seqDB-31481:使用reelectLocation()指定NodeID重新选举位置集主节点
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
   var location = "location_31481";

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
   var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values1 = getSnapshotSystem( db, locationPrimary1 );
   assert.equal( values1[0]["Location"], location );
   assert.equal( values1[0]["IsLocationPrimary"], true );

   // 设置NodeID为位置集主节点NodeID，位置集主节点ID不变
   rg.reelectLocation( location, { NodeID: values1[0]["NodeID"] } );
   var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values2 = getSnapshotSystem( db, locationPrimary2 );
   assert.equal( values2[0]["IsLocationPrimary"], true );
   assert.equal( values2[0]["NodeID"], values1[0]["NodeID"] );

   // 30s内选举成功,位置集主节点NodeID为指定的ID
   var data = new Date();
   var seconds1 = data.getSeconds();

   var nodeId1 = getNodeId( db, nodeNames[0] );
   rg.reelectLocation( location, { NodeID: nodeId1 } );
   var data = new Date();
   var seconds2 = data.getSeconds();
   if( ( seconds2 - seconds1 ) > 30 )
   {
      throw new Error( "Seconds默认值超过30s！" + ",实际为:" + ( seconds2 - seconds1 ) + "秒" );
   }
   var locationPrimary3 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values3 = getSnapshotDatabase( db, locationPrimary3 );
   assert.equal( values3[0]["Location"], location );
   assert.equal( values3[0]["IsLocationPrimary"], true );
   assert.equal( values3[0]["NodeID"], nodeId1 );

   // 60s内选举成功,位置集主节点NodeID为指定的ID
   var data = new Date();
   var seconds1 = data.getSeconds();
   var nodeId2 = getNodeId( db, nodeNames[1] );
   rg.reelectLocation( location, { NodeID: nodeId2, Seconds: 60 } );
   var data = new Date();
   var seconds2 = data.getSeconds();
   if( ( seconds2 - seconds1 ) > 60 )
   {
      throw new Error( "Seconds默认值超过60s！" + ",实际为:" + ( seconds2 - seconds1 ) + "秒" );
   }
   var locationPrimary4 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values4 = getSnapshotDatabase( db, locationPrimary4 );
   assert.equal( values4[0]["Location"], location );
   assert.equal( values4[0]["IsLocationPrimary"], true );
   assert.equal( values4[0]["NodeID"], nodeId2 );

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );
}