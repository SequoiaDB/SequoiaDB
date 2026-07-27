/******************************************************************************
 * Verify dr_location file/config handling and no-op behavior.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureClusterState();

   testLocationFiles();
   testGroupModeFiles();
   testConfigFiles();

   drAssertClusterStateUnchanged( originalState );
}

function testLocationFiles ()
{
   var invalidFile = drWriteTempFile( "invalid_location.info",
      "invalid content without a section\n" );
   var invalidResult = drRun( "sdb_init_location.sh",
      ["check", "-f", invalidFile] );
   drAssertFailure( invalidResult );
   drAssertContains( invalidResult, "not found or invalid" );
   drAssertNotContains( invalidResult, "TypeError" );

   var emptyFile = drWriteTempFile( "empty_location.info", "" );
   drAssertFailure( drRun( "sdb_init_location.sh",
      ["init", "-f", emptyFile] ) );

   var headerFile = drWriteTempFile( "header_only_location.info",
      "[location]\n" );
   var headerResult = drRun( "sdb_init_location.sh",
      ["init", "-f", headerFile] );
   drAssertFailure( headerResult );
   drAssertContains( headerResult, "not found or invalid" );

   var duplicateHostFile = drWriteTempFile( "duplicate_host.info",
      "[location1]\nhost1\n\n[location2]\nhost1\n" );
   var duplicateResult = drRun( "sdb_init_location.sh",
      ["init", "-f", duplicateHostFile] );
   drAssertFailure( duplicateResult );
   drAssertContains( duplicateResult, "duplicate" );
}

function testGroupModeFiles ()
{
   var emptyFile = "empty_mode.info";
   drWriteFile( drToolDir + "/" + emptyFile, "" );
   var emptyResult = drRun( "sdb_start_maintenance.sh",
      ["-f", emptyFile] );
   drAssertFailure( emptyResult );
   drAssertContains( emptyResult, "not found or invalid" );

   var headerFile = "header_only_mode.info";
   drWriteFile( drToolDir + "/" + headerFile,
      "[location]\n" );
   var headerResult = drRun( "sdb_start_critical.sh",
      ["-f", headerFile] );
   drAssertFailure( headerResult );
   drAssertContains( headerResult, "not found or invalid" );

   var domainOnlyFile = "domain_only.info";
   drWriteFile( drToolDir + "/" + domainOnlyFile,
      "[domain]\ndr_location_missing_domain\n" );
   var domainResult = drRun( "sdb_start_maintenance.sh",
      ["-f", domainOnlyFile] );
   drAssertFailure( domainResult );
   drAssertContains( domainResult, "cannot be used without" );

   var mixedTargetFile = "mixed_target.info";
   drWriteFile( drToolDir + "/" + mixedTargetFile,
      "[location]\nlocation1\n\n[hostname]\nhost1\n" );
   var mixedResult = drRun( "sdb_start_critical.sh",
      ["-f", mixedTargetFile] );
   drAssertFailure( mixedResult );
   drAssertContains( mixedResult, "same time" );
}

function testConfigFiles ()
{
   var missingConfig = "missing_config.js";
   drAssertFailure( drRun( "sdb_start_maintenance.sh",
      ["-l", "location1", "-c", missingConfig] ) );

   var syntaxConfig = "syntax_error.js";
   drWriteFile( drToolDir + "/" + syntaxConfig,
      "sdbCoord = ;\n" );
   drAssertFailure( drRun( "sdb_start_maintenance.sh",
      ["-l", "location1", "-c", syntaxConfig] ) );

   var invalidMinConfig = "invalid_min_keep_time.js";
   drWriteFile( drToolDir + "/" + invalidMinConfig,
      drBuildConfig( 0, 1000 ) );
   var invalidMinResult = drRun( "sdb_start_maintenance.sh",
      ["-l", "location1", "-c", invalidMinConfig] );
   drAssertFailure( invalidMinResult );
   drAssertContains( invalidMinResult, "minKeepTime" );

   var invalidRelationConfig = "invalid_keep_time_relation.js";
   drWriteFile( drToolDir + "/" + invalidRelationConfig,
      drBuildConfig( 10, 10 ) );
   var invalidRelationResult = drRun( "sdb_start_maintenance.sh",
      ["-l", "location1", "-c", invalidRelationConfig] );
   drAssertFailure( invalidRelationResult );
   drAssertContains( invalidRelationResult, "greater than minKeepTime" );
}
