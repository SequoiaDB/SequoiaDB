/************************************
*@Description: and/or/not combaniation use numberical functions
*@author:      zhaoyu
*@createdate:  2016.10.25
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

   //insert data 
   var doc = [{ No: 1, a: 8, b: 26 },
   { No: 2, a: 9, b: 27 },
   { No: 3, a: 26, b: 25 },
   { No: 4, a: 27, b: 24 },
   { No: 5, b: 8 },
   { No: 6, b: 9 },
   { No: 7, b: 26 },
   { No: 8, b: 27 }];
   dbcl.insert( doc );

   //seqDB-10322
   var findCondition1 = { $and: [{ a: { $add: 2, $gt: 10 } }, { a: { $subtract: 7, $lt: 20 } }] };
   var expRecs1 = [{ No: 2, a: 9, b: 27 },
   { No: 3, a: 26, b: 25 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $subtract: 7, $lt: 20 } }, { a: { $add: 2, $gt: 10 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   var findCondition2 = { a: { $add: 2, $gt: 10, $subtract: 7, $lt: 20 } };
   var expRecs2 = [{ No: 2, a: 9, b: 27 },
   { No: 3, a: 26, b: 25 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { $and: [{ a: { $add: 2, $gt: 10 } }, { b: { $subtract: 7, $lt: 20 } }] };
   var expRecs3 = [{ No: 3, a: 26, b: 25 },
   { No: 4, a: 27, b: 24 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   //seqDB-10323
   var findCondition4 = { $or: [{ a: { $add: 2, $lt: 11 } }, { a: { $subtract: 7, $gt: 19 } }] };
   var expRecs4 = [{ No: 1, a: 8, b: 26 },
   { No: 4, a: 27, b: 24 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var findCondition5 = { $or: [{ a: { $add: 2, $gt: 10 } }, { b: { $subtract: 7, $lt: 20 } }] };
   var expRecs5 = [{ No: 1, a: 8, b: 26 },
   { No: 2, a: 9, b: 27 },
   { No: 3, a: 26, b: 25 },
   { No: 4, a: 27, b: 24 },
   { No: 5, b: 8 },
   { No: 6, b: 9 },
   { No: 7, b: 26 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   //seqDB-10324
   var findCondition6 = { $not: [{ a: { $add: 2, $gt: 10 } }, { a: { $subtract: 7, $lt: 20 } }] };
   var expRecs6 = [{ No: 1, a: 8, b: 26 },
   { No: 4, a: 27, b: 24 },
   { No: 5, b: 8 },
   { No: 6, b: 9 },
   { No: 7, b: 26 },
   { No: 8, b: 27 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );

   var findCondition7 = { $not: [{ a: { $add: 2, $gt: 10 } }, { b: { $subtract: 7, $lt: 20 } }] };
   var expRecs7 = [{ No: 1, a: 8, b: 26 },
   { No: 2, a: 9, b: 27 },
   { No: 5, b: 8 },
   { No: 6, b: 9 },
   { No: 7, b: 26 },
   { No: 8, b: 27 }];
   checkResult( dbcl, findCondition7, null, expRecs7, { _id: 1 } );
}

