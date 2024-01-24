/******************************************************************************
@Description :   seqDB-15992: 创建集合时，创建3个自增字段 
@Modify list :   2018-10-17    xiaoni Zhao  Init
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15992";

   commDropCL( db, COMMCSNAME, clName );

   var autoIncrementArray = new Array( { Field: "id1", Increment: 2 },
      { Field: "id2", Increment: 4 },
      { Field: "id3", Increment: 1 } );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: autoIncrementArray } );

   //check autoIncrement 
   var clID = getCLID( db,  COMMCSNAME, clName );
   var sequenceNames = ["SYS_" + clID + "_" + autoIncrementArray[0].Field + "_SEQ",
   "SYS_" + clID + "_" + autoIncrementArray[1].Field + "_SEQ",
   "SYS_" + clID + "_" + autoIncrementArray[2].Field + "_SEQ"];
   var expIncrements = [{ Field: autoIncrementArray[0].Field, SequenceName: sequenceNames[0] },
   { Field: autoIncrementArray[1].Field, SequenceName: sequenceNames[1] },
   { Field: autoIncrementArray[2].Field, SequenceName: sequenceNames[2] }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expIncrements );

   //check sequence
   var expSequences = [{ Increment: 2 },
   { Increment: 4 },
   { Increment: 1 }];
   for( var i in sequenceNames )
   {
      checkSequence( db, sequenceNames[i], expSequences[i] );
   }

   //insert records
   var rc = dbcl.find();
   var expRecs = new Array();
   for( var i = 0; i < 100; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( {
         "a": i, "id1": 1 + autoIncrementArray[0].Increment * i,
         "id2": 1 + autoIncrementArray[1].Increment * i,
         "id3": 1 + autoIncrementArray[2].Increment * i
      } );
   }
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

