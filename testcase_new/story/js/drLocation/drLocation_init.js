/******************************************************************************
 * Verify a successful dr_location init, show, and check workflow.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;


main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   var plan = drBuildHostLocationPlan( originalState );
   var activeLocation = "dr_location_primary";
   var configPath = drToolDir + "/config/init_success.js";
   var configArg = "config/init_success.js";

   try
   {
      drWriteFile( configPath,
         drBuildConfig( 100, 1000, plan, activeLocation ) );

      var initResult = drRun( "sdb_init_location.sh",
         ["init", "-c", configArg], DR_OPERATION_TIMEOUT );
      drAssertSuccess( initResult );
      drAssertContains( initResult,
         "[INFO] Initialization completed successfully" );

      var initializedState = drCaptureLocationState();
      drAssertLocationPlan( initializedState, plan, activeLocation );

      var showResult = drRun( "sdb_init_location.sh",
         ["show", "--quiet", "-c", configArg], DR_OPERATION_TIMEOUT );
      drAssertSuccess( showResult );
      drAssertNotContains( showResult, "[INFO] Show file content:" );

      var showOutput = drReadTextFile( drOutputDir + "/show.out" );
      var locationInfo = drReadTextFile( drOutputDir + "/location.info" );
      if( showOutput === "" || locationInfo === "" )
         throw new Error( "show must generate show.out and location.info" );
      for( var location in plan )
      {
         if( showOutput.indexOf( location ) < 0 ||
            locationInfo.indexOf( location ) < 0 )
            throw new Error( "show output is missing Location " + location );
         for( var i = 0; i < plan[location].length; ++i )
            if( locationInfo.indexOf( plan[location][i] ) < 0 )
               throw new Error( "location.info is missing Host " +
                  plan[location][i] );
      }

      var checkResult = drRun( "sdb_init_location.sh",
         ["check", "-c", configArg], DR_OPERATION_TIMEOUT );
      drAssertSuccess( checkResult );
      drAssertContains( checkResult,
         "All configurations are the same, check passed" );
   }
   finally
   {
      drRestoreLocationState( originalState );
      drAssertLocationStateEquals( originalState,
         drCaptureLocationState() );
   }
}