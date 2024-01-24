/******************************************************************************
@Description :   seqDB-15986:新增嵌套类型的自增字段
@Modify list :   2018-10-16    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15986";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { a: 1, b: 1 } );

   dbcl.createAutoIncrement( { Field: "a.aa" } );

   dbcl.createAutoIncrement( { Field: "b.bb.bbb" } );

   var options = [{ Field: "c.1" }];
   create( dbcl, options, false );

   //check autoIncrement
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a.aa_SEQ", "SYS_" + clID + "_b.bb.bbb_SEQ"];
   var expAotoIncrement = [{ Field: "a.aa", SequenceName: sequenceNames[0] },
   { Field: "b.bb.bbb", SequenceName: sequenceNames[1] }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expAotoIncrement );

   //check sequence
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], {} );
   }

   dbcl.dropAutoIncrement( ["a.aa", "b.bb.bbb"] );

   commDropCL( db, COMMCSNAME, clName );
}

