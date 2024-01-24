/******************************************************************************
@Description : seqDB-12257:停止组中的少数节点，组中仍旧有主
@Modify list : 2014-6-12  xiaojun Hu  Init
               2019-11-26  Zhao xiaoni Modified
******************************************************************************/
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
   var primaryNode = group[0].PrimaryNode;
   var nodeIndexes = getMinorityNodeIndexes( group );

   try
   {
      for( var i = 0; i < nodeIndexes.length; i++ )
      {
         var svcName = group[nodeIndexes[i]].svcname;
         var hostName = group[nodeIndexes[i]].HostName;
         var nodeID = group[nodeIndexes[i]].NodeID;
         if( nodeID === primaryNode )
         {
            var isContainPrimaryNode = true;
         }
         db.getRG( groupName ).getNode( hostName, svcName ).stop();
      }

      var primaryNodeID = existPrimaryNode( groupName );
      if( isContainPrimaryNode && primaryNodeID === primaryNode )
      {
         throw new Error( "Primary node id is " + primaryNodeID + "after stop the primary node" );
      }
      else if( !isContainPrimaryNode && primaryNodeID !== primaryNode )
      {
         throw new Error( "Primary node id changed from " + primaryNode + " to " + primaryNodeID );
      }
   }
   finally
   {
      db.getRG( groupName ).start();
      sleep( 14000 );
      commCheckBusinessStatus( db );
   }
}

