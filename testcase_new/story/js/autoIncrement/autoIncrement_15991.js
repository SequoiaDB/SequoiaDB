/******************************************************************************
@Description :   seqDB-15991: 创建集合时，创建1个自增字段 
@Modify list :   2018-10-17    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15991";
   var field = "id1";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expArr = [{ Field: field, SequenceName: sequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );
   checkSequence( db, sequenceName, {} );

   dbcl.insert( { a: 1 } );

   var rc = dbcl.find();
   var expRecs = [{ "id1": 1, "a": 1 }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

