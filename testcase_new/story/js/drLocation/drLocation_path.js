/******************************************************************************
 * Verify relative/absolute input paths and the tool's fixed output directory.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   var plan = drBuildHostLocationPlan( originalState );
   var activeLocation = "dr_location_primary";
   var relativeConfig = "config/path_relative.js";
   var absoluteConfig = drToolDir + "/config/path_absolute.js";
   var relativeFile = "path_relative.info";
   var absoluteFile = drWorkDir + "path_absolute.info";

   try
   {
      drWriteFile( drToolDir + "/" + relativeConfig,
         drBuildConfig( 100, 1000, plan, activeLocation ) );
      drWriteFile( absoluteConfig,
         drBuildConfig( 100, 1000, plan, activeLocation ) );
      var checkPlan = JSON.parse( JSON.stringify( plan ) );
      var sourceLocation = Object.keys( checkPlan ).sort()[0];
      var movedHost = checkPlan[sourceLocation].shift();
      checkPlan["dr_location_path_mismatch"] = [movedHost];
      var locationContent = drBuildLocationFile( checkPlan, activeLocation );
      drWriteFile( drToolDir + "/" + relativeFile, locationContent );
      drWriteFile( absoluteFile, locationContent );

      drAssertSuccess( drRun( "sdb_init_location.sh",
         ["init", "-c", relativeConfig], DR_OPERATION_TIMEOUT ) );
      drAssertLocationPlan( drCaptureLocationState(), plan, activeLocation );

      drClearPathOutputs();
      var relativeShow = drRun( "sdb_init_location.sh",
         ["show", "--quiet", "-c", relativeConfig], DR_OPERATION_TIMEOUT );
      drAssertSuccess( relativeShow );
      drAssertFixedShowOutput( relativeShow );

      drClearPathOutputs();
      var absoluteShow = drRun( "sdb_init_location.sh",
         ["show", "--quiet", "-c", absoluteConfig], DR_OPERATION_TIMEOUT );
      drAssertSuccess( absoluteShow );
      drAssertFixedShowOutput( absoluteShow );
      drAssertLocationPlan( drCaptureLocationState(), plan, activeLocation );

      drClearPathOutputs();
      var relativeCheck = drRun( "sdb_init_location.sh",
         ["check", "-f", relativeFile, "-c", relativeConfig],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( relativeCheck );
      drAssertFixedCheckOutput( relativeCheck );

      drClearPathOutputs();
      var absoluteCheck = drRun( "sdb_init_location.sh",
         ["check", "-f", absoluteFile, "-c", relativeConfig],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( absoluteCheck );
      drAssertFixedCheckOutput( absoluteCheck );
   }
   finally
   {
      drRestoreLocationState( originalState );
      drAssertLocationStateEquals( originalState,
         drCaptureLocationState() );
   }
}

function drClearPathOutputs ()
{
   drCmd.run( "rm -f " +
      drShellQuote( drOutputDir + "/show.out" ) + " " +
      drShellQuote( drOutputDir + "/location.info" ) + " " +
      drShellQuote( drOutputDir + "/check.out" ) );
}

function drAssertFixedShowOutput ( result )
{
   drAssertContains( result, "Output show file:" );
   drAssertContains( result, "/output/show.out" );
   drAssertContains( result, "Output location file:" );
   drAssertContains( result, "/output/location.info" );
   if( drReadTextFile( drOutputDir + "/show.out" ) === "" ||
      drReadTextFile( drOutputDir + "/location.info" ) === "" )
      throw new Error( "show did not write to the fixed output directory" );
}

function drAssertFixedCheckOutput ( result )
{
   drAssertContains( result, "Output check file:" );
   drAssertContains( result, "/output/check.out" );
   if( drReadTextFile( drOutputDir + "/check.out" ) === "" )
      throw new Error( "check did not write to the fixed output directory" );
}

function drAssertPathOutputsEmpty ()
{
   var files = ["show.out", "location.info", "check.out"];
   for( var i = 0; i < files.length; ++i )
      if( drReadTextFile( drOutputDir + "/" + files[i] ) !== "" )
         throw new Error( "Rejected absolute config left output file " +
            files[i] );
}
