/******************************************************************************
 * @Description   : seqDB-31480: 使用reelectLocation()指定错误的Location参数
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.06.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31480";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );

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

   // 设置错误的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( "Failed_location_31480" );
   } );

   var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location, 10 );
   var values2 = getSnapshotDatabase( db, locationPrimary2 );
   assert.equal( values2[0]["Location"], location );
   assert.equal( values2[0]["NodeID"], values1[0]["NodeID"] );

   var primaryNode = getPrimaryNode( db, group );
   assert.equal( values2[0]["NodeID"], primaryNode );

   setLocationForNodes( rg, nodelist, "" );
}