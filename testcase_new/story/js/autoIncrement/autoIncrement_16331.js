/******************************************************************************
@Description :   seqDB-16331:  创建已存在的自增字段  
@Modify list :   2018-11-12    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16331";

   commDropCL( db, COMMCSNAME, clName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: [{ Field: "a1", Increment: 2 }, { Field: "a1", Increment: 3 }, { Field: "a1", Increment: 4 }] } );
   } );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1", Increment: 2 } } );

   assert.tryThrow( SDB_AUTOINCREMENT_FIELD_CONFLICT, function()
   {
      dbcl.createAutoIncrement( { Field: "a1" } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}
