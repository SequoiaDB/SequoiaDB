/******************************************************************************
@Description :   seqDB-15982:  集合中存在记录，创建自增字段 
@Modify list :   2018-10-15    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15982";
   var field = "id1";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { a: 1 } );

   var rc = dbcl.find();
   var expRecs = [{ "a": 1 }];
   checkRec( rc, expRecs );

   dbcl.createAutoIncrement( { Field: field } );

   dbcl.insert( { a: 7 } );

   var rc = dbcl.find().sort( { field: 1 } );
   var expRecs = [{ "a": 1 }, { "id1": 1, "a": 7 }];
   checkRec( rc, expRecs );

   dbcl.update( { $set: { a: 77 } }, { id1: 1 } );

   rc = dbcl.find().sort( { field: 1 } );
   expRecs = [{ "a": 1 }, { "id1": 1, "a": 77 }];
   checkRec( rc, expRecs );

   dbcl.update( { $set: { a: 777 } }, { a: 1 } );

   rc = dbcl.find().sort( { field: 1 } );
   expRecs = [{ "a": 777 }, { "id1": 1, "a": 77 }];
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( field );

   dbcl.remove();

   while( dbcl.find().next() )
   {
      throw new Error( "remove error" );
   }

   commDropCL( db, COMMCSNAME, clName );
}

