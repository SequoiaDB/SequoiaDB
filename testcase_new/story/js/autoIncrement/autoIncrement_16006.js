/******************************************************************************
@Description :   seqDB-16006:  普通集合中存在自增字段，删除集合 
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16006";
   var field = "id1";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: field } } );

   dbcl.insert( { a: 1 } );

   var rc = dbcl.find();
   var expRecs = [{ "id1": 1, "a": 1 }];
   checkRec( rc, expRecs );

   //drop CL and check
   commDropCL( db, COMMCSNAME, clName );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   while( cursor.next() )
   {
      if( cursor.current().toObj().AutoIncrement.length !== 0 )
      {
         throw new Error( "drop failed!" );
      }
   }

   //create CL again
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: field } } );

   //check autoIncrement and sequence
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expSequenceObj = [{ Field: field, SequenceName: sequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expSequenceObj );
   checkSequence( db, sequenceName, {} );

   //insert record and check
   var coordNodes = getCoordNodeNames( db );
   var expRecs = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "b": i, "c": i } );
      expRecs.push( { "b": i, "c": i, "id1": 1 + i * 1000 } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

