/******************************************************************************
@Description :   seqDB-15995:  创建集合时，创建自增字段为嵌套字段  
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15995";

   commDropCL( db, COMMCSNAME, clName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: { Field: "a.1" } } );
   } );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.aa" } } );

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.aa.aa" } } );

   commDropCL( db, COMMCSNAME, clName );
}