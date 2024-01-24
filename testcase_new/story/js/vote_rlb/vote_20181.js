/* *****************************************************************************
@discretion: seqDB-20181:指定其他组的备节点ID/本组故障的备节点ID，重新选主
@author：2018-11-04 zhao xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length < 2 )
   {
      return;
   }
   var group1 = groups[0];
   var group2 = groups[1]
   var groupName1 = group1[0].GroupName;
   var groupName2 = group2[0].GroupName;
   var primaryPos1 = group1[0].PrimaryPos;
   var primaryPos2 = group2[0].PrimaryPos;
   var slaveNode1Pos = primaryPos1 === 1 ? primaryPos1 + 1 : 1;
   var slaveNode2Pos = primaryPos1 === group1.length - 1 ? primaryPos1 - 1 : group1.length - 1;
   var slaveNode1 = group1[slaveNode1Pos];
   var slaveNode2 = group1[slaveNode2Pos];
   var masterNode = group1[primaryPos1];
   var group2SlaveNodePos = primaryPos2 === 1 ? primaryPos2 + 1 : 1;
   var group2SlaveNode = group2[group2SlaveNodePos];

   waitSync( masterNode, slaveNode1 );
   waitSync( masterNode, slaveNode2 );

   //指定其他组的备节点ID，重新选主
   var nodeID = group2SlaveNode.NodeID;
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      db.getRG( groupName1 ).reelect( { Seconds: 60, NodeID: parseInt( nodeID ) } );
   } );

   //指定本组故障的备节点ID，重新选主
   db.getRG( groupName1 ).getNode( slaveNode1.HostName, slaveNode1.svcname ).stop();
   nodeID = slaveNode1.NodeID;
   try
   {
      db.getRG( groupName1 ).reelect( { Seconds: 60, NodeID: parseInt( nodeID ) } );
      var node = db.getRG( groupName1 ).getMaster();
      var nodeName = node.getHostName() + ":" + node.getServiceName();
      if( nodeName !== slaveNode2.HostName + ":" + slaveNode2.svcname )
      {
         throw new Error( "\nnodeName: " + nodeName + "\nsalveNode2: " + slaveNode2.HostName + ":" + slaveNode2.svcname );
      }
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         throw new Error( e );
      }
   }
   db.getRG( groupName1 ).getNode( slaveNode1.HostName, slaveNode1.svcname ).start();
   sleep( 14000 );
   commCheckBusinessStatus( db );

   //指定主机名和服务名不存在，重新选主
   var hostName = group2SlaveNode.HostName;
   var svcName = group2SlaveNode.svcname;
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      db.getRG( groupName1 ).reelect( { Seconds: 60, HostName: hostName, ServiceName: svcName } );
   } );
}
