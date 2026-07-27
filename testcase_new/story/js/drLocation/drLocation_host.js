/******************************************************************************
 * Verify Host targets, including multiple nodes on the same physical Host.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_host";
   var domainName = CHANGEDPREFIX + "_dr_host_domain";
   var locations = [
      CHANGEDPREFIX + "_host_primary",
      CHANGEDPREFIX + "_host_same_city",
      CHANGEDPREFIX + "_host_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      var nodes = drCreateSixNodeLocationGroup( groupName, locations );
      commCreateDomain( db, domainName, [groupName] );

      var targetHost = nodes[0].hostname;
      var targetNodeCount = 0;
      for( var i = 0; i < nodes.length; ++i )
         if( nodes[i].hostname === targetHost ) ++targetNodeCount;
      if( targetNodeCount < 2 )
         throw new Error( "Host target requires at least two nodes on " +
            targetHost + ", actual " + targetNodeCount );

      var startMaintenance = drRun( "sdb_start_maintenance.sh",
         ["-H", targetHost, "-d", domainName], DR_OPERATION_TIMEOUT );
      drAssertSuccess( startMaintenance );
      var maintenanceStarted = targetNodeCount < nodes.length;
      if( maintenanceStarted )
      {
         drAssertGroupModeHost( groupName, "maintenance", targetHost,
            targetNodeCount );
      }
      else
      {
         drAssertContains( startMaintenance, "WARNING" );
         drAssertNoGroupModes( groupName );
      }

      var stopMaintenance = drRun( "sdb_stop_maintenance.sh",
         ["-H", targetHost, "-d", domainName, "--check"],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopMaintenance );
      if( !maintenanceStarted )
         drAssertContains( stopMaintenance,
            "Skip Host not in MaintenanceMode" );
      drAssertNoGroupModes( groupName );

      var startCritical = drRun( "sdb_start_critical.sh",
         ["-H", targetHost, "-d", domainName], DR_OPERATION_TIMEOUT );
      drAssertSuccess( startCritical );
      drAssertGroupModeHost( groupName, "critical", targetHost, 1 );

      var stopCritical = drRun( "sdb_stop_critical.sh",
         ["-H", targetHost, "-d", domainName, "--check"],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopCritical );
      drAssertNoGroupModes( groupName );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
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
         commDropDomain( db, domainName );
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

   if( firstError !== null ) throw firstError;
}
