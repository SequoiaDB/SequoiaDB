/******************************************************************************
*@Description : test SdbSnapshotOption�����洢����
*               TestLink : seqDB-15730:ʹ��SdbSnapshotOption�����洢����
*@auhor       : CSQ 
******************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = "cl_15730";
   var fullName = COMMCSNAME + "." + clName;
   var groupName = commGetGroups( db )[0][0].GroupName;
   db.getCS( COMMCSNAME ).createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   db.createProcedure( function test15730 ( fullName ) { return new SdbSnapshotOption().cond( { Name: fullName } ).sel( { Name: 1, archiveon: 1 } ).sort( { Version: 1 } ).options( { expand: false } ).limit( 1 ).skip( 0 ).flags( 1 ); } );
   try
   {
      var sdbSnapshotOption = db.eval( 'test15730("' + fullName + '")' );
      var cursor = db.snapshot( SDB_SNAP_CATALOG, sdbSnapshotOption );
      var actResult = [];
      var expResult = [{ "Name": fullName, archiveon: 1 }];
      while( cursor.next() )
      {
         actResult.push( cursor.current().toObj() );
      }
      checkResult( actResult, expResult );

      commDropCL( db, COMMCSNAME, clName );
   }
   finally
   {
      db.removeProcedure( "test15730" );
   }
}

