/******************************************************************************
@Description :   seqDB-16035:  Field字段参数校验 
@Modify list :   2018-10-23    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16035";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id1" } } );

   //illegal Field value
   var fields = [null, { Field: "$id2" }, { Field: " id3" }, { Field: 5 }];
   for( var i = 0; i < fields.length; i++ )
   {
      create( dbcl, fields[i], false );
   }

   //legal Field value
   fields = [{ Field: "id6" }, { Field: "id$7" }, { Field: "id 8" }, { Field: "id9" }, { Field: "id" }, { Field: "id99" }];
   for( var i = 0; i < fields.length; i++ )
   {
      create( dbcl, fields[i], true );
   }

   //check autoIncrement count
   var cursor = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var count = cursor.current().toObj().AutoIncrement.length;
   if( count !== 7 )
   {
      throw new Error( "Expect count is 7, but act count is " + count );
   }

   //check autoIncrement value
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_id1_SEQ",
   "SYS_" + clID + "_id6_SEQ",
   "SYS_" + clID + "_id$7_SEQ",
   "SYS_" + clID + "_id 8_SEQ",
   "SYS_" + clID + "_id9_SEQ",
   "SYS_" + clID + "_id_SEQ",
   "SYS_" + clID + "_id99_SEQ"];
   var expIncrements = [{ Field: "id1", SequenceName: sequenceNames[0] },
   { Field: "id6", SequenceName: sequenceNames[1] },
   { Field: "id$7", SequenceName: sequenceNames[2] },
   { Field: "id 8", SequenceName: sequenceNames[3] },
   { Field: "id9", SequenceName: sequenceNames[4] },
   { Field: "id", SequenceName: sequenceNames[5] },
   { Field: "id99", SequenceName: sequenceNames[6] }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expIncrements );

   //insert records and check
   dbcl.insert( { "a": 1 } );

   var rc = dbcl.find();
   var expRecs = [{ "a": 1, "id1": 1, "id6": 1, "id$7": 1, "id 8": 1, "id": 1, "id9": 1, "id99": 1 }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

