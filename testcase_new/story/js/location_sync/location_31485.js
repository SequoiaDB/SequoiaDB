/******************************************************************************
 * @Description   : seqDB-31485: 使用reelectLocation()同时指定hostName、serviceName重选举位置集主节点
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.06.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31485";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );
   // 将备节点设置location
   var slaveNodes = getGroupSlaveNodeName( db, group );
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode1.setLocation( location );
   slaveNode2.setLocation( location );

   var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values1 = getSnapshotSystem( db, locationPrimary1 );
   assert.equal( values1[0]["Location"], location );
   assert.equal( values1[0]["IsLocationPrimary"], true );
   var node = slaveNode1.getHostName() + ":" + slaveNode1.getServiceName();
   // 指定有效的hostName,svcName
   if( locationPrimary1 == node )
   {
      validValue( db, rg, group, location, slaveNode2 );
   } else
   {
      validValue( db, rg, group, location, slaveNode1 );
   }
   // 指定无效的hostName，有效的svcName报错
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      rg.reelectLocation( location, { HostName: "nodehostname-31485", ServiceName: group[2].svcname } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}

function validValue ( db, rg, group, location, slaveNode )
{
   var hostName = slaveNode.getHostName();
   var svcName = slaveNode.getServiceName();
   rg.reelectLocation( location, { HostName: hostName, ServiceName: svcName } );
   checkAndGetLocationHasPrimary( db, group, location, 10 );
   var nodeName = hostName + ":" + svcName;
   var values2 = getSnapshotDatabase( db, nodeName );
   assert.equal( values2[0]["Location"], location );
   assert.equal( values2[0]["IsLocationPrimary"], true );
}