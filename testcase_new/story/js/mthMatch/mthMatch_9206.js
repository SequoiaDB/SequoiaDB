/************************************
*@Description: find condition type is arr,key exists $ ,find condition include arr/obj/regex/other,use and
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
   var doc = [{ No: 1, a: "String", b: 1, c: { d: 1 }, e: -200, f: "string" },
   { No: 2, a: "String", b: 1, c: { d: 1 }, e: 200, f: "string" },
   { No: 3, a: "aString", b: 1, c: { d: 1 }, e: 200, f: "string" },
   { No: 4, a: "string", b: 2, c: { d: 1 }, e: 200, f: "string" },
   { No: 5, a: "String", b: 1, c: { h: 1 }, e: -200, f: "string" },
   { No: 6, a: "String", b: 1, c: { d: 1 }, e: -20, f: "string" },
   { No: 7, a: "String", b: 1, c: { d: 1 }, e: -200, f: "strings" }];
   dbcl.insert( doc );

   //seqDB-9206
   var findCondition1 = { $and: [{ a: { $regex: '^string$', $options: 'i' } }, { b: 1 }, { c: { d: 1 } }, { e: { $abs: 1, $et: 200 } }, { $and: [{ f: { $type: 1, $et: 2 } }, { c: { $type: 1, $et: 3 } }] }] };
   var expRecs1 = [{ No: 1, a: "String", b: 1, c: { d: 1 }, e: -200, f: "string" },
   { No: 2, a: "String", b: 1, c: { d: 1 }, e: 200, f: "string" },
   { No: 7, a: "String", b: 1, c: { d: 1 }, e: -200, f: "strings" }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ b: { $et: 1 } }, { "c": { $et: { d: 1 } } }, { c: { $type: 1, $et: 3 } }, { e: { $abs: 1, $et: 200 } }, { f: { $type: 1, $et: 2 } }, { a: { $regex: "^string$", $options: "i" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );
}

