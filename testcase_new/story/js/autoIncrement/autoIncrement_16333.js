/******************************************************************************
@Description :   seqDB-16333:  删除不存在的自增字段 
@Modify list :   2018-11-12    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16333";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1" } } );

   assert.tryThrow( SDB_AUTOINCREMENT_FIELD_NOT_EXIST, function()
   {
      dbcl.dropAutoIncrement( "b1" );
   } );

   commDropCL( db, COMMCSNAME, clName );
}
