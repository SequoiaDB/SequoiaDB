/******************************************************************************
@Description : Test stop majority node in data group and then start it.
@Modify list : 2014-6-12  xiaojun Hu  Init
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
   var nodeIndexes = getMajorityNodeIndexes( group );

   try
   {
      for( var i = 0; i < nodeIndexes.length; i++ )
      {
         var svcName = group[nodeIndexes[i]].svcname;
         var hostName = group[nodeIndexes[i]].HostName;
         db.getRG( groupName ).getNode( hostName, svcName ).stop();
      }

      notExistPrimaryNode( groupName );
   }
   finally
   {
      db.getRG( groupName ).start();
      sleep( 14000 );
      commCheckBusinessStatus( db );
   }
}
