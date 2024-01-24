/* *****************************************************************************
@discretion:  seqDB-20174:单独指定HostName、ServiceName，重新选主
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
   var primaryNode = group[primaryPos];

   waitSync( primaryNode, slaveNode1 );

   //指定HostName执行选主
   var hostName = slaveNode1.HostName;
   db.getRG( groupName ).reelect( { Seconds: 60, HostName: hostName } );

   var masterNodeHostName = db.getRG( groupName ).getMaster().getHostName();
   if( masterNodeHostName != hostName )
   {
      throw new Error( "Expected master node's hostName is " + hostName + ", but actual is " + masterNodeHostName );
   }

   waitSync( slaveNode1, slaveNode2 );

   //指定ServiceName执行选主
   svcName = slaveNode2.svcname;
   db.getRG( groupName ).reelect( { Seconds: 60, ServiceName: svcName } );

   var masterNodeSvcName = db.getRG( groupName ).getMaster().getServiceName();
   if( masterNodeSvcName != svcName )
   {
      throw new Error( "Reelect failed!" );
   }
}

