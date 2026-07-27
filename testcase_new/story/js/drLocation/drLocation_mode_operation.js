/******************************************************************************
 * Verify reads and writes before, during, and after GroupMode operations.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_operation";
   var csName = CHANGEDPREFIX + "_dr_operation_cs";
   var clName = "records";
   var locations = [
      CHANGEDPREFIX + "_op_primary",
      CHANGEDPREFIX + "_op_same_city",
      CHANGEDPREFIX + "_op_offsite"
   ];
   var expected = [];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      drCreateSixNodeLocationGroup( groupName, locations );
      var cl = commCreateCL( db, csName, clName,
         { Group: groupName, ReplSize: -1 } );

      drInsertPhase( cl, expected, "before_mode" );

      var maintenance = drRun( "sdb_start_maintenance.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( maintenance );
      drAssertGroupModeLocation( groupName, "maintenance", locations[1] );
      drInsertPhase( cl, expected, "during_maintenance" );
      drAssertRecords( cl, expected );

      var stopMaintenance = drRun( "sdb_stop_maintenance.sh",
         ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopMaintenance );
      drAssertNoGroupModes( groupName );
      drInsertPhase( cl, expected, "after_maintenance" );

      var critical = drRun( "sdb_start_critical.sh",
         ["-l", locations[0]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( critical );
      drAssertGroupModeLocation( groupName, "critical", locations[0] );
      drInsertPhase( cl, expected, "during_critical" );
      drAssertRecords( cl, expected );

      var stopCritical = drRun( "sdb_stop_critical.sh",
         ["-l", locations[0], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( stopCritical );
      drAssertNoGroupModes( groupName );
      drInsertPhase( cl, expected, "after_critical" );
      drAssertRecords( cl, expected );

      commCheckBusinessStatus( db, 120, true );
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
         commDropCS( db, csName, true, "drop operation test cs",
            { SkipRecycleBin: true } );
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

function drInsertPhase ( cl, expected, phase )
{
   var record = { seq: expected.length, phase: phase };
   cl.insert( record );
   expected.push( record );
}

function drAssertRecords ( cl, expected )
{
   var actual = [];
   var cursor = cl.find().sort( { seq: 1 } );
   try
   {
      while( cursor.next() )
      {
         var record = cursor.current().toObj();
         actual.push( { seq: record.seq, phase: record.phase } );
      }
   }
   finally
   {
      cursor.close();
   }
   if( JSON.stringify( actual ) !== JSON.stringify( expected ) )
      throw new Error( "Unexpected records.\nExpected: " +
         JSON.stringify( expected ) + "\nActual: " + JSON.stringify( actual ) );
}
