/************************************
*@Description: many fields use mod to match,
*@author:      zhaoyu
*@createdate:  2016.10.13
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
   var doc = [{ No: 1, a: 12, b: -25 },
   { No: 2, a: -12, b: -25 },
   { No: 3, a: [12, 25], b: [-12, -25] },
   { No: 4, a: [-12, -25], b: [12, 25] },
   { No: 5, a: [{ 0: 12 }, { 1: 25 }], b: [{ 0: -12 }, { 1: -25 }] },
   { No: 6, a: [{ 0: -12 }, { 1: -25 }], b: [{ 0: 12 }, { 1: 25 }] },
   { No: 7, a: { 0: 12, 1: 25 }, b: { 0: -12, 1: -25 } },
   { No: 8, a: { 0: -12, 1: -25 }, b: { 0: 12, 1: 25 } }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $mod: 5, $et: 2 }, b: { $mod: -2, $et: -1 } };
   var expRecs1 = [{ No: 1, a: 12, b: -25 },
   { No: 3, a: [12, 25], b: [-12, -25] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { a: { $mod: { $numberLong: "5" }, $et: { $decimal: "2", $precision: [100, 2] } }, b: { $mod: -2, $et: -1 }, c: { $mod: 1, $et: 1 } };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { "a.0": { $mod: 5, $et: 2 }, "b.1": { $mod: -2, $et: -1 } };
   var expRecs3 = [{ No: 3, a: [12, 25], b: [-12, -25] },
   { No: 5, a: [{ 0: 12 }, { 1: 25 }], b: [{ 0: -12 }, { 1: -25 }] },
   { No: 7, a: { 0: 12, 1: 25 }, b: { 0: -12, 1: -25 } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var findCondition4 = { "a.0": { $mod: 5, $et: 2 }, "b.1": { $mod: -2, $et: -1 }, "c.0": { $mod: 1, $et: -1 } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );
}
