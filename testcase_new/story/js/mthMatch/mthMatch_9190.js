/************************************
*@Description: find condition type is obj,key exists $ and feildName type is regex 
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
   var doc = [{ No: 1, a: "string" },
   { No: 2, a: "String" },
   { No: 3, a: "astring" },
   { No: 4, a: "strings" }];
   dbcl.insert( doc );

   //seqDB-9190
   var findCondition1 = { a: { $regex: '^string$', $options: 'i' } };
   var expRecs1 = [{ No: 1, a: "string" },
   { No: 2, a: "String" }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ a: { $regex: "^string$", $options: "i" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   var findCondition2 = { a: { $regex: '^string$', $options: 'i', $et: "string" } };
   var expRecs2 = [{ No: 1, a: "string" }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   explainExpRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      UseExtSort: false,
      Query: { $and: [{ a: { $et: "string" } }, { a: { $regex: "^string$", $options: "i" } }] },
      IXBound: { a: [["string", "string"]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, null, explainExpRecs2 );

   var findCondition3 = { a: { $regex: '^string$', $options: 'i', b: "string" } };
   InvalidArgCheck( dbcl, findCondition3, null, SDB_INVALIDARG );

   //feildName exists $ or .
   dbcl.remove();
   //insert data 
   var doc = [{ No: 1, a: ["string", { b: "string" }] },
   { No: 2, a: ["String", { b: "String" }] },
   { No: 3, a: ["astring", { b: "astring" }] },
   { No: 4, a: ["strings", { b: "strings" }] }];
   dbcl.insert( doc );

   //seqDB-9190
   var findCondition1 = { "a.$1": { $regex: '^string$', $options: 'i' } };
   var expRecs1 = [{ No: 1, a: ["string", { b: "string" }] },
   { No: 2, a: ["String", { b: "String" }] },];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "a.$1": { $regex: "^string$", $options: "i" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   var findCondition2 = { "a.$1": { $regex: '^string$', $options: 'i', $et: "string" } };
   var expRecs2 = [{ No: 1, a: ["string", { b: "string" }] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   explainExpRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "a",
      UseExtSort: false,
      Query: { $and: [{ "a.$1": { $et: "string" } }, { "a.$1": { $regex: "^string$", $options: "i" } }] },
      IXBound: { a: [["string", "string"]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, null, explainExpRecs2 );

   var findCondition3 = { "a.$1": { $regex: '^string$', $options: 'i', b: "string" } };
   InvalidArgCheck( dbcl, findCondition3, null, SDB_INVALIDARG );

   var findCondition4 = { "a.0": { $regex: '^string$', $options: 'i' } };
   var expRecs4 = [{ No: 1, a: ["string", { b: "string" }] },
   { No: 2, a: ["String", { b: "String" }] },];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   explainExpRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "a.0": { $regex: "^string$", $options: "i" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, null, explainExpRecs4 );

   var findCondition5 = { "a.0": { $regex: '^string$', $options: 'i', $et: "string" } };
   var expRecs5 = [{ No: 1, a: ["string", { b: "string" }] }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   explainExpRecs5 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ "a.0": { $et: "string" } }, { "a.0": { $regex: "^string$", $options: "i" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition5, null, null, explainExpRecs5 );

   var findCondition6 = { "a.0": { $regex: '^string$', $options: 'i', b: "string" } };
   InvalidArgCheck( dbcl, findCondition6, null, -6 );

}
