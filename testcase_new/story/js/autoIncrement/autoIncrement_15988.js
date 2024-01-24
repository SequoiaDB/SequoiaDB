/******************************************************************************
@Description :   seqDB-15988:range分区表上创建/删除自增字段 
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db);
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_15988";
   var field = "id1";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: dataGroupNames[0], ShardingKey: { a: 1 }, ShardingType: "range" } );

   dbcl.insert( { a: 1 } );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], 50 );

   dbcl.createAutoIncrement( { Field: field } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expArr = [{ Field: field, SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expArr );

   checkSequence( db, sequenceName, {} );

   dbcl.insert( { a: 2 } );

   var rc = dbcl.find().sort( { "id1": 1 } );
   var expRecs = [{ "a": 1 }, { "id1": 1, "a": 2 }];
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( field );

   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   if( cursor.current().toObj().AutoIncrement.length !== 0 )
   {
      throw new Error( "drop autoIncrement failed!" );
   }

   dbcl.insert( { "a": 3 } );

   var rc = dbcl.find().sort( { "id1": 1 } );
   var expRecs = [{ "a": 1 }, { "a": 3 }, { "id1": 1, "a": 2 }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

