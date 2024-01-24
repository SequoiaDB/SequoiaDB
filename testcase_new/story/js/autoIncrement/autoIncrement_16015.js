/******************************************************************************
@Description :   seqDB-16015:  修改最小值   
@Modify list :   2018-10-22    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16015";
   var acquireSize = 500;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1", AcquireSize: acquireSize, Increment: -1 } } );

   //insert records and check
   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i, "b": i } );
      expRecs.push( { "a": i, "b": i, "id1": -1 - i * acquireSize } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": -1 } );
   checkRec( rc, expRecs );

   //alter attribute and check
   dbcl.setAttributes( { AutoIncrement: { Field: "id1", MinValue: -2000 } } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
   if( cursor.current().toObj().MinValue !== -2000 )
   {
      throw new Error( "alter failed!" );
   }

   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      try
      {
         cl.insert( { "a": i, "b": i } );
         expRecs.push( { "a": i, "b": i, "id1": -1 - coordNodes.length * acquireSize - i * acquireSize } );
      } catch( e )
      {
         if( e.message != SDB_SEQUENCE_EXCEEDED )
         {
            throw new Error( e );
         }
      }

      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": -1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
