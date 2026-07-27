/******************************************************************************
 * Verify check output semantics and unknown-Host failure behavior.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   try
   {
      testEmptyLocationDifference( originalState );
      drRestoreLocationState( originalState );

      var plan = drBuildHostLocationPlan( originalState );
      var activeLocation = "dr_location_primary";
      var initConfig = drToolDir + "/config/check_base.js";
      drWriteFile( initConfig,
         drBuildConfig( 100, 1000, plan, activeLocation ) );
      drAssertSuccess( drRun( "sdb_init_location.sh",
         ["init", "-c", "config/check_base.js"],
         DR_OPERATION_TIMEOUT ) );
      drAssertLocationPlan( drCaptureLocationState(), plan, activeLocation );

      testHostLocationDifference( plan, activeLocation );
      testActiveLocationDifference( plan, activeLocation );
      testUnknownHostFailure( plan, activeLocation );
   }
   finally
   {
      drRestoreLocationState( originalState );
      drAssertLocationStateEquals( originalState,
         drCaptureLocationState() );
   }
}

function testEmptyLocationDifference ( originalState )
{
   var emptyHost = originalState.nodes[0].hostName;
   for( var i = 0; i < originalState.nodes.length; ++i )
   {
      var node = originalState.nodes[i];
      if( node.hostName === emptyHost )
         drGetReplicaGroup( node.groupName ).getNode(
            node.hostName, node.serviceName ).setLocation( "" );
   }

   var configPath = drToolDir + "/config/check_empty.js";
   drWriteFile( configPath, drBuildConfig( 100, 1000, {}, "" ) );
   var stateBeforeCheck = drCaptureClusterState();
   drClearCheckOutput();
   var result = drRun( "sdb_init_location.sh",
      ["check", "-c", "config/check_empty.js"], DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertCheckSection( "[Diff: Empty Location]" );
   drAssertClusterStateUnchanged( stateBeforeCheck );
}

function testHostLocationDifference ( plan, activeLocation )
{
   var mismatchPlan = JSON.parse( JSON.stringify( plan ) );
   var targetHost = "";
   for( var location in mismatchPlan )
   {
      if( targetHost === "" && mismatchPlan[location].length > 0 )
         targetHost = mismatchPlan[location][0];
      for( var i = mismatchPlan[location].length - 1; i >= 0; --i )
         if( mismatchPlan[location][i] === targetHost )
            mismatchPlan[location].splice( i, 1 );
   }
   mismatchPlan["dr_check_mismatch"] = [targetHost];

   drWriteFile( drToolDir + "/config/check_host.js",
      drBuildConfig( 100, 1000, mismatchPlan, activeLocation ) );
   var stateBeforeCheck = drCaptureClusterState();
   drClearCheckOutput();
   var result = drRun( "sdb_init_location.sh",
      ["check", "-c", "config/check_host.js"], DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertCheckSection( "[Diff: Host Location]" );
   drAssertClusterStateUnchanged( stateBeforeCheck );
}

function testActiveLocationDifference ( plan, activeLocation )
{
   var mismatchPlan = JSON.parse( JSON.stringify( plan ) );
   var expectedActive = "dr_check_expected_active";
   mismatchPlan[expectedActive] = [];
   drWriteFile( drToolDir + "/config/check_active.js",
      drBuildConfig( 100, 1000, mismatchPlan, expectedActive ) );

   var stateBeforeCheck = drCaptureClusterState();
   drClearCheckOutput();
   var result = drRun( "sdb_init_location.sh",
      ["check", "-c", "config/check_active.js"], DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertCheckSection( "[Diff: ActiveLocation]" );
   drAssertClusterStateUnchanged( stateBeforeCheck );
}

function testUnknownHostFailure ( plan, activeLocation )
{
   var unknownPlan = JSON.parse( JSON.stringify( plan ) );
   unknownPlan[activeLocation].push( "dr-check-host-not-exist" );
   drWriteFile( drToolDir + "/config/check_unknown.js",
      drBuildConfig( 100, 1000, unknownPlan, activeLocation ) );

   var stateBeforeCheck = drCaptureClusterState();
   drClearCheckOutput();
   var result = drRun( "sdb_init_location.sh",
      ["check", "-c", "config/check_unknown.js"],
      DR_OPERATION_TIMEOUT );
   drAssertFailure( result );
   drAssertContains( result, "does not in current cluster" );
   if( drReadTextFile( drOutputDir + "/check.out" ) !== "" )
      throw new Error( "Unknown Host failure must not leave check.out" );
   drAssertClusterStateUnchanged( stateBeforeCheck );
}

function drClearCheckOutput ()
{
   drCmd.run( "rm -f " + drShellQuote( drOutputDir + "/check.out" ) );
}

function drAssertCheckSection ( section )
{
   var output = drReadTextFile( drOutputDir + "/check.out" );
   if( output.indexOf( section ) < 0 )
      throw new Error( "check.out is missing " + section +
         "\nActual output:\n" + output );
}