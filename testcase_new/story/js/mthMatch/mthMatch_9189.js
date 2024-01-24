/************************************
*@Description: use and/or/not, check explain
*@author:      zhaoyu
*@createdate:  2016.11.2
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
   var doc = [{ No: 1, a: 1 },
   { No: 2, a: 2 },
   { No: 3, a: 3 },
   { No: 4, a: 4 },
   { No: 5, a: 5 },
   { No: 6, a: 6 },
   { No: 7, a: 7 },
   { No: 8, a: 8 },
   { No: 9, a: 9 },
   { No: 10, a: 10 }];
   dbcl.insert( doc );

   //seqDB-9189
   var findCondition1 = { $and: [{ a: { $gt: 5 } }] };
   var expRecs1 = [{ No: 6, a: 6 },
   { No: 7, a: 7 },
   { No: 8, a: 8 },
   { No: 9, a: 9 },
   { No: 10, a: 10 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ a: { $gt: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   //seqDB-9210
   var findCondition2 = { $or: [{ a: { $lt: 5 } }] };
   var expRecs2 = [{ No: 1, a: 1 },
   { No: 2, a: 2 },
   { No: 3, a: 3 },
   { No: 4, a: 4 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   explainExpRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ a: { $lt: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, null, explainExpRecs2 );

   var findCondition3 = { $not: [{ a: { $ne: 5 } }] };
   var expRecs3 = [{ No: 5, a: 5 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $not: [{ a: { $ne: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );

   //drop index
   commDropIndex( dbcl, "a" );
   var findCondition4 = { $and: [{ a: { $gt: 5 } }] };
   var expRecs4 = [{ No: 6, a: 6 },
   { No: 7, a: 7 },
   { No: 8, a: 8 },
   { No: 9, a: 9 },
   { No: 10, a: 10 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   explainExpRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ a: { $gt: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, null, explainExpRecs4 );

   var findCondition5 = { $or: [{ a: { $lt: 5 } }] };
   var expRecs5 = [{ No: 1, a: 1 },
   { No: 2, a: 2 },
   { No: 3, a: 3 },
   { No: 4, a: 4 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   explainExpRecs5 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ a: { $lt: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition5, null, null, explainExpRecs5 );

   var findCondition3 = { $not: [{ a: { $ne: 5 } }] };
   var expRecs3 = [{ No: 5, a: 5 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $not: [{ a: { $ne: 5 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );
}

