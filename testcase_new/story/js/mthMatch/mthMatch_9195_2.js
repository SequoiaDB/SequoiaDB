/************************************
*@Description: type as function
*@author:      zhaoyu
*@createdate:  2016.11.1
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: ["1", "2", "3"] },
   { No: 3, a: ["1", 2, 3] },
   { No: 4, a: { 0: 1, 1: 2, 2: 3 } },
   { No: 5, a: { 0: "1", 1: "2", 2: "3" } },
   { No: 6, a: { 0: 1, 1: "2", 2: "3" } }];
   dbcl.insert( doc );

   var findCondition1 = { "a.$0": { $type: 1, $et: 2 } };
   var expRecs1 = [{ No: 2, a: ["1", "2", "3"] },
   { No: 3, a: ["1", 2, 3] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );
}
