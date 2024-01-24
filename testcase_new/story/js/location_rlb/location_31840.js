/******************************************************************************
 * @Description   : 复制组设置好Location与ActiveLocation后，节点修改Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_31840_1";
   var location2 = "location_31840_2";
   var dataGroupName = commGetDataGroupNames( db )[0];
   var port1 = parseInt( RSRVPORTBEGIN ) + 10;
   var port2 = parseInt( RSRVPORTBEGIN ) + 20;
   var dbpath1 = RSRVNODEDIR + "data/" + port1;
   var dbpath2 = RSRVNODEDIR + "data/" + port2;

   try
   {
      var rg = db.getRG( dataGroupName );
      var hostName = rg.getMaster().getHostName();
      var node1 = rg.createNode( hostName, port1, dbpath1, { diaglevel: 5 } );
      var node2 = rg.createNode( hostName, port2, dbpath2, { diaglevel: 5 } );
      rg.start();

      node1.setLocation( location1 );
      node2.setLocation( location2 );

      rg.setActiveLocation( location1 );
      checkGroupActiveLocation( db, dataGroupName, location1 );

      // 修改location值为ActiveLocation的值,ActiveLocation不变
      node2.setLocation( location1 );
      checkGroupActiveLocation( db, dataGroupName, location1 );

      // 修改ActiveLocation所有节点的location
      node1.setLocation( location2 );
      node2.setLocation( location2 );
      checkGroupActiveLocation( db, dataGroupName, undefined );

      // 删除ActiveLocation所有节点的location
      node1.setLocation( location1 );
      node2.setLocation( location1 );
      rg.setActiveLocation( location1 );

      node1.setLocation( "" );
      node2.setLocation( "" );
      checkGroupActiveLocation( db, dataGroupName, undefined );

      node1.setLocation( location1 );
      node2.setLocation( location1 );
      rg.setActiveLocation( location1 );

      rg.removeNode( hostName, port1 );
      rg.removeNode( hostName, port2 );
      checkGroupActiveLocation( db, dataGroupName, undefined );
   }
   finally
   {
      removeNode( rg, hostName, port1 );
      removeNode( rg, hostName, port2 );
   }
}