/******************************************************************************
 * @Description   : seqDB-31842: 集群已经设置ActiveLocation，停止主节点后重新选举
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "guangzhou.nansha_31842";
   var location2 = "guangzhou_31842";
   var location3 = "guangzhou.panyu_31842";
   var port1 = parseInt( RSRVPORTBEGIN ) + 10;
   var port2 = parseInt( RSRVPORTBEGIN ) + 20;
   var port3 = parseInt( RSRVPORTBEGIN ) + 30;
   var port4 = parseInt( RSRVPORTBEGIN ) + 40;
   var dbpath1 = RSRVNODEDIR + "data/" + port1;
   var dbpath2 = RSRVNODEDIR + "data/" + port2;
   var dbpath3 = RSRVNODEDIR + "data/" + port3;
   var dbpath4 = RSRVNODEDIR + "data/" + port4;
   try
   {
      var dataGroupName = commGetDataGroupNames( db )[0];
      var rg = db.getRG( dataGroupName );
      var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );
      var masterNode = rg.getMaster();
      var slaveNode1 = rg.getNode( slaveNodes[0] );
      var slaveNode2 = rg.getNode( slaveNodes[1] );
      db.updateConf( { "weight": 100 }, { "NodeName": masterNode.getHostName() + ":" + masterNode.getServiceName() } );
      db.updateConf( { "weight": 90 }, { "NodeName": slaveNodes[0] } );
      db.updateConf( { "weight": 80 }, { "NodeName": slaveNodes[1] } );
      masterNode.setLocation( location1 );
      slaveNode1.setLocation( location1 );
      slaveNode2.setLocation( location1 );
      var hostName = rg.getMaster().getHostName();
      var svcName = rg.getMaster().getServiceName();
      removeNode( rg, hostName, port1 );
      removeNode( rg, hostName, port2 );
      removeNode( rg, hostName, port3 );
      removeNode( rg, hostName, port4 );
      var node1 = rg.createNode( hostName, port1, dbpath1, { diaglevel: 5, "weight": 90 } );
      var node2 = rg.createNode( hostName, port2, dbpath2, { diaglevel: 5, "weight": 80 } );
      var node3 = rg.createNode( hostName, port3, dbpath3, { diaglevel: 5, "weight": 90 } );
      var node4 = rg.createNode( hostName, port4, dbpath4, { diaglevel: 5, "weight": 80 } );
      rg.start();
      commCheckBusinessStatus( db );

      node1.setLocation( location2 );
      node1.setLocation( location2 );
      node3.setLocation( location3 );
      node4.setLocation( location3 );

      rg.setActiveLocation( location1 );
      masterNode.stop();
      waitGetMaster( rg );

      var newMasterNode = rg.getMaster();
      assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(), slaveNodes[0] );

      slaveNode1.stop();
      slaveNode2.stop();
      waitGetMaster( rg );
      var newMasterNode = rg.getMaster();
      assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(), node3._nodename );

      rg.start();
      commCheckBusinessStatus( db );
      rg.reelect( { ServiceName: svcName } );

      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }
   finally
   {
      db.deleteConf( { weight: 1 } );
      removeNode( rg, hostName, port1 );
      removeNode( rg, hostName, port2 );
      removeNode( rg, hostName, port3 );
      removeNode( rg, hostName, port4 );
   }
}

function waitGetMaster ( rg )
{
   var doTime = 0;
   var timeOut = 30;
   while( doTime < timeOut )
   {
      try
      {
         rg.getMaster();
         break;
      }
      catch( e )
      {
         if( e != SDB_RTN_NO_PRIMARY_FOUND )
         {
            throw new Error( e );
         }
      }
      sleep( 1000 );
      doTime++;
   }
   if( doTime >= timeOut )
   {
      throw new Error( "get master node time out" )
   }
}