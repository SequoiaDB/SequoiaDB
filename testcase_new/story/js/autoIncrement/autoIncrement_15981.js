/******************************************************************************
@Description :   seqDB-15981:  集合中不存在记录，创建/删除自增字段 
@Modify list :   2018-10-15    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15981";
   var field = "id1";
   var cacheSize = 10;
   var acquireSize = 1;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.createAutoIncrement( { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize } );

   //check sequence
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expSequenceObj = { CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, sequenceName, expSequenceObj );

   dbcl.insert( { a: 7 } );

   var rc = dbcl.find();
   var expRecs = [{ id1: 1, a: 7 }];
   checkRec( rc, expRecs );

   dbcl.update( { $set: { a: 77 } }, { id1: 1 } );

   rc = dbcl.find();
   expRecs = [{ id1: 1, a: 77 }];
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( field );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement.length !== 0 )
   {
      throw new Error( "drop autoIncrement failed!" );
   }

   dbcl.insert( { a: 777 } );

   rc = dbcl.find().sort( { id1: 1 } );
   expRecs = [{ a: 777 }, { id1: 1, a: 77 }];
   checkRec( rc, expRecs );

   dbcl.createAutoIncrement( { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize } );

   //insert records and check
   var coordNodes = getCoordNodeNames( db );
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": 2, "b": 2 } );
      expRecs.push( { "a": 2, "b": 2, "id1": 1 + i } );
      coord.close();
   }

   var rc = dbcl.find().sort( { "id1": 1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

