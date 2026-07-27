/******************************************************************************
 * Verify every dr_location command fails safely without a catalog primary.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_catalog_fault";
   var locations = [
      CHANGEDPREFIX + "_catalog_primary",
      CHANGEDPREFIX + "_catalog_same_city",
      CHANGEDPREFIX + "_catalog_offsite"
   ];
   var configArg = "config/catalog_fault.js";
   var cataRG = null;
   var catalogStopped = false;
   var baselineState = null;
   var firstError = null;
   var issues = [];

   try
   {
      cataRG = db.getCataRG();
      var detailCursor = cataRG.getDetail();
      var catalogDetail;
      try
      {
         catalogDetail = detailCursor.current().toObj();
      }
      finally
      {
         detailCursor.close();
      }
      if( ( catalogDetail.Group || [] ).length < 3 )
      {
         throw new Error( "Catalog no-primary test requires at least three nodes" );
      }

      drAssertNoGroupModes();
      drCreateLocationGroup( groupName, locations, 1 );
      drWriteFile( drToolDir + "/" + configArg,
         drBuildConfig( 100, 1000, {}, "" ) );

      var startMaintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( startMaintenance );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );
      baselineState = drCaptureClusterState();

      var survivor = cataRG.getMaster();
      cataRG.stop();
      catalogStopped = true;
      survivor.start();
      sleep( 3000 );

      var commands = [
         { name: "show", script: "sdb_init_location.sh",
            args: ["show", "-q", "-c", configArg], output: true },
         { name: "check", script: "sdb_init_location.sh",
            args: ["check", "-c", configArg], output: true },
         { name: "init", script: "sdb_init_location.sh",
            args: ["init", "-c", configArg], output: true },
         { name: "start", script: "sdb_start_critical.sh",
            args: ["-l", locations[0]] },
         { name: "stop", script: "sdb_stop_maintenance.sh",
            args: ["-l", locations[1], "--check"] },
         { name: "restore", script: "sdb_restore_cluster.sh", args: [] }
      ];

      for( var i = 0; i < commands.length; ++i )
      {
         if( commands[i].output ) drClearCatalogOutputs();
         try
         {
            var result = drRun( commands[i].script, commands[i].args, 30 );
            if( result.rc === 0 )
               issues.push( commands[i].name +
                  " unexpectedly succeeded without a catalog primary" );
            else if( result.rc === 124 )
               issues.push( commands[i].name +
                  " timed out without a catalog primary" );
            if( commands[i].output && !drCatalogOutputsEmpty() )
               issues.push( commands[i].name +
                  " left output without a catalog primary" );
         }
         catch( e )
         {
            issues.push( commands[i].name +
               " runner failed before returning a command status: " + e );
         }
      }
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      if( cataRG !== null )
      {
         try
         {
            cataRG.start();
            catalogStopped = false;
         }
         catch( e )
         {
            if( firstError === null ) firstError = e;
         }
      }
      try
      {
         commCheckBusinessStatus( db, 120, true );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         if( baselineState !== null )
         {
            drAssertClusterStateUnchanged( baselineState );
            drAssertGroupModeLocation(
               groupName, "maintenance", locations[1] );
         }
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drStopGroupModes( groupName );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drDropReplicaGroup( groupName );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
   }

   if( catalogStopped )
      throw new Error( "Catalog group recovery did not complete" );
   if( firstError !== null ) throw firstError;
   if( issues.length > 0 )
      throw new Error( "Unexpected catalog no-primary behavior:\n" +
         issues.join( "\n" ) );
}

function drClearCatalogOutputs ()
{
   drCmd.run( "rm -f " +
      drShellQuote( drOutputDir + "/show.out" ) + " " +
      drShellQuote( drOutputDir + "/location.info" ) + " " +
      drShellQuote( drOutputDir + "/check.out" ) );
}

function drCatalogOutputsEmpty ()
{
   return drReadTextFile( drOutputDir + "/show.out" ) === "" &&
      drReadTextFile( drOutputDir + "/location.info" ) === "" &&
      drReadTextFile( drOutputDir + "/check.out" ) === "";
}
