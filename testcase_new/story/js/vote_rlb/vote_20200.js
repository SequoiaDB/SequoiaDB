/* *****************************************************************************
@discretion: seqDB-20200:同时指定NodeID、HostName和ServiceName，重新选主
@author：2018-11-04 zhao xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groups = getGroupsWithNodeNum( 3 );
   if( groups.length === 0 )
   {
      return;
   }
   var group = groups[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var slaveNode1Pos = primaryPos === 1 ? primaryPos + 1 : 1;
   var slaveNode2Pos = primaryPos === group.length - 1 ? primaryPos - 1 : group.length - 1;
   var slaveNode1 = group[slaveNode1Pos];
   var slaveNode2 = group[slaveNode2Pos];
   var masterNode = group[primaryPos];

   waitSync( masterNode, slaveNode1 );
   waitSync( masterNode, slaveNode2 );

   //指定NodeID、HostName和ServiceName,执行选主
   var slaveNode1HostName = slaveNode1.HostName;
   var slaveNode1SvcName = slaveNode1.svcname;
   var slaveNode2ID = slaveNode2.NodeID;
   db.getRG( groupName ).reelect( { Seconds: 60, NodeID: parseInt( slaveNode2ID ), HostName: slaveNode1HostName, ServiceName: slaveNode1SvcName } );

   masterNode = db.getRG( groupName ).getMaster();
   var masterNodeID = masterNode.getNodeDetail().split( ":" )[0];
   if( masterNodeID != slaveNode2ID )
   {
      throw new Error( "Reelect failed! expct nodeID is " + slaveNode2ID + ", act nodeID is " + masterNodeID );
   }
}
