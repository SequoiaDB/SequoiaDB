/************************************
*@Description:capped cl,records not filled one block, pop record direction set -1,then insert records
*@author:      zhaoyu
*@createdate:  2017.7.11
*@testlinkCase: seqDB-11798
**************************************/
main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11798";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, true, true );

   var rd = new commDataGenerator();
   var recordNum = 10000;
   var recs = rd.getRecords( recordNum, ["int", "string", "bool", "date",
      "binary", "regex", "null"], ['a'] );
   dbcl.insert( recs );

   checkRecords( dbcl, null, null, { _id: 1 }, null, null, recs );

   var lastLogicalID = getLogicalID( dbcl, null, null, { _id: -1 }, 1, null )[0];

   if( lastLogicalID < 33554396 )
   {
      dbcl.pop( { LogicalID: 0, Direction: -1 } );
   } else
   {
      throw new Error( "TEST_CONDITION_NOT_FULFILLED" );
   }

   var recordNumPop = countRecords( dbcl, null );
   assert.equal( recordNumPop, 0 );

   var rd = new commDataGenerator();
   var recordNum = 1000;
   var recs = rd.getRecords( recordNum, ["int", "string", "bool", "date",
      "binary", "regex", "null"], ['a'] );
   dbcl.insert( recs );

   var firstLogicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, null )[0];
   var lastLogicalID1 = getLogicalID( dbcl, null, null, { _id: -1 }, 1, null )[0];

   assert.equal( firstLogicalID, 0 )

   var recordNumInsert = countRecords( dbcl, null );

   assert.equal( recordNumInsert, recordNum )

   checkRecords( dbcl, null, null, { _id: 1 }, null, null, recs );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}