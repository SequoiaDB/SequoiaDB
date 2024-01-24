/************************************
*@Description: find condition type is obj,key Non-exists $ and feildName type is other 
*@author:      zhaoyu
*@createdate:  2016.10.24
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
   var doc = [{ No: 1, a: { b: 1, c: 1 }, d: { b: 1, c: 1 } },
   { No: 2, a: { b: 2, c: 2 }, d: { b: 2, c: 2 } },
   { No: 3, a: { b: 3, c: 3 }, d: { b: 3, c: 3 } },
   { No: 4, a: { b: 4, c: 4 }, d: { b: 4, c: 4 } }];
   dbcl.insert( doc );

   //seqDB-9192
   var findCondition1 = { a: { b: 1, c: 1 } };
   var expRecs1 = [{ No: 1, a: { b: 1, c: 1 }, d: { b: 1, c: 1 } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      UseExtSort: false,
      Query: { $and: [{ a: { $et: { b: 1, c: 1 } } }] },
      IXBound: { a: [[{ b: 1, c: 1 }, { b: 1, c: 1 }]] },
      NeedMatch: false
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   var findCondition2 = { d: { b: 1, c: 1 } };
   var expRecs2 = [{ No: 1, a: { b: 1, c: 1 }, d: { b: 1, c: 1 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   explainExpRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ d: { $et: { b: 1, c: 1 } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, null, explainExpRecs2 );

   //feildName exists $ or .
   dbcl.remove();
   //insert data 
   var doc = [{ No: 1, a: [{ b: 1, c: 1 }], d: [{ b: 1, c: 1 }] },
   { No: 2, a: [{ b: 2, c: 2 }], d: [{ b: 2, c: 2 }] },
   { No: 3, a: [{ b: 3, c: 3 }], d: [{ b: 3, c: 3 }] },
   { No: 4, a: [{ b: 4, c: 4 }], d: [{ b: 4, c: 4 }] }];
   dbcl.insert( doc );

   //seqDB-9192
   var findCondition1 = { "a.$0": { b: 1, c: 1 } };
   var expRecs1 = [{ No: 1, a: [{ b: 1, c: 1 }], d: [{ b: 1, c: 1 }] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      UseExtSort: false,
      Query: { $and: [{ "a.$0": { $et: { b: 1, c: 1 } } }] },
      IXBound: { a: [[{ b: 1, c: 1 }, { b: 1, c: 1 }]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   var findCondition2 = { "d.$0": { b: 1, c: 1 } };
   var expRecs2 = [{ No: 1, a: [{ b: 1, c: 1 }], d: [{ b: 1, c: 1 }] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   explainExpRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "d.$0": { $et: { b: 1, c: 1 } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, null, explainExpRecs2 );

   var findCondition3 = { "a.0": { b: 1, c: 1 } };
   var expRecs3 = [{ No: 1, a: [{ b: 1, c: 1 }], d: [{ b: 1, c: 1 }] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "a.0": { $et: { b: 1, c: 1 } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );

   var findCondition4 = { "d.0": { b: 1, c: 1 } };
   var expRecs4 = [{ No: 1, a: [{ b: 1, c: 1 }], d: [{ b: 1, c: 1 }] }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   explainExpRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "d.0": { $et: { b: 1, c: 1 } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, null, explainExpRecs4 );
}
