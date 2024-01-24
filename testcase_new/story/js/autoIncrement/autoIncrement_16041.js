/******************************************************************************
@Description :   seqDB-16041:  Cycled字段参数校验 
@Modify list :   2018-10-25    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16041";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a0" } } );

   var expRecs = [];
   for( var i = 0; i < 2; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( { "a": i, "a0": i + 1 } );
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled true
   dbcl.setAttributes( { AutoIncrement: { Field: "a0", Cycled: true, MaxValue: 1001, Increment: 1000, CacheSize: 1, AcquireSize: 1 } } );

   for( var i = 0; i < 2; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( { "a": i, "a0": i * 1000 + 1 } );
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled false
   dbcl.setAttributes( { AutoIncrement: { Field: "a0", Cycled: false, MaxValue: 1001, Increment: 1000, CacheSize: 1, AcquireSize: 1 } } );

   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      dbcl.insert( { "a": i } );
   } );

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   //alter Cycled string
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.setAttributes( { Field: "a0", Cycled: "cycled" } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}