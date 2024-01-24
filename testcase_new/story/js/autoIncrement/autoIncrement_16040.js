/******************************************************************************
@Description :   seqDB-16040:  AcquireSize字段参数校验
@Modify list :   2018-10-25    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16040";

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a0" } } );

   dbcl.createAutoIncrement( [{ Field: "a1", AcquireSize: 2147483647, CacheSize: 2147483647 },
   { Field: "a2", AcquireSize: 2000, CacheSize: 2000 }] );

   var options = [{ Field: "a3", AcquireSize: -10000 },
   { Field: "a4", AcquireSize: 2147483648 },
   { Field: "a5", AcquireSize: 0 },
   { Field: "a6", AcquireSize: 123.4 },
   { Field: "a7", AcquireSize: { "$decimal": "123.456" } }];
   for( var i = 0; i < options.length; i++ )
   {
      create( dbcl, options[i], false );
   }

   //check Sequence
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_a0_SEQ",
   "SYS_" + clID + "_a1_SEQ",
   "SYS_" + clID + "_a2_SEQ"];
   var expSequences = [{},
   { AcquireSize: 2147483647, CacheSize: 2147483647 },
   { AcquireSize: 2000, CacheSize: 2000 }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expSequences[i] );
   }

   //insert records
   var coordNodes = getCoordNodeNames( db );
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[i] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a": i } );
      expRecs.push( { "a": i, "a0": 1 + i * 1000, "a1": 1 + i * 2147483647, "a2": 1 + i * 2000 } );
      coord.close();
   }

   var rc = dbcl.find();
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

