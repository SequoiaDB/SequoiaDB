/******************************************************************************
 * Verify CriticalMode takeover when only one two-node Location remains.
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );

function test ()
{
   var groupName = CHANGEDPREFIX + "_dr_takeover";
   var csName = CHANGEDPREFIX + "_dr_takeover_cs";
   var clName = "records";
   var locations = [
      CHANGEDPREFIX + "_takeover_primary",
      CHANGEDPREFIX + "_takeover_same_city",
      CHANGEDPREFIX + "_takeover_offsite"
   ];
   var firstError = null;

   try
   {
      drAssertNoGroupModes();
      var nodes = drCreateSixNodeLocationGroup( groupName, locations );
      var rg = db.getRG( groupName );
      var cl = commCreateCL( db, csName, clName,
         { Group: groupName, ReplSize: -1 } );
      cl.insert( { seq: 0, phase: "before_takeover" } );
      drAssertTakeoverRecords( cl, [
         { seq: 0, phase: "before_takeover" }
      ] );

      var stoppedIndexes = [4, 5, 0, 1];
      for( var i = 0; i < stoppedIndexes.length; ++i )
      {
         var nodeInfo = nodes[stoppedIndexes[i]];
         rg.getNode( nodeInfo.hostname, nodeInfo.svcname ).stop();
      }
      for( var i = 0; i < stoppedIndexes.length; ++i )
      {
         var nodeInfo = nodes[stoppedIndexes[i]];
         drWaitNodeReportedError(
            nodeInfo.hostname + ":" + nodeInfo.svcname, 60 );
      }

      var start = drRun( "sdb_start_critical.sh",
         ["-l", locations[1]], DR_OPERATION_TIMEOUT );
      drAssertSuccess( start );
      drAssertGroupModeLocation( groupName, "critical", locations[1] );
      drWaitGroupMaster( groupName, 120 );

      drAssertTakeoverRecords( cl, [
         { seq: 0, phase: "before_takeover" }
      ] );
      cl.insert( { seq: 1, phase: "during_takeover" } );
      drAssertTakeoverRecords( cl, [
         { seq: 0, phase: "before_takeover" },
         { seq: 1, phase: "during_takeover" }
      ] );

      rg.start();
      commCheckBusinessStatus( db, 120, true );

      var stop = drRun( "sdb_stop_critical.sh",
         ["-l", locations[1], "--check"], DR_OPERATION_TIMEOUT );
      drAssertSuccess( stop );
      drAssertNoGroupModes( groupName );
      drAssertTakeoverRecords( cl, [
         { seq: 0, phase: "before_takeover" },
         { seq: 1, phase: "during_takeover" }
      ] );
   }
   catch( e )
   {
      firstError = e;
   }
   finally
   {
      try
      {
         if( drReplicaGroupExists( groupName ) )
            db.getRG( groupName ).start();
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
         commCheckBusinessStatus( db, 120, true );
      }
      catch( e )
      {
         if( firstError === null ) firstError = e;
      }
      try
      {
         commDropCS( db, csName, true, "drop takeover test cs",
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

function drAssertTakeoverRecords ( cl, expected )
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
      throw new Error( "Unexpected takeover records. Expected: " +
         JSON.stringify( expected ) + ", actual: " +
         JSON.stringify( actual ) );
}
