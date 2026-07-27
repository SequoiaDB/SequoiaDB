/******************************************************************************
 * Verify Domain filtering isolates GroupMode operations to matching groups.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var targetGroup = CHANGEDPREFIX + "_dr_domain_in";
   var outsideGroup = CHANGEDPREFIX + "_dr_domain_out";
   var domainName = CHANGEDPREFIX + "_dr_domain";
   var locations = [
      CHANGEDPREFIX + "_dm_primary",
      CHANGEDPREFIX + "_dm_same_city",
      CHANGEDPREFIX + "_dm_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      drCreateLocationGroup( targetGroup, locations, 1 );
      drCreateLocationGroup( outsideGroup, locations, 1 );
      commCreateDomain( db, domainName, [targetGroup] );

      var maintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1], "-d", domainName], DR_OPERATION_TIMEOUT );
      drAssertSuccess( maintenance );
      drAssertGroupModeLocation( targetGroup, "maintenance", locations[1] );
      drAssertNoGroupModes( outsideGroup );

      // stop 仅指定 -d 时：工具要求必须同时指定 -l/--location 或 -H/--hostname
      // （domain 仅为二次过滤），应直接失败且不清掉已有 MaintenanceMode
      var domainOnlyStop = drRun( "sdb_stop_maintenance.sh",
         ["-d", domainName], DR_OPERATION_TIMEOUT );
      drAssertFailure( domainOnlyStop );
      drAssertContains( domainOnlyStop,
         'Cannot specify only "-d,--domain" without specifying "-H,--hostname" or "-l,--location"' );
      drAssertGroupModeLocation( targetGroup, "maintenance", locations[1] );
      drAssertNoGroupModes( outsideGroup );

      var stopMaintenance = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1], "-d", domainName, "--check"],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopMaintenance );
      drAssertNoGroupModes( targetGroup );
      drAssertNoGroupModes( outsideGroup );

      var critical = drRun( "sdb_start_critical.sh",
         ["-l", locations[0], "-d", domainName], DR_OPERATION_TIMEOUT );
      drAssertSuccess( critical );
      drAssertGroupModeLocation( targetGroup, "critical", locations[0] );
      drAssertNoGroupModes( outsideGroup );

      var stopCritical = drRun( "sdb_stop_critical.sh",
         ["-l", locations[0], "-d", domainName, "--check"],
         DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopCritical );
      drAssertNoGroupModes( targetGroup );
      drAssertNoGroupModes( outsideGroup );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      try
      {
         drStopGroupModes( targetGroup );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drStopGroupModes( outsideGroup );
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
         drDropReplicaGroup( targetGroup );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         drDropReplicaGroup( outsideGroup );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
   }

   if( firstError !== null ) throw firstError;
}
