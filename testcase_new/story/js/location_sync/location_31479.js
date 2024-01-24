/******************************************************************************
 * @Description   : seqDB-31479: 使用reelectLocation()不指定option参数重新选举位置集主节点
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
   var location = "location_31479";

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

   // 位置集重选举不指定option参数
   rg.reelectLocation( location );
   checkAndGetLocationHasPrimary( db, group, location, 10 );

   // 位置集主节点改变
   var primaryNode = getPrimaryNode( db, group );
   assert.notEqual( values1[2], primaryNode );

   setLocationForNodes( rg, nodelist, "" );
}