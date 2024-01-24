/******************************************************************************
@Description :   seqDB-12011:在_id字段上创建全文索引
@Modify list :   2018-10-10  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12011";
   var textIndexName = "a_12011";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   createIndexOnId( dbcl, textIndexName );

   createIndexContainId( dbcl, textIndexName );

   dropCL( db, COMMCSNAME, clName, true, true );
}

function createIndexOnId ( dbcl, textIndexName )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( textIndexName, { _id: "text" } );
   } );
}

function createIndexContainId ( dbcl, textIndexName )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndex( textIndexName, { _id: "text", a: "text" } );
   } );
}
