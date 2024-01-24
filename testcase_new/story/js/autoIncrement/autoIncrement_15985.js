/******************************************************************************
@Description :   seqDB-15985:新增自增字段与已存在的自增字段存在包含关系
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15985";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.aa" } } );

   //create lower layer field
   createAutoIncrement( dbcl, "a.aa.aa" );

   //create higher layer field
   createAutoIncrement( dbcl, "a" );

   //create same layer field
   dbcl.createAutoIncrement( { Field: "a.bb" } );

   //check autoIncrement 
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a.aa_SEQ",
   "SYS_" + clID + "_a.bb_SEQ"];
   var expIncrements = [{ Field: "a.aa", SequenceName: sequenceNames[0] },
   { Field: "a.bb", SequenceName: sequenceNames[1] }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrements );

   //check sequence
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], {} );
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.insert( { a: 1 } );
   } );

   var doc = [{ a: { cc: 2 } }, { a: { bb: { bbb: 3 } } }];
   dbcl.insert( doc );
   var expRecs = [{ "a": { "cc": 2, "aa": 1, "bb": 1 } }, { "a": { "bb": { "bbb": 3 }, "aa": 2 } }];
   var rc = dbcl.find();
   checkRec( rc, expRecs );

   dbcl.dropAutoIncrement( ["a.aa", "a.bb"] );

   commDropCL( db, COMMCSNAME, clName );
}

function createAutoIncrement ( dbcl, field )
{
   assert.tryThrow( SDB_AUTOINCREMENT_FIELD_CONFLICT, function()
   {
      dbcl.createAutoIncrement( { Field: field } );
   } );

}