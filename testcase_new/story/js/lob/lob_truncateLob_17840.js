/***************************************************************************
@Description :seqDB-17840: oid字符串参数校验
@Modify list :
2019-2-15  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_17840";
   commDropCL( db, COMMCSNAME, clName, true, true );

   //create collection
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //truncateLob oid string checked
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.truncateLob( "", 1 );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.truncateLob( "sas", 12 );
   } );
   commDropCL( db, COMMCSNAME, clName, true, true );
}


