/******************************************************************************
@Description :   seqDB-16716:创建集合时，指定CurrentValue属性 
@Modify list :   2018-11-27    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16716";

   commDropCL( db, COMMCSNAME, clName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: { Field: "a1", CurrentValue: 5 } } );
   } );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a2" } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_a2_SEQ";
   var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
   if( cursor.current().toObj().Initial !== true )
   {
      throw new Error( "need_error!" );
   }

   dbcl.setAttributes( { AutoIncrement: { Field: "a2", CurrentValue: 50 } } );

   var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
   if( cursor.current().toObj().Initial !== false || cursor.current().toObj().CurrentValue !== 50 )
   {
      throw new Error( "need_error!" );
   }

   //insert records
   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i } );
      expRecs.push( { "a": i, "a2": 51 + i * 1000 } );
      coord.close();
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
