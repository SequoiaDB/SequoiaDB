/******************************************************************************
 * Verify init supports Domain-scoped Location / ActiveLocation updates.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   var targetGroup = CHANGEDPREFIX + "_dr_init_dm_in";
   var outsideGroup = CHANGEDPREFIX + "_dr_init_dm_out";
   var domainIn = CHANGEDPREFIX + "_dr_init_dm_in";
   var domainOut = CHANGEDPREFIX + "_dr_init_dm_out";
   var seedLocations = [
      CHANGEDPREFIX + "_init_dm_primary",
      CHANGEDPREFIX + "_init_dm_same_city",
      CHANGEDPREFIX + "_init_dm_offsite"
   ];
   var firstError = null;

   try
   {
      drCreateLocationGroup( targetGroup, seedLocations, 1 );
      drCreateLocationGroup( outsideGroup, seedLocations, 1 );
      commCreateDomain( db, domainIn, [targetGroup] );
      commCreateDomain( db, domainOut, [outsideGroup] );

      testSelectModeInit( targetGroup, outsideGroup, domainIn );
      testExcludeModeInit( targetGroup, outsideGroup, domainOut );
      testInitFromDomainFile( targetGroup, outsideGroup, domainIn );
      testInvalidDomainConfig( targetGroup, domainIn );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      try { commDropDomain( db, domainIn ); }
      catch( e ) { if( firstError === null ) firstError = e; }
      try { commDropDomain( db, domainOut ); }
      catch( e ) { if( firstError === null ) firstError = e; }
      try { drDropReplicaGroup( targetGroup ); }
      catch( e ) { if( firstError === null ) firstError = e; }
      try { drDropReplicaGroup( outsideGroup ); }
      catch( e ) { if( firstError === null ) firstError = e; }
      try
      {
         drRestoreLocationState( originalState );
         drAssertLocationStateEquals( originalState,
            drCaptureLocationState() );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
   }

   if( firstError !== null ) throw firstError;
}

function drBuildHostOnlyPlan ( groupName, locationName )
{
   var plan = {};
   plan[locationName] = drCollectGroupHosts( groupName );
   return plan;
}

function drAssertGroupStateUnchanged ( groupName, expectedState )
{
   var actual = drGetGroupLocationState( groupName );
   if( JSON.stringify( expectedState ) !== JSON.stringify( actual ) )
      throw new Error( "Group " + groupName + " Location unexpectedly changed." +
         "\nBefore: " + JSON.stringify( expectedState ) +
         "\nAfter: " + JSON.stringify( actual ) );
}

function testSelectModeInit ( targetGroup, outsideGroup, domainIn )
{
   var activeLocation = "dr_init_dm_select_primary";
   var plan = drBuildHostOnlyPlan( targetGroup, activeLocation );
   plan.domains = [domainIn];
   plan.domainMode = 1;

   var outsideBefore = drGetGroupLocationState( outsideGroup );
   var configPath = drToolDir + "/config/init_domain_select.js";
   drWriteFile( configPath, drBuildConfig( 100, 1000, plan, activeLocation ) );

   var result = drRun( "sdb_init_location.sh",
      ["init", "-c", "config/init_domain_select.js"],
      DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertContains( result, "Domains: " + JSON.stringify( [domainIn] ) );
   drAssertContains( result, "DomainMode: 1" );
   drAssertContains( result,
      "[INFO] Initialization completed successfully" );

   drAssertGroupLocationPlan( targetGroup, plan, activeLocation );
   drAssertGroupStateUnchanged( outsideGroup, outsideBefore );
}

function testExcludeModeInit ( targetGroup, outsideGroup, domainOut )
{
   // Seed outside group with a known Domain-scoped Location first.
   var seedActive = "dr_init_dm_exclude_seed";
   var seedPlan = drBuildHostOnlyPlan( outsideGroup, seedActive );
   seedPlan.domains = [domainOut];
   seedPlan.domainMode = 1;
   drWriteFile( drToolDir + "/config/init_domain_exclude_seed.js",
      drBuildConfig( 100, 1000, seedPlan, seedActive ) );
   drAssertSuccess( drRun( "sdb_init_location.sh",
      ["init", "-c", "config/init_domain_exclude_seed.js"],
      DR_OPERATION_TIMEOUT ) );
   var outsideBefore = drGetGroupLocationState( outsideGroup );

   // Exclude domainOut: outside group must stay unchanged; target group updates.
   var excludeActive = "dr_init_dm_exclude_primary";
   var excludePlan = drBuildHostOnlyPlan( targetGroup, excludeActive );
   // Also include outside hosts in the plan; Domain filter should skip them.
   excludePlan[excludeActive] =
      excludePlan[excludeActive].concat( drCollectGroupHosts( outsideGroup ) );
   excludePlan.domains = [domainOut];
   excludePlan.domainMode = 0;

   drWriteFile( drToolDir + "/config/init_domain_exclude.js",
      drBuildConfig( 100, 1000, excludePlan, excludeActive ) );
   var result = drRun( "sdb_init_location.sh",
      ["init", "-c", "config/init_domain_exclude.js"],
      DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertContains( result, "DomainMode: 0" );

   var expectTarget = {};
   expectTarget[excludeActive] = drCollectGroupHosts( targetGroup );
   drAssertGroupLocationPlan( targetGroup, expectTarget, excludeActive );
   drAssertGroupStateUnchanged( outsideGroup, outsideBefore );
}

function testInitFromDomainFile ( targetGroup, outsideGroup, domainIn )
{
   var outsideBefore = drGetGroupLocationState( outsideGroup );
   var activeLocation = "dr_init_dm_file_primary";
   var hosts = drCollectGroupHosts( targetGroup );
   var fileContent = "[domains(1)]\n" + domainIn + "\n\n" +
      "[" + activeLocation + "(active)]\n" + hosts.join( "\n" ) + "\n";
   var locationFile = drWriteTempFile( "init_domain_location.info",
      fileContent );

   var result = drRun( "sdb_init_location.sh",
      ["init", "-f", locationFile], DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertContains( result, "Domains: " + JSON.stringify( [domainIn] ) );
   drAssertContains( result, "DomainMode: 1" );

   var expectPlan = {};
   expectPlan[activeLocation] = hosts;
   drAssertGroupLocationPlan( targetGroup, expectPlan, activeLocation );
   drAssertGroupStateUnchanged( outsideGroup, outsideBefore );
}

function testInvalidDomainConfig ( targetGroup, domainIn )
{
   var hosts = drCollectGroupHosts( targetGroup );

   var missingDomainPlan = { "dr_init_dm_primary": hosts };
   missingDomainPlan.domains = ["dr_location_domain_not_exist"];
   missingDomainPlan.domainMode = 1;
   drWriteFile( drToolDir + "/config/init_domain_missing.js",
      drBuildConfig( 100, 1000, missingDomainPlan, "dr_init_dm_primary" ) );
   var missingResult = drRun( "sdb_init_location.sh",
      ["init", "-c", "config/init_domain_missing.js"],
      DR_OPERATION_TIMEOUT );
   drAssertFailure( missingResult );
   drAssertContains( missingResult, "Cannot find domain" );

   var badModePlan = { "dr_init_dm_primary": hosts };
   badModePlan.domains = [domainIn];
   badModePlan.domainMode = 2;
   drWriteFile( drToolDir + "/config/init_domain_bad_mode.js",
      drBuildConfig( 100, 1000, badModePlan, "dr_init_dm_primary" ) );
   var badModeResult = drRun( "sdb_init_location.sh",
      ["init", "-c", "config/init_domain_bad_mode.js"],
      DR_OPERATION_TIMEOUT );
   drAssertFailure( badModeResult );
   drAssertContains( badModeResult, "domainMode must be in [0, 1]" );

   var badDomainFile = drWriteTempFile( "bad_domain_mode.info",
      "[domains(2)]\n" + domainIn + "\n\n[loc(active)]\n" +
      hosts[0] + "\n" );
   var badFileResult = drRun( "sdb_init_location.sh",
      ["init", "-f", badDomainFile], DR_OPERATION_TIMEOUT );
   drAssertFailure( badFileResult );
   drAssertContains( badFileResult,
      "can only be [domains(0)] or [domains(1)]" );
}
