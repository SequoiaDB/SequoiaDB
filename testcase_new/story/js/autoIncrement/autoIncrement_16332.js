/******************************************************************************
@Description :   seqDB-16332:  修改不存在的自增字段的属性 
@Modify list :   2018-11-12    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16332";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1" } } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setAttributes( { Field: "a2", Increment: 3 } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}
