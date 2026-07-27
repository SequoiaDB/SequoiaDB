/******************************************************************************
 * Verify stop --check keeps CriticalMode while any cluster node is abnormal.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_stop_check";
   var locations = [
      CHANGEDPREFIX + "_check_primary",
      CHANGEDPREFIX + "_check_same_city",
      CHANGEDPREFIX + "_check_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      var nodes = drCreateSixNodeLocationGroup( groupName, locations );
      var rg = db.getRG( groupName );

      var start = drRun( "sdb_start_critical.sh",
         ["-l", locations[0]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( start );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );

      var faultNodeInfo = nodes[4];
      var faultNode = rg.getNode( faultNodeInfo.hostname,
         faultNodeInfo.svcname );
      var faultNodeName = faultNodeInfo.hostname + ":" +
         faultNodeInfo.svcname;
      faultNode.stop();
      drWaitNodeReportedError( faultNodeName, 60 );

      var protectedStop = drRun( "sdb_stop_critical.sh",
         ["-l", locations[0], "--check"], DR_OPERATION_TIMEOUT );
      drAssertFailure( protectedStop );
      // Tool message uses lowercase "ok" and "cannot not stop CriticalMode".
      drAssertContains( protectedStop, "Some nodes are not ok" );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );

      faultNode.start();
      commCheckBusinessStatus( db, 120, true );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );

      var healthyStop = drRun( "sdb_stop_critical.sh",
         ["-l", locations[0], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( healthyStop );
      drAssertNoGroupModes( groupName );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      try
      {
         if( drReplicaGroupExists( groupName ) )
            db.getRG( groupName ).start();
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drStopGroupModes( groupName );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         commCheckBusinessStatus( db, 120, true );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drDropReplicaGroup( groupName );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
   }

   if( firstError !== null ) throw firstError;
}
