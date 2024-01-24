/************************************
*@Description: many fields use abs to match,
*@author:      zhaoyu
*@createdate:  2016.10.12
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
   var doc = [{ No: 1, a: 1, b: 1, c: 1 },
   { No: 2, a: 2, b: 2, c: 2 },
   { No: 3, a: -1, b: -1, c: -1 },
   { No: 4, a: -2, b: -2, c: -2 },
   { No: 5, a: [1, 3], b: [1, 3], c: [1, 3] },
   { No: 6, a: [2, 3], b: [2, 3], c: [2, 3] },
   { No: 7, a: [-1, -3], b: [-1, -3], c: [-1, -3] },
   { No: 8, a: [-2, -3], b: [-2, -3], c: [-2, -3] },
   { No: 9, a: [{ 0: 1 }, { 1: 3 }], b: [{ 0: 1 }, { 1: 3 }], c: [{ 0: 1 }, { 1: 3 }] },
   { No: 10, a: [{ 0: 2 }, { 1: 3 }], b: [{ 0: 2 }, { 1: 3 }], c: [{ 0: 2 }, { 1: 3 }] },
   { No: 11, a: [{ 0: -1 }, { 1: -3 }], b: [{ 0: -1 }, { 1: -3 }], c: [{ 0: -1 }, { 1: -3 }] },
   { No: 12, a: [{ 0: -2 }, { 1: -3 }], b: [{ 0: -2 }, { 1: -3 }], c: [{ 0: -2 }, { 1: -3 }] },
   { No: 13, a: { 0: 1, 1: 3 }, b: { 0: 1, 1: 3 }, c: { 0: 1, 1: 3 } },
   { No: 14, a: { 0: 2, 1: 3 }, b: { 0: 2, 1: 3 }, c: { 0: 2, 1: 3 } },
   { No: 15, a: { 0: -1, 1: -3 }, b: { 0: -1, 1: -3 }, c: { 0: -1, 1: -3 } },
   { No: 16, a: { 0: -2, 1: -3 }, b: { 0: -2, 1: -3 }, c: { 0: -2, 1: -3 } }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $abs: 1, $et: 1 }, b: { $abs: 1, $et: 1 }, c: { $abs: 1, $et: 1 } };
   var expRecs1 = [{ No: 1, a: 1, b: 1, c: 1 },
   { No: 3, a: -1, b: -1, c: -1 },
   { No: 5, a: [1, 3], b: [1, 3], c: [1, 3] },
   { No: 7, a: [-1, -3], b: [-1, -3], c: [-1, -3] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { a: { $abs: 1, $et: 1 }, b: { $abs: 1, $et: 2 }, c: { $abs: 1, $et: 1 } };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { a: { $abs: 1, $et: 1 }, b: { $abs: 1, $et: 1 }, d: { $abs: 1, $et: 1 } };
   var expRecs3 = [];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var findCondition4 = { "a.1": { $abs: 1, $et: 3 }, "b.0": { $abs: 1, $et: 1 }, "c.1": { $abs: 1, $et: 3 } };
   var expRecs4 = [{ No: 5, a: [1, 3], b: [1, 3], c: [1, 3] },
   { No: 7, a: [-1, -3], b: [-1, -3], c: [-1, -3] },
   { No: 9, a: [{ 0: 1 }, { 1: 3 }], b: [{ 0: 1 }, { 1: 3 }], c: [{ 0: 1 }, { 1: 3 }] },
   { No: 11, a: [{ 0: -1 }, { 1: -3 }], b: [{ 0: -1 }, { 1: -3 }], c: [{ 0: -1 }, { 1: -3 }] },
   { No: 13, a: { 0: 1, 1: 3 }, b: { 0: 1, 1: 3 }, c: { 0: 1, 1: 3 } },
   { No: 15, a: { 0: -1, 1: -3 }, b: { 0: -1, 1: -3 }, c: { 0: -1, 1: -3 } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var findCondition5 = { "a.1": { $abs: 1, $et: 4 }, "b.0": { $abs: 1, $et: 1 }, "c.1": { $abs: 1, $et: 3 } };
   var expRecs5 = [];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   var findCondition6 = { "a.1": { $abs: 1, $et: 3 }, "b.0": { $abs: 1, $et: 1 }, "d.1": { $abs: 1, $et: 3 } };
   var expRecs6 = [];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );
}
