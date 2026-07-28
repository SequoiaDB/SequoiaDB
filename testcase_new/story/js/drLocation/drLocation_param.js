/******************************************************************************
 * Verify help text and command-line argument validation for dr_location tools.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureClusterState();

   testHelp();
   testUnknownArguments();
   testMissingArguments();
   testStartWithNoMatchingTarget();

   drAssertClusterStateUnchanged( originalState );
}

function testHelp ()
{
   var scripts = [
      "sdb_init_location.sh",
      "sdb_start_maintenance.sh",
      "sdb_stop_maintenance.sh",
      "sdb_start_critical.sh",
      "sdb_stop_critical.sh",
      "sdb_restore_cluster.sh"
   ];

   for( var i = 0; i < scripts.length; ++i )
   {
      var result = drRun( scripts[i], ["--help"] );
      drAssertSuccess( result );
      drAssertContains( result, "Usage:" );
   }
}

function testUnknownArguments ()
{
   var scripts = [
      "sdb_init_location.sh",
      "sdb_start_maintenance.sh",
      "sdb_stop_maintenance.sh",
      "sdb_start_critical.sh",
      "sdb_stop_critical.sh",
      "sdb_restore_cluster.sh"
   ];

   for( var i = 0; i < scripts.length; ++i )
   {
      var result = drRun( scripts[i], ["--dr-location-invalid-option"] );
      drAssertFailure( result );
   }

   var unknownCommand = drRun( "sdb_init_location.sh", ["unknown"] );
   drAssertFailure( unknownCommand );
   drAssertContains( unknownCommand, "Unknown command" );
}

function testMissingArguments ()
{
   var initOptions = ["-c", "-f"];
   for( var i = 0; i < initOptions.length; ++i )
   {
      var checkResult = drRun( "sdb_init_location.sh",
         ["check", initOptions[i]], 1 );
      drAssertFailure( checkResult );
      drAssertContains( checkResult, "Usage:" );
   }

   var groupModeScripts = [
      "sdb_start_maintenance.sh",
      "sdb_stop_maintenance.sh",
      "sdb_start_critical.sh",
      "sdb_stop_critical.sh"
   ];
   var groupModeOptions = ["-c", "-f", "-l", "-H", "-d"];
   for( var scriptIndex = 0;
      scriptIndex < groupModeScripts.length; ++scriptIndex )
   {
      for( var optionIndex = 0;
         optionIndex < groupModeOptions.length; ++optionIndex )
      {
         var result = drRun( groupModeScripts[scriptIndex],
            [groupModeOptions[optionIndex]], 1 );
         drAssertFailure( result );
         drAssertContains( result, "Usage:" );
      }
   }

   var restoreResult = drRun( "sdb_restore_cluster.sh", ["-c"], 1 );
   drAssertFailure( restoreResult );
   drAssertContains( restoreResult, "Usage:" );
}

function testStartWithNoMatchingTarget ()
{
   var maintenance = drRun( "sdb_start_maintenance.sh" );
   drAssertFailure( maintenance );
   drAssertContains( maintenance, "No target to start MaintenanceMode" );

   var critical = drRun( "sdb_start_critical.sh" );
   drAssertFailure( critical );
   drAssertContains( critical, "No target to start CriticalMode" );

   var maintenanceDomain = drRun( "sdb_start_maintenance.sh",
      ["-d", "dr_location_missing_domain"] );
   drAssertFailure( maintenanceDomain );
   drAssertContains( maintenanceDomain, "No target to start MaintenanceMode" );

   var criticalDomain = drRun( "sdb_start_critical.sh",
      ["-d", "dr_location_missing_domain"] );
   drAssertFailure( criticalDomain );
   drAssertContains( criticalDomain, "No target to start CriticalMode" );
}
