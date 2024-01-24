/******************************************************************************
 * @Description   : seqDB-31478: 复制组主节点与位置集主节点不是同一节点设置location后查看IsLocationPrimary值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.06.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_31478";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );
   var masterNode = rg.getMaster();
   var hostName = masterNode.getHostName();
   var port1 = parseInt( RSRVPORTBEGIN ) + 10;
   var port2 = parseInt( RSRVPORTBEGIN ) + 20;
   var port3 = parseInt( RSRVPORTBEGIN ) + 30;
   var dbpath1 = RSRVNODEDIR + "data/" + port1;
   var dbpath2 = RSRVNODEDIR + "data/" + port2;
   var dbpath3 = RSRVNODEDIR + "data/" + port3;

   try
   {
      rg.createNode( hostName, port1, dbpath1, { diaglevel: 5 } );
      rg.createNode( hostName, port2, dbpath2, { diaglevel: 5 } );
      rg.createNode( hostName, port3, dbpath3, { diaglevel: 5 } );
      rg.start();
      commCheckBusinessStatus( db, 300 );

      //清空节点location
      setLocationForNodes( rg, nodelist, "" );

      // 设置location
      setLocationForNodes( rg, nodelist, location );
      // 将主节点的location设置为""
      masterNode.setLocation( "" );

      var locationPrimary1 = checkAndGetLocationHasPrimary( db, group, location, 30 );
      var values1 = getSnapshotDatabase( db, locationPrimary1 );
      assert.equal( values1[0]["Location"], location );
      assert.equal( values1[0]["IsLocationPrimary"], true );

      var values2 = getSnapshotSystem( db, locationPrimary1 );
      assert.equal( values2[0]["Location"], location );
      assert.equal( values2[0]["IsLocationPrimary"], true );

      // 移除一个备节点
      rg.removeNode( hostName, port1 );
      checkAndGetLocationHasPrimary( db, group, location, 30 );
      assert.equal( values2[0]["IsLocationPrimary"], true );

      // 移除另一个节点
      rg.removeNode( hostName, port2 );
      var locationPrimary2 = checkAndGetLocationHasPrimary( db, group, location, 30 );
      var values3 = getSnapshotSystem( db, locationPrimary2 );
      assert.equal( values3[0]["Location"], location );
      assert.equal( values3[0]["IsLocationPrimary"], true );

      // 将主节点设置location
      masterNode.setLocation( location );
      var locationPrimary3 = checkAndGetLocationHasPrimary( db, group, location, 30 );
      var values4 = getSnapshotDatabase( db, locationPrimary3 );
      assert.equal( values4[0]["Location"], location );
      assert.equal( values4[0]["IsLocationPrimary"], true );

      rg.removeNode( hostName, port3 );
   }
   finally
   {
      removeNode( rg, hostName, port1 );
      removeNode( rg, hostName, port2 );
      removeNode( rg, hostName, port3 );
      setLocationForNodes( rg, nodelist, "" );
   }
}