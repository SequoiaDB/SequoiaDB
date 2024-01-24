/************************************
*@Description: arr use returnMatch
*@author:      zhaoyu
*@createdate:  2016.10.31
*@testlinkCase: seqDB-10318
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
   var doc = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: [3, 4, 5, 6] }];
   dbcl.insert( doc );

   //seqDB-10318
   var findCondition1 = { a: { $returnMatch: 0, $lt: 3 } };
   var expRecs1 = [{ No: 1, a: [1, 2] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $lt: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   var findCondition2 = { a: { $expand: 1, $lt: 3 } };
   var expRecs2 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var explainRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $lt: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, { No: 1 }, explainRecs2 );

   var findCondition3 = { a: { $returnMatch: 0, $lte: 3 } };
   var expRecs3 = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: [3] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var explainRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $lte: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, explainRecs3 );

   var findCondition4 = { a: { $expand: 1, $lte: 3 } };
   var expRecs4 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 },
   { No: 2, a: 3 },
   { No: 2, a: 4 },
   { No: 2, a: 5 },
   { No: 2, a: 6 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var explainRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $lte: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, { No: 1 }, explainRecs4 );

   var findCondition5 = { a: { $returnMatch: 0, $gte: 3 } };
   var expRecs5 = [{ No: 1, a: [3] },
   { No: 2, a: [3, 4, 5, 6] }];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   var explainRecs5 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $gte: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition5, null, { No: 1 }, explainRecs5 );

   var findCondition6 = { a: { $expand: 1, $gte: 3 } };
   var expRecs6 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 },
   { No: 2, a: 3 },
   { No: 2, a: 4 },
   { No: 2, a: 5 },
   { No: 2, a: 6 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );

   var explainRecs6 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $gte: 3 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition6, null, { No: 1 }, explainRecs6 );

   var findCondition7 = { a: { $returnMatch: 0, $ne: 4 } };
   var expRecs7 = [{ No: 1, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition7, null, expRecs7, { _id: 1 } );

   var explainRecs7 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $ne: 4 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition7, null, { No: 1 }, explainRecs7 );

   var findCondition8 = { a: { $expand: 1, $ne: 4 } };
   var expRecs8 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 }];
   checkResult( dbcl, findCondition8, null, expRecs8, { _id: 1 } );

   var explainRecs8 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $ne: 4 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition8, null, { No: 1 }, explainRecs8 );

   var findCondition9 = { a: { $returnMatch: 0, $isnull: 0 } };
   var expRecs9 = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: [3, 4, 5, 6] }];
   checkResult( dbcl, findCondition9, null, expRecs9, { _id: 1 } );

   var explainRecs9 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $isnull: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition9, null, { No: 1 }, explainRecs9 );

   var findCondition10 = { a: { $expand: 1, $isnull: 0 } };
   var expRecs10 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 },
   { No: 2, a: 3 },
   { No: 2, a: 4 },
   { No: 2, a: 5 },
   { No: 2, a: 6 }];
   checkResult( dbcl, findCondition10, null, expRecs10, { _id: 1 } );

   var explainRecs10 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $isnull: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition10, null, { No: 1 }, explainRecs10 );

   var findCondition11 = { a: { $returnMatch: 0, $exists: 1 } };
   var expRecs11 = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: [3, 4, 5, 6] }];
   checkResult( dbcl, findCondition11, null, expRecs11, { _id: 1 } );

   var explainRecs11 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $exists: 1 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition11, null, { No: 1 }, explainRecs11 );

   var findCondition12 = { a: { $expand: 1, $exists: 1 } };
   var expRecs12 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 },
   { No: 2, a: 3 },
   { No: 2, a: 4 },
   { No: 2, a: 5 },
   { No: 2, a: 6 }];
   checkResult( dbcl, findCondition12, null, expRecs12, { _id: 1 } );

   var explainRecs12 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $exists: 1 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition12, null, { No: 1 }, explainRecs12 );

   //drop Index
   commDropIndex( dbcl, "a" );

   //SEQUOIADBMAINSTREAM-2028
   var findCondition13 = { a: { $returnMatch: 1, $et: [1, 2, 3] } };
   var expRecs13 = [{ No: 1, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition13, null, expRecs13, { _id: 1 } );

   var findCondition14 = { a: { $expand: 1, $et: [1, 2, 3] } };
   var expRecs14 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 }];
   checkResult( dbcl, findCondition14, null, expRecs14, { _id: 1 } );

   var findCondition15 = { a: { $et: [1, 2, 3], $expand: 1 } };
   var expRecs15 = [{ No: 1, a: 1 },
   { No: 1, a: 2 },
   { No: 1, a: 3 }];
   checkResult( dbcl, findCondition15, null, expRecs15, { _id: 1 } );
}

