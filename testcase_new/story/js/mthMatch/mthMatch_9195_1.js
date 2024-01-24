/************************************
*@Description: type as function
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
   var doc = [{ No: 1, a: 123, b: { $numberLong: "9223372036854775807" }, c: 123.123, d: { $decimal: "123", $precision: [5, 2] } },
   { No: 2, a: "string", b: { $oid: "123abcd00ef12358902300ef" }, c: true, d: { $date: "2012-12-12" } },
   { No: 3, a: { $timestamp: "2012-01-01-13.14.26.124233" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, d: { $regex: '^string$', $options: 'i' } },
   { No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null },
   { No: 5, b: MinKey(), c: MaxKey() }];
   dbcl.insert( doc );

   //many field,some exists,some Non-exists,use gt/lt
   var findCondition1 = { a: { $type: 1, $gt: 2 }, b: { $type: 1, $lt: 17 }, c: { $type: 1, $mod: [2, 0] }, d: { $type: 1, $ne: "null" } };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $type: 1, $gt: 2 } }, { b: { $type: 1, $lt: 17 } }, { c: { $type: 1, $mod: [2, 0] } }, { d: { $type: 1, $ne: "null" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   var findCondition1a = { a: { $type: 1, $gt: 2 }, b: { $type: 1, $lt: 17 }, c: { $type: 1, $mod: [2, 0] } };
   var expRecs1a = [{ No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null }];
   checkResult( dbcl, findCondition1a, null, expRecs1a, { No: 1 } );

   var explainRecs1a = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $type: 1, $gt: 2 } }, { b: { $type: 1, $lt: 17 } }, { c: { $type: 1, $mod: [2, 0] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1a, null, { No: 1 }, explainRecs1a );

   //use in/nin/all
   var findCondition2 = { a: { $type: 1, $in: [2, -1, 16, 17, 3] }, b: { $type: 2, $in: ["int64", "array"] } };
   var expRecs2 = [{ No: 1, a: 123, b: { $numberLong: "9223372036854775807" }, c: 123.123, d: { $decimal: "123.00", $precision: [5, 2] } },
   { No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var explainRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $type: 1, $in: [2, -1, 16, 17, 3] } }, { b: { $type: 2, $in: ["int64", "array"] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, { No: 1 }, explainRecs2 );

   var findCondition3 = { a: { $type: 1, $nin: [16, 2] }, b: { $type: 2, $nin: ["decimal", "minkey"] } };
   var expRecs3 = [{ No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var explainRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $type: 1, $nin: [16, 2] } }, { b: { $type: 2, $nin: ["decimal", "minkey"] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, explainRecs3 );

   var findCondition4 = { a: { $type: 1, $all: [16, 2] } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var explainRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $type: 1, $all: [16, 2] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, { No: 1 }, explainRecs4 );

   //exists
   var findCondition5 = { a: { $type: 1, $exists: 1 } };
   var expRecs5 = [{ No: 1, a: 123, b: { $numberLong: "9223372036854775807" }, c: 123.123, d: { $decimal: "123.00", $precision: [5, 2] } },
   { No: 2, a: "string", b: { $oid: "123abcd00ef12358902300ef" }, c: true, d: { $date: "2012-12-12" } },
   { No: 3, a: { $timestamp: "2012-01-01-13.14.26.124233" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, d: { $regex: '^string$', $options: 'i' } },
   { No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $type: 1, $isnull: 0 } };
   var expRecs6 = [{ No: 1, a: 123, b: { $numberLong: "9223372036854775807" }, c: 123.123, d: { $decimal: "123.00", $precision: [5, 2] } },
   { No: 2, a: "string", b: { $oid: "123abcd00ef12358902300ef" }, c: true, d: { $date: "2012-12-12" } },
   { No: 3, a: { $timestamp: "2012-01-01-13.14.26.124233" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, d: { $regex: '^string$', $options: 'i' } },
   { No: 4, a: { 0: 1 }, b: [1, 2, 3], c: null }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );
}

