/******************************************************************************
 * Verify current MaintenanceMode stop --check behavior around node faults.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_maintenance_check";
   var locations = [
      CHANGEDPREFIX + "_maintenance_primary",
      CHANGEDPREFIX + "_maintenance_same_city",
      CHANGEDPREFIX + "_maintenance_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      var nodes = drCreateSixNodeLocationGroup( groupName, locations );
      var rg = db.getRG( groupName );

      var start = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( start );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      // Keep one node of the target Location online so MaintenanceMode remains
      // visible; stop_maintenance --check validates faults in the target only.
      rg.getNode( nodes[2].hostname, nodes[2].svcname ).stop();
      drWaitNodeReportedError(
         nodes[2].hostname + ":" + nodes[2].svcname, 60 );

      var targetFaultStop = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
      drAssertFailure( targetFaultStop );
      drAssertContains( targetFaultStop,
         "Some nodes are not ok, cannot not stop MaintenanceMode" );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      rg.start();
      commCheckBusinessStatus( db, 120, true );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      var healthyStop = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( healthyStop );
      // Must actually stop, not idempotent-skip while mode remains.
      drAssertContains( healthyStop, "Stopping MaintenanceMode for Location" );
      drAssertNotContains( healthyStop, "Skip Location not in MaintenanceMode" );
      drAssertNoGroupModes( groupName );

      testFaultOutsideTarget( rg, nodes, groupName, locations );
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

function testFaultOutsideTarget ( rg, nodes, groupName, locations )
{
   var targetHosts = {};
   targetHosts[nodes[2].hostname] = true;
   targetHosts[nodes[3].hostname] = true;

   var outsideNode = null;
   for( var i = 4; i <= 5; ++i )
   {
      if( !targetHosts[nodes[i].hostname] )
      {
         outsideNode = nodes[i];
         break;
      }
   }
   if( outsideNode === null )
   {
      println( "No physical Host outside the target Location; " +
         "skip outside-fault subcase" );
      return;
   }

   var start = drRun( "sdb_start_maintenance.sh",
      ["-l", locations[1]], DR_OPERATION_TIMEOUT );
   drAssertSuccess( start );
   drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

   rg.getNode( outsideNode.hostname, outsideNode.svcname ).stop();
   drWaitNodeReportedError(
      outsideNode.hostname + ":" + outsideNode.svcname, 60 );

   // Outside-target faults must not block stop_maintenance --check.
   var outsideFaultStop = drRun( "sdb_stop_maintenance.sh",
      ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
   drAssertSuccess( outsideFaultStop );
   drAssertContains( outsideFaultStop, "Stopping MaintenanceMode for Location" );
   drAssertNoGroupModes( groupName );

   rg.start();
   commCheckBusinessStatus( db, 120, true );
}
