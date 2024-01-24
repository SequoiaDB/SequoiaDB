/******************************************************************************
@Description :   seqDB-15994: 创建集合时，创建重复的自增字段 
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15994";

   commDropCL( db, COMMCSNAME, clName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: [{ Field: "id1" }, { Field: "id1" }] } );
   } );
}