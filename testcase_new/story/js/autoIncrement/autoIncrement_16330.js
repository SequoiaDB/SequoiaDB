/******************************************************************************
@Description :   seqDB-16330:  MaxValue字段参数校验  
@Modify list :   2018-11-13    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16330";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a1" } } );

   var options = [{ Field: "a2", MaxValue: 2000, MinValue: 3000 },
   { Field: "a3", MaxValue: 2000, MinValue: 2000 },
   { Field: "a5", MaxValue: { "$numberLong": "-9223372036854775808" }, MinValue: { "$numberLong": "-9223372036854775860" } }, { Field: "a6", MaxValue: { "$numberLong": "-9223372036854775809" }, MinValue: { "$numberLong": "-9223372036854775860" } }, { Field: "a9", MaxValue: 20.55 },
   { Field: "a10", MaxValue: { "$decimal": "123.456" } }];
   for( var i = 0; i < options.length; i++ )
   {
      create( dbcl, options[i], false );
   }

   dbcl.createAutoIncrement( [{ Field: "a4", MaxValue: 5000 },
   { Field: "a7", MaxValue: { "$numberLong": "9223372036854775807" } },
   { Field: "a8", MaxValue: { "$numberLong": "9223372036854775808" } }] );

   //check Sequence
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a1_SEQ",
   "SYS_" + clID + "_a4_SEQ",
   "SYS_" + clID + "_a7_SEQ",
   "SYS_" + clID + "_a8_SEQ"];
   var expSequences = [{},
   { MaxValue: 5000 },
   { MaxValue: { "$numberLong": "9223372036854775807" } },
   { MaxValue: { "$numberLong": "9223372036854775807" } }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expSequences[i] );
   }

   var expRecs = [];
   for( var i = 0; i < 50; i++ )
   {
      dbcl.insert( { a: i } );
      expRecs.push( { "a": i, "a1": 1 + i, "a4": 1 + i, "a7": 1 + i, "a8": 1 + i } );
   }

   var rc = dbcl.find().sort( { "a1": 1 } );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

