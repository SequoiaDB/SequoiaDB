/******************************************************************************
 * Verify current init behavior when one configured Host does not exist.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   var expectedPlan = drBuildHostLocationPlan( originalState );
   var configuredPlan = JSON.parse( JSON.stringify( expectedPlan ) );
   var missingHost = "dr-location-host-not-exist";
   var activeLocation = "dr_location_primary";
   var configPath = drToolDir + "/config/init_partial.js";

   configuredPlan[activeLocation].push( missingHost );

   try
   {
      drWriteFile( configPath,
         drBuildConfig( 100, 1000, configuredPlan, activeLocation ) );

      var result = drRun( "sdb_init_location.sh",
         ["init", "-c", "config/init_partial.js"],
         DR_OPERATION_TIMEOUT );

      // The tool catches a per-Host error and completes the remaining init.
      drAssertSuccess( result );
      drAssertContains( result,
         "[ERROR] Failed to set location for " + missingHost );
      drAssertContains( result,
         "[INFO] Initialization completed successfully" );
      drAssertLocationPlan( drCaptureLocationState(), expectedPlan,
         activeLocation );
   }
   finally
   {
      drRestoreLocationState( originalState );
      drAssertLocationStateEquals( originalState,
         drCaptureLocationState() );
   }
}