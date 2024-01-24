/******************************************************************************
@Description :   seqDB-15983:  同时创建/删除多个自增字段 
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15983";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.createAutoIncrement( [{ Field: "id1", CacheSize: 10, AcquireSize: 1 },
   { Field: "id2", CacheSize: 10, AcquireSize: 1 }] );

   //check autoIncrement
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_id1_SEQ", "SYS_" + clID + "_id2_SEQ"];
   var expArr = [{ Field: "id1", SequenceName: sequenceNames[0] },
   { Field: "id2", SequenceName: sequenceNames[1] }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expArr );

   //check sequence
   var expArr = [{ CacheSize: 10, AcquireSize: 1 }, { CacheSize: 10, AcquireSize: 1 }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expArr[i] );
   }

   dbcl.insert( { a: 1, b: 1 } );

   var rc = dbcl.find();
   var expRecs = [{ id1: 1, id2: 1, a: 1, b: 1 }];
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( ["id1", "id2"] );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement.length !== 0 )
   {
      throw new Error( "drop autoIncrement failed!" );
   }

   rc = dbcl.find();
   expRecs = [{ id1: 1, id2: 1, a: 1, b: 1 }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

