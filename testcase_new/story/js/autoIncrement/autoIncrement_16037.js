/******************************************************************************
@Description :   seqDB-16037:  StartValue字段参数校验 
@Modify list :   2018-10-23    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16037";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create autoIncrement
   dbcl.createAutoIncrement( [{ Field: "a1" },
   { Field: "a4", StartValue: 5 },
   { Field: "a5", StartValue: { "$numberLong": "-9223372036854775808" }, Increment: -1 },
   { Field: "a6", StartValue: { "$numberLong": "9223372036854775807" } },
   { Field: "a7", StartValue: { "$numberLong": "9223372036854775809" } },
   { Field: "a8", StartValue: { "$numberLong": "-9223372036854775809" }, Increment: -1 },
   { Field: "a9", StartValue: 2, MinValue: 2 },
   { Field: "a10", StartValue: 10000, MaxValue: 10000 }] );


   //StartValue is illegal 
   var options = [{ Field: "a2", StartValue: 2, MinValue: 5 },
   { Field: "a3", StartValue: 20000, MaxValue: 10000 },
   { Field: "a11", StartValue: 123.4 },
   { Field: "a12", StartValue: { "$decimal": "123.456" } }];
   for( var i = 0; i < options.length; i++ )
   {
      create( dbcl, options[i], false );
   }

   //check Sequence
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a1_SEQ",
   "SYS_" + clID + "_a4_SEQ",
   "SYS_" + clID + "_a5_SEQ",
   "SYS_" + clID + "_a6_SEQ",
   "SYS_" + clID + "_a7_SEQ",
   "SYS_" + clID + "_a8_SEQ",
   "SYS_" + clID + "_a9_SEQ",
   "SYS_" + clID + "_a10_SEQ"];
   var expSequences = [{},
   { StartValue: 5, CurrentValue: 5 },
   {
      Increment: -1, StartValue: { "$numberLong": "-9223372036854775808" },
      CurrentValue: { "$numberLong": "-9223372036854775808" }, MaxValue: -1,
      MinValue: { "$numberLong": "-9223372036854775808" }
   },
   {
      StartValue: { "$numberLong": "9223372036854775807" },
      CurrentValue: { "$numberLong": "9223372036854775807" }
   },
   {
      StartValue: { "$numberLong": "9223372036854775807" },
      CurrentValue: { "$numberLong": "9223372036854775807" }
   },
   {
      Increment: -1, StartValue: { "$numberLong": "-9223372036854775808" },
      CurrentValue: { "$numberLong": "-9223372036854775808" }, MaxValue: -1,
      MinValue: { "$numberLong": "-9223372036854775808" }
   },
   { StartValue: 2, CurrentValue: 2, MinValue: 2 },
   { StartValue: 10000, CurrentValue: 10000, MaxValue: 10000 }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expSequences[i] );
   }

   //insert records
   dbcl.insert( { "q": 1 } );
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      dbcl.insert( { "q": 2 } );
   } );

   //check records
   var rc = dbcl.find();
   var expRecs = [{
      "q": 1, "a1": 1, "a4": 5, "a5": { "$numberLong": "-9223372036854775808" },
      "a6": { "$numberLong": "9223372036854775807" }, "a7": { "$numberLong": "9223372036854775807" },
      "a8": { "$numberLong": "-9223372036854775808" }, "a9": 2, "a10": 10000
   }];
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

