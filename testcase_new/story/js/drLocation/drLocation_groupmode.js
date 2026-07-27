/******************************************************************************
 * Verify dr_location GroupMode start/stop, idempotency, and conflicts.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_groupmode";
   var locations = [
      CHANGEDPREFIX + "_gm_primary",
      CHANGEDPREFIX + "_gm_same_city",
      CHANGEDPREFIX + "_gm_offsite"
   ];

   try
   {
      drAssertNoGroupModes();
      drCreateSixNodeLocationGroup( groupName, locations );

      var startMaintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( startMaintenance );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      var repeatedMaintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( repeatedMaintenance );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      var conflict = drRun( "sdb_start_critical.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( conflict );
      drAssertContains( conflict, "WARNING" );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      var stopMaintenance = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopMaintenance );
      drAssertNoGroupModes( groupName );

      var repeatedStop = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( repeatedStop );
      drAssertContains( repeatedStop,
         "Skip Location not in MaintenanceMode: " + locations[1] );
      drAssertNoGroupModes( groupName );

      var startCritical = drRun( "sdb_start_critical.sh",
         ["-l", locations[0]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( startCritical );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );

      var stopCritical = drRun( "sdb_stop_critical.sh",
         ["-l", locations[0], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopCritical );
      drAssertNoGroupModes( groupName );
   }
   finally
   {
      drDropReplicaGroup( groupName );
   }
}