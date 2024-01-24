/* *****************************************************************************
@discretion: seqDB-20099:指定NodeID，重新选主
             seqDB-20100:指定HostName和ServiceName，重新选主
@author：2018-10-26 zhao xiaoni
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

   //指定NodeID执行选主
   var nodeID = slaveNode1.NodeID;
   db.getRG( groupName ).reelect( { Seconds: 60, NodeID: parseInt( nodeID ) } );

   var hostName = slaveNode1.HostName;
   var svcName = slaveNode1.svcname;
   checkReelect( groupName, hostName, svcName );

   waitSync( slaveNode1, slaveNode2 );

   //指定HostName和ServiceName执行选主
   hostName = slaveNode2.HostName;
   svcName = slaveNode2.svcname;
   db.getRG( groupName ).reelect( { Seconds: 60, HostName: hostName, ServiceName: svcName } );

   checkReelect( groupName, hostName, svcName );
}