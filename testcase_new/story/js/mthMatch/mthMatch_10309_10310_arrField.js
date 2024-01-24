/************************************
*@Description: use cast
*@author:      zhaoyu
*@createdate:  2016.7.18
*@testlinkCase:cover all testcast in testlink
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, a: [1, 2, 3], b: [-1, -2, -3] },
   { No: 2, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }], b: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { No: 3, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: -1, 1: -2, 2: -3 } }];
   dbcl.insert( doc );

   var condition1 = { a: { $cast: 100, $et: 1 }, b: { $cast: 100, $et: -1 } };
   var expect1 = [{ No: 1, a: [1, 2, 3], b: [-1, -2, -3] }];
   checkResult( dbcl, condition1, null, expect1, { _id: 1 } );

   var condition2 = { "a.0": { $cast: 100, $et: 1 }, "b.0": { $cast: 100, $et: -1 } };
   var expect2 = [{ No: 1, a: [1, 2, 3], b: [-1, -2, -3] },
   { No: 2, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }], b: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { No: 3, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: -1, 1: -2, 2: -3 } }];
   checkResult( dbcl, condition2, null, expect2, { _id: 1 } );

}

