/************************************
*@Description: arr use returnMatch
*@author:      zhaoyu
*@createdate:  2016.10.25
*@testlinkCase: seqDB-10315/seqDB-10316/seqDB-10317/seqDB-10318
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
   { No: 2, a: [1, 2, 3, 4] },
   { No: 3, a: [1, 2, 3, 10, 12] },
   { No: 4, a: [1, 2, 3, 4, 10, 13] }];
   dbcl.insert( doc );

   //seqDB-10317
   var findCondition1 = { a: { $expand: 1, $size: 1, $et: 4 } };
   var expRecs1 = [{ No: 2, a: 1 }, { No: 2, a: 2 }, { No: 2, a: 3 }, { No: 2, a: 4 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $size: 1, $et: 4 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   var findCondition2 = { a: { $returnMatch: 2, $size: 1, $et: 4 } };
   var expRecs2 = [{ No: 2, a: [1, 2, 3, 4] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var explainRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [2, -1], $size: 1, $et: 4 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, { No: 1 }, explainRecs2 );

   //seqDB-10316
   var findCondition3 = { "a.$0": { $expand: 1, $et: 4 } };
   expRecs3 = [{ No: 2, a: [1, 2, 3, 4] },
   { No: 4, a: [1, 2, 3, 4, 10, 13] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var explainRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      Query: { $and: [{ "a.$0": { $expand: 1, $et: 4 } }] },
      IXBound: { a: [[4, 4]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, explainRecs3 );

   var findCondition4 = { "a.$0": { $returnMatch: 2, $et: 4 } };
   expRecs4 = [{ No: 2, a: [1, 2, 3, 4] },
   { No: 4, a: [1, 2, 3, 4, 10, 13] }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var explainRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      Query: { $and: [{ "a.$0": { $returnMatch: [2, -1], $et: 4 } }] },
      IXBound: { a: [[4, 4]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, { No: 1 }, explainRecs4 );

   //seqDB-10315
   //SEQUOIADBMAINSTREAM-2026
   var findCondition5 = { a: { $returnMatch: 0, $all: [4, 3] } };
   var expRecs5 = [{ No: 2, a: [3, 4] },
   { No: 4, a: [3, 4] }];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   var findCondition6 = { a: { $expand: 1, $all: [4, 3] } };
   var expRecs6 = [{ No: 2, a: 1 }, { No: 2, a: 2 }, { No: 2, a: 3 }, { No: 2, a: 4 },
   { No: 4, a: 1 }, { No: 4, a: 2 }, { No: 4, a: 3 }, { No: 4, a: 4 }, { No: 4, a: 10 }, { No: 4, a: 13 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );

   var explainRecs6 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      Query: { $and: [{ a: { $expand: 1, $all: [4, 3] } }] },
      IXBound: { a: [[4, 4]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition6, null, { No: 1 }, explainRecs6 );

   var findCondition7 = { a: { $returnMatch: [2, 3], $nin: [4, 12] } };
   var expRecs7 = [{ No: 1, a: [3] }];
   checkResult( dbcl, findCondition7, null, expRecs7, { _id: 1 } );

   var explainRecs7 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [2, 3], $nin: [4, 12] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition7, null, { No: 1 }, explainRecs7 );

   var findCondition8 = { a: { $expand: 1, $nin: [4, 12] } };
   var expRecs8 = [{ No: 1, a: 1 }, { No: 1, a: 2 }, { No: 1, a: 3 }];
   checkResult( dbcl, findCondition8, null, expRecs8, { _id: 1 } );

   var explainRecs8 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $nin: [4, 12] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition8, null, { No: 1 }, explainRecs8 );

   //seqDB-10318
   var findCondition9 = { a: { $returnMatch: [1, 3], $mod: [2, 0] } };
   var expRecs9 = [{ No: 1, a: null },
   { No: 2, a: [4] },
   { No: 3, a: [10, 12] },
   { No: 4, a: [4, 10] }];
   checkResult( dbcl, findCondition9, null, expRecs9, { _id: 1 } );

   var explainRecs9 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [1, 3], $mod: [2, 0] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition9, null, { No: 1 }, explainRecs9 );

   var findCondition10 = { a: { $expand: 1, $mod: [5, 4] } };
   var expRecs10 = [{ No: 2, a: 1 }, { No: 2, a: 2 }, { No: 2, a: 3 }, { No: 2, a: 4 },
   { No: 4, a: 1 }, { No: 4, a: 2 }, { No: 4, a: 3 }, { No: 4, a: 4 }, { No: 4, a: 10 }, { No: 4, a: 13 }];
   checkResult( dbcl, findCondition10, null, expRecs10, { _id: 1 } );

   var explainRecs10 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $mod: [5, 4] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition10, null, { No: 1 }, explainRecs10 );

   var findCondition11 = { a: { $returnMatch: 0, $gt: 10 } };
   var expRecs11 = [{ No: 3, a: [12] },
   { No: 4, a: [13] }];
   checkResult( dbcl, findCondition11, null, expRecs11, { _id: 1 } );

   var explainRecs11 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $returnMatch: [0, -1], $gt: 10 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition11, null, { No: 1 }, explainRecs11 );

   var findCondition12 = { a: { $expand: 1, $gt: 10 } };
   var expRecs12 = [{ No: 3, a: 1 }, { No: 3, a: 2 }, { No: 3, a: 3 }, { No: 3, a: 10 }, { No: 3, a: 12 },
   { No: 4, a: 1 }, { No: 4, a: 2 }, { No: 4, a: 3 }, { No: 4, a: 4 }, { No: 4, a: 10 }, { No: 4, a: 13 }];
   checkResult( dbcl, findCondition12, null, expRecs12, { _id: 1 } );

   var explainRecs12 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $expand: 1, $gt: 10 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition12, null, { No: 1 }, explainRecs12 );
}

