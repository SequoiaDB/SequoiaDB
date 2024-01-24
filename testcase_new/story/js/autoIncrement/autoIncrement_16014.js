/******************************************************************************
@Description :   seqDB-16014:  修改起始值  
@Modify list :   2018-10-22    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16014";
   var acquireSize = 10;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1", AcquireSize: acquireSize } } );

   //insert records and check
   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i, "b": i } );
      expRecs.push( { "a": i, "b": i, "id1": 1 + i * acquireSize } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   //alter attributes and check
   dbcl.setAttributes( { AutoIncrement: { Field: "id1", StartValue: 1002 } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
   if( cursor.current().toObj().StartValue !== 1002 )
   {
      throw new Error( "alter failed!" );
   }

   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": 2, "b": 2 } );
      expRecs.push( { "a": 2, "b": 2, "id1": 1 + coordNodes.length * acquireSize + i * acquireSize } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

