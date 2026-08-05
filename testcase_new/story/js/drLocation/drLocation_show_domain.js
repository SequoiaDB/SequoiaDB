/******************************************************************************
 * Verify show supports Domain filtering for layout and GroupMode output.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var originalState = drCaptureLocationState();
   var targetGroup = CHANGEDPREFIX + "_dr_show_dm_in";
   var outsideGroup = CHANGEDPREFIX + "_dr_show_dm_out";
   var domainIn = CHANGEDPREFIX + "_dr_show_dm_in";
   var domainOut = CHANGEDPREFIX + "_dr_show_dm_out";
   // Keep three Locations per group. CriticalMode is started on ActiveLocation
   // so show can emit Location-scoped GroupMode rows (Maintenance is not merged
   // as Location in show output).
   var seedLocations = [
      CHANGEDPREFIX + "_show_dm_primary",
      CHANGEDPREFIX + "_show_dm_same_city",
      CHANGEDPREFIX + "_show_dm_offsite"
   ];
   var targetLocations = [
      "dr_show_dm_target_primary",
      "dr_show_dm_target_same_city",
      "dr_show_dm_target_offsite"
   ];
   var outsideLocations = [
      "dr_show_dm_outside_primary",
      "dr_show_dm_outside_same_city",
      "dr_show_dm_outside_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      var targetNodes = drCreateLocationGroup( targetGroup, seedLocations, 1 );
      var outsideNodes = drCreateLocationGroup( outsideGroup, seedLocations, 1 );
      commCreateDomain( db, domainIn, [targetGroup] );
      commCreateDomain( db, domainOut, [outsideGroup] );

      // Assign domain-unique Location names per node.
      assignGroupLocations( targetGroup, targetNodes, targetLocations );
      assignGroupLocations( outsideGroup, outsideNodes, outsideLocations );

      // show 配置中的 initLocationObject 必须同时包含 Location 与 Domain。
      var locationPlan = mergeLocationPlans(
         buildLocationPlanFromNodes( targetNodes, targetLocations ),
         buildLocationPlanFromNodes( outsideNodes, outsideLocations ) );

      // Use CriticalMode on ActiveLocation: show merges it as "Location: xxx".
      var critical = drRun( "sdb_start_critical.sh",
         ["-l", targetLocations[0], "-d", domainIn],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( critical );
      drAssertGroupModeLocation( targetGroup, "critical",
         targetLocations[0] );
      drAssertNoGroupModes( outsideGroup );

      var outsideCritical = drRun( "sdb_start_critical.sh",
         ["-l", outsideLocations[0], "-d", domainOut],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( outsideCritical );
      drAssertGroupModeLocation( outsideGroup, "critical",
         outsideLocations[0] );

      testShowSelectDomain( domainIn, targetNodes, outsideNodes,
         targetLocations[0], outsideLocations[0], locationPlan,
         targetLocations[0] );
      testShowExcludeDomain( domainOut, targetNodes, outsideNodes,
         targetLocations[0], outsideLocations[0], locationPlan,
         targetLocations[0] );
      testShowMissingDomain( locationPlan, targetLocations[0] );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      try { drStopGroupModes( targetGroup ); }
      catch( e ) { if( firstError === null ) firstError = e; }
      try { drStopGroupModes( outsideGroup ); }
      catch( e ) { if( firstError === null ) firstError = e; }
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

function assignGroupLocations ( groupName, nodes, locations )
{
   if( nodes.length !== locations.length )
      throw new Error( "nodes/locations length mismatch for " + groupName );
   var rg = db.getRG( groupName );
   for( var i = 0; i < nodes.length; ++i )
      rg.getNode( nodes[i].hostname, nodes[i].svcname ).setLocation(
         locations[i] );
   rg.setActiveLocation( locations[0] );
}

function buildLocationPlanFromNodes ( nodes, locations )
{
   var plan = {};
   for( var i = 0; i < nodes.length; ++i )
   {
      var location = locations[i];
      var host = String( nodes[i].hostname );
      if( plan[location] === undefined ) plan[location] = [];
      if( plan[location].indexOf( host ) < 0 ) plan[location].push( host );
   }
   return plan;
}

function mergeLocationPlans ( left, right )
{
   var plan = JSON.parse( JSON.stringify( left ) );
   for( var location in right )
   {
      if( plan[location] === undefined ) plan[location] = [];
      for( var i = 0; i < right[location].length; ++i )
      {
         var host = right[location][i];
         if( plan[location].indexOf( host ) < 0 ) plan[location].push( host );
      }
   }
   return plan;
}

function drBuildShowDomainConfig ( locationPlan, domains, domainMode,
   activeLocation )
{
   var plan = JSON.parse( JSON.stringify( locationPlan ) );
   plan.domains = domains;
   plan.domainMode = domainMode;
   return drBuildConfig( 100, 1000, plan, activeLocation );
}

function drHostNames ( nodes )
{
   var hosts = {};
   for( var i = 0; i < nodes.length; ++i )
      hosts[String( nodes[i].hostname )] = true;
   return Object.keys( hosts );
}

function drExclusiveHosts ( candidateNodes, otherNodes )
{
   var otherHosts = drHostNames( otherNodes );
   var exclusive = [];
   var hosts = drHostNames( candidateNodes );
   for( var i = 0; i < hosts.length; ++i )
      if( otherHosts.indexOf( hosts[i] ) < 0 )
         exclusive.push( hosts[i] );
   return exclusive;
}

function testShowSelectDomain ( domainIn, targetNodes, outsideNodes,
   targetCritLoc, outsideCritLoc, locationPlan, activeLocation )
{
   drWriteFile( drToolDir + "/config/show_domain_select.js",
      drBuildShowDomainConfig( locationPlan, [domainIn], 1, activeLocation ) );

   var result = drRun( "sdb_init_location.sh",
      ["show", "-c", "config/show_domain_select.js"],
      DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertContains( result, "[INFO] Apply domain mode: 1" );
   drAssertContains( result,
      "[INFO] Apply domains scope: " + JSON.stringify( [domainIn] ) );
   drAssertNotContains( result, "[INFO] Apply scope: cluster" );

   var showOutput = drReadTextFile( drOutputDir + "/show.out" );
   if( showOutput === "" )
      throw new Error( "show must generate show.out" );

   // Domain filter is validated mainly via GroupMode Location rows. On a
   // single-host mixed-Location cluster, layout may only have PartialHost.
   if( showOutput.indexOf( "[GroupMode Runtime Info]" ) < 0 )
      throw new Error( "select-mode show missing GroupMode Runtime Info\n" +
         showOutput );
   if( showOutput.indexOf( "Location: " + targetCritLoc ) < 0 )
      throw new Error( "select-mode show missing target GroupMode Location\n" +
         showOutput );
   if( showOutput.indexOf( "Location: " + outsideCritLoc ) >= 0 )
      throw new Error( "select-mode show still contains outside GroupMode " +
         "Location\n" + showOutput );

   var targetHosts = drHostNames( targetNodes );
   for( var t = 0; t < targetHosts.length; ++t )
      if( showOutput.indexOf( targetHosts[t] ) < 0 )
         throw new Error( "select-mode show missing target Host: " +
            targetHosts[t] + "\n" + showOutput );

   var outsideOnlyHosts = drExclusiveHosts( outsideNodes, targetNodes );
   for( var h = 0; h < outsideOnlyHosts.length; ++h )
   {
      if( showOutput.indexOf( outsideOnlyHosts[h] ) >= 0 )
         throw new Error( "select-mode show still contains outside-only Host: " +
            outsideOnlyHosts[h] + "\n" + showOutput );
   }
}

function testShowExcludeDomain ( domainOut, targetNodes, outsideNodes,
   targetCritLoc, outsideCritLoc, locationPlan, activeLocation )
{
   drWriteFile( drToolDir + "/config/show_domain_exclude.js",
      drBuildShowDomainConfig( locationPlan, [domainOut], 0, activeLocation ) );

   var result = drRun( "sdb_init_location.sh",
      ["show", "-c", "config/show_domain_exclude.js"],
      DR_OPERATION_TIMEOUT );
   drAssertSuccess( result );
   drAssertContains( result, "[INFO] Apply domain mode: 0" );
   drAssertContains( result,
      "[INFO] Apply domains scope: " + JSON.stringify( [domainOut] ) );

   var showOutput = drReadTextFile( drOutputDir + "/show.out" );
   if( showOutput.indexOf( "[GroupMode Runtime Info]" ) < 0 )
      throw new Error( "exclude-mode show missing GroupMode Runtime Info\n" +
         showOutput );
   if( showOutput.indexOf( "Location: " + targetCritLoc ) < 0 )
      throw new Error( "exclude-mode show missing target GroupMode Location\n" +
         showOutput );
   if( showOutput.indexOf( "Location: " + outsideCritLoc ) >= 0 )
      throw new Error( "exclude-mode show still contains excluded GroupMode " +
         "Location\n" + showOutput );

   var targetHosts = drHostNames( targetNodes );
   for( var i = 0; i < targetHosts.length; ++i )
      if( showOutput.indexOf( targetHosts[i] ) < 0 )
         throw new Error( "exclude-mode show missing target Host: " +
            targetHosts[i] + "\n" + showOutput );

   var outsideOnlyHosts = drExclusiveHosts( outsideNodes, targetNodes );
   for( var o = 0; o < outsideOnlyHosts.length; ++o )
   {
      if( showOutput.indexOf( outsideOnlyHosts[o] ) >= 0 )
         throw new Error( "exclude-mode show still contains excluded Host: " +
            outsideOnlyHosts[o] + "\n" + showOutput );
   }
}

function testShowMissingDomain ( locationPlan, activeLocation )
{
   drWriteFile( drToolDir + "/config/show_domain_missing.js",
      drBuildShowDomainConfig( locationPlan,
         ["dr_location_domain_not_exist"], 1, activeLocation ) );

   var result = drRun( "sdb_init_location.sh",
      ["show", "-c", "config/show_domain_missing.js"],
      DR_OPERATION_TIMEOUT );
   drAssertFailure( result );
   drAssertContains( result, "Cannot find domain" );
}
