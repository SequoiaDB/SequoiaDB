/******************************************************************************
 * Verify restore clears real Maintenance/Critical modes and is idempotent.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_restore";
   var locations = [
      CHANGEDPREFIX + "_dr_primary",
      CHANGEDPREFIX + "_dr_same_city",
      CHANGEDPREFIX + "_dr_offsite"
   ];

   try
   {
      drAssertNoGroupModes();
      drCreateSixNodeLocationGroup( groupName, locations );

      var maintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( maintenance );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );

      var restoreMaintenance = drRun( "sdb_restore_cluster.sh", [],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( restoreMaintenance );
      drAssertNoGroupModes();

      var critical = drRun( "sdb_start_critical.sh",
         ["-l", locations[0]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( critical );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );

      var restoreCritical = drRun( "sdb_restore_cluster.sh", [],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( restoreCritical );
      drAssertNoGroupModes();

      var repeatedRestore = drRun( "sdb_restore_cluster.sh", [],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( repeatedRestore );
      drAssertNoGroupModes();
      commCheckBusinessStatus( db, 120, true );
   }
   finally
   {
      drDropReplicaGroup( groupName );
   }
}