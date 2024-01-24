/******************************************************************************
@Description :   seqDB-15996:  创建集合时，创建字段名存在包含关系的自增字段 
@Modify list :   2018-10-18    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15996";

   commDropCL( db, COMMCSNAME, clName );

   //create higher layer field
   createCL( clName, "a" );

   //create lower layer field
   createCL( clName, "a.aa.aaa" );

   //create same layer field
   commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: [{ Field: "a.aa" }, { Field: "a.bb" }] } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a.aa_SEQ", "SYS_" + clID + "_a.bb_SEQ"];
   var expArr = [{ Field: "a.aa", SequenceName: sequenceNames[0] }, { Field: "a.bb", SequenceName: sequenceNames[1] }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], {} );
   }

   commDropCL( db, COMMCSNAME, clName );
}

function createCL ( clName, field )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( COMMCSNAME ).createCL( clName, { AutoIncrement: [{ Field: "a.aa" }, { Field: field }] } );
   } );
}