/******************************************************************************
 * Verify connection failures are fast, non-mutating, and leave no output.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureClusterState();
   testInvalidCoord();
   testUnreadableConfig();
   testInvalidAuthenticationWhenEnabled();
   drAssertClusterStateUnchanged( originalState );
}

function testInvalidCoord ()
{
   var normalCoord = COORDHOSTNAME + ":" + COORDSVCNAME;
   var config = drBuildConfig( 100, 1000 ).replace(
      "sdbCoord = \"" + normalCoord + "\";",
      "sdbCoord = \"127.0.0.1:1\";" );
   drWriteFile( drToolDir + "/config/invalid_coord.js", config );

   drClearGeneratedOutput();
   var stateBeforeShow = drCaptureClusterState();
   var show = drRun( "sdb_init_location.sh",
      ["show", "-c", "config/invalid_coord.js"], 10 );
   drAssertFailure( show );
   drAssertGeneratedOutputEmpty();
   drAssertClusterStateUnchanged( stateBeforeShow );

   var stateBeforeRestore = drCaptureClusterState();
   var restore = drRun( "sdb_restore_cluster.sh",
      ["-c", "config/invalid_coord.js"], 10 );
   drAssertFailure( restore );
   drAssertClusterStateUnchanged( stateBeforeRestore );
}

function testInvalidAuthenticationWhenEnabled ()
{
   var invalidUser = "dr_location_user_not_exist";
   var invalidPassword = "dr_location_invalid_password";
   var connection;
   try
   {
      connection = new Sdb( COORDHOSTNAME, COORDSVCNAME,
         invalidUser, invalidPassword );
      connection.listReplicaGroups();
      println( "Authentication is not enabled; skip invalid credential check" );
      return;
   }
   catch( e )
   {
   }
   finally
   {
      if( connection !== undefined ) connection.close();
   }

   var config = drBuildConfig( 100, 1000 )
      .replace( "sdbUser = \"\";",
         "sdbUser = \"" + invalidUser + "\";" )
      .replace( "sdbPassword = \"\";",
         "sdbPassword = \"" + invalidPassword + "\";" );
   drWriteFile( drToolDir + "/config/invalid_auth.js", config );

   drClearGeneratedOutput();
   var stateBeforeCheck = drCaptureClusterState();
   var result = drRun( "sdb_init_location.sh",
      ["show", "-c", "config/invalid_auth.js"], 10 );
   drAssertFailure( result );
   drAssertGeneratedOutputEmpty();
   drAssertClusterStateUnchanged( stateBeforeCheck );
}

function testUnreadableConfig ()
{
   if( parseInt( drCmd.run( "id -u" ), 10 ) === 0 )
   {
      println( "Running as root; skip unreadable config permission check" );
      return;
   }

   var configPath = drToolDir + "/config/unreadable.js";
   drWriteFile( configPath, drBuildConfig( 100, 1000 ) );
   try
   {
      drCmd.run( "chmod 000 " + drShellQuote( configPath ) );
      drClearGeneratedOutput();
      var stateBeforeShow = drCaptureClusterState();
      var result = drRun( "sdb_init_location.sh",
         ["show", "-q", "-c", "config/unreadable.js"], 10 );
      drAssertFailure( result );
      drAssertGeneratedOutputEmpty();
      drAssertClusterStateUnchanged( stateBeforeShow );
   }
   finally
   {
      drCmd.run( "chmod 600 " + drShellQuote( configPath ) );
   }
}

function drClearGeneratedOutput ()
{
   drCmd.run( "rm -f " +
      drShellQuote( drOutputDir + "/show.out" ) + " " +
      drShellQuote( drOutputDir + "/location.info" ) + " " +
      drShellQuote( drOutputDir + "/check.out" ) );
}

function drAssertGeneratedOutputEmpty ()
{
   var files = ["show.out", "location.info", "check.out"];
   for( var i = 0; i < files.length; ++i )
      if( drReadTextFile( drOutputDir + "/" + files[i] ) !== "" )
         throw new Error( "Connection failure left output file " + files[i] );
}
