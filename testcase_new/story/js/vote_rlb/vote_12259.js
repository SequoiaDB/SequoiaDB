/******************************************************************************
@Description : seqDB-12259:停止组中的主节点，能选出新的主节点              
@Modify list : 2014-6-12  xiaojun Hu  Init
               2019-11-26 Zhao xiaoni Modified
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
   var primaryPos = group[0].PrimaryPos;
   var svcName = group[primaryPos].svcname;
   var hostName = group[primaryPos].HostName;
   db.getRG( groupName ).getNode( hostName, svcName ).stop();

   try
   {
      var primaryNodeID = existPrimaryNode( groupName );
      if( primaryNodeID === primaryNode )
      {
         throw new Error( "Primary node id is not changed after stopped" );
      }
   }
   finally
   {
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      sleep( 14000 );
   }
}
