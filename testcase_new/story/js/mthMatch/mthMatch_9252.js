/************************************
*@Description: index scan,use gt/gte/lt/lte to comapare,
               type is arr/obj,value set typical data;
               use arr/obj name or arr/obj element to compare
*@author:      zhaoyu
*@createdate:  2016.8.6
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );
   commCreateIndex( dbcl, "a1", { "a.1": 1 } );

   hintCondition = { '': '' };

   //insert data 
   var doc = [{ No: 1, a: [-1, -2, -3] },
   { No: 2, a: [1, 2, 3] },
   { No: 3, a: [2, 3, 4] },
   { No: 4, a: [3, 4, 5] },
   { No: 5, a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { No: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { No: 7, a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { No: 8, a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { No: 9, a: { 0: -1, 1: -2, 2: -3 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3 } },
   { No: 11, a: { 0: 2, 1: 3, 2: 4 } },
   { No: 12, a: { 0: 3, 1: 4, 2: 5 } },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: { $date: "2000-01-01" } },
   { No: 15, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
   { No: 16, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 17, a: { $regex: "^z", "$options": "i" } },
   { No: 18, a: null },
   { No: 19, a: "abc" },
   { No: 20, a: MinKey() },
   { No: 21, a: MaxKey() },
   { No: 22, a: true },
   { No: 23, a: false }];
   dbcl.insert( doc );

   //arr or obj name query
   //$gt:2
   var findCondition1 = { a: { $gt: 2 } };
   var expRecs1 = [{ No: 2, a: [1, 2, 3] },
   { No: 3, a: [2, 3, 4] },
   { No: 4, a: [3, 4, 5] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //$gte:2
   var findCondition2 = { a: { $gte: 2 } };
   var expRecs2 = [{ No: 2, a: [1, 2, 3] },
   { No: 3, a: [2, 3, 4] },
   { No: 4, a: [3, 4, 5] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //$lt:2
   var findCondition3 = { a: { $lt: 2 } };
   var expRecs3 = [{ No: 1, a: [-1, -2, -3] },
   { No: 2, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //$lte:2
   var findCondition4 = { a: { $lte: 2 } };
   var expRecs4 = [{ No: 1, a: [-1, -2, -3] },
   { No: 2, a: [1, 2, 3] },
   { No: 3, a: [2, 3, 4] }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //arr or obj element query
   //$gt:2
   var findCondition5 = { "a.1": { $gt: 2 } };
   var expRecs5 = [{ No: 7, a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { No: 8, a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { No: 11, a: { 0: 2, 1: 3, 2: 4 } },
   { No: 12, a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //$gte:2
   var findCondition6 = { "a.1": { $gte: 2 } };
   var expRecs6 = [{ No: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { No: 7, a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { No: 8, a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { No: 10, a: { 0: 1, 1: 2, 2: 3 } },
   { No: 11, a: { 0: 2, 1: 3, 2: 4 } },
   { No: 12, a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   //$lt:2
   var findCondition7 = { "a.1": { $lt: 2 } };
   var expRecs7 = [{ No: 5, a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { No: 9, a: { 0: -1, 1: -2, 2: -3 } }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //$lte:2
   var findCondition8 = { "a.1": { $lte: 2 } };
   var expRecs8 = [{ No: 5, a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { No: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { No: 9, a: { 0: -1, 1: -2, 2: -3 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );
}

