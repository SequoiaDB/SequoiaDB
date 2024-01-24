/************************************
*@Description: size as function
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

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert data 
   var doc = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   dbcl.insert( doc );

   //gt
   var findCondition1 = { a: { $size: 1, $gt: 0 }, b: { $size: 1, $mod: [2, 1] } };
   var expRecs1 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $gt: 0 } }, { b: { $size: 1, $mod: [2, 1] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   //use in/nin/all
   var findCondition2 = { a: { $size: 1, $in: [0, 1, 10, 2, 3, 4, 5] }, b: { $size: 1, $in: [0, 2, 4] } };
   var expRecs2 = [{ No: 2, a: [1, 2], b: [1, 2] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var explainRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $in: [0, 1, 10, 2, 3, 4, 5] } }, { b: { $size: 1, $in: [0, 2, 4] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, { No: 1 }, explainRecs2 );

   var findCondition3 = { a: { $size: 1, $nin: [7, 8, 2] }, b: { $size: 1, $nin: [5, 3, -1] } };
   var expRecs3 = [{ No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var explainRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $nin: [7, 8, 2] } }, { b: { $size: 1, $nin: [5, 3, -1] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, explainRecs3 );

   var findCondition4 = { a: { $size: 1, $all: [16, 2] } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var explainRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $all: [16, 2] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, { No: 1 }, explainRecs4 );

   //exists/isnull
   var findCondition5 = { a: { $size: 1, $exists: 1 } };
   var expRecs5 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var explainRecs5 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $exists: 1 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition5, null, { No: 1 }, explainRecs5 );

   var findCondition6 = { a: { $size: 1, $isnull: 0 } };
   var expRecs6 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   var explainRecs6 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $size: 1, $isnull: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition6, null, { No: 1 }, explainRecs6 );


}
