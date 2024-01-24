/************************************
*@Description: numberical value use functions,combinate all kinds of matches
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
   var doc = [{ No: 1, a: -10, b: -9 },
   { No: 2, a: 10, b: 11 },
   { No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] },
   { No: 5, a: { 0: -10, 1: 10 }, b: { 0: -10, 1: 10 } },
   { No: 6, a: { 0: 10, 1: -10 }, b: { 0: 10, 1: -10 } },
   { No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 9, a: "strings", b: "strings" },
   { No: 10, a: "aString", b: "aString" },
   { No: 11, b: -10 },
   { No: 12, b: 10 },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 15, a: [" \n\r\tstrings\t\r\n ", "aString"], b: ["strings", "aString"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] },
   { No: 17, a: { 0: "string", 1: "String" }, b: { 0: "string", 1: "String" } },
   { No: 18, a: { 0: "String", 1: "string" }, b: { 0: "String", 1: "string" } },
   { No: 19, a: { 0: "strings", 1: "aString" }, b: { 0: "strings", 1: "aString" } },
   { No: 20, a: { 0: "aString", 1: "strings" }, b: { 0: "aString", 1: "strings" } }];
   dbcl.insert( doc );

   //seqDB-10313
   var findCondition1 = { a: { $abs: 1, $exists: 0 } };
   var expRecs1 = [{ No: 11, b: -10 },
   { No: 12, b: 10 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var explainRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $abs: 1, $exists: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, { No: 1 }, explainRecs1 );

   var findCondition2 = { a: { $floor: 1, $type: 1, $et: 100 } };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var explainRecs2 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $floor: 1, $type: 1, $et: 100 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition2, null, { No: 1 }, explainRecs2 );

   var findCondition3 = { a: { $ceiling: 1, $isnull: 1 }, b: { $ceiling: 1, $isnull: 0 } };
   var expRecs3 = [{ No: 11, b: -10 },
   { No: 12, b: 10 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var explainRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $ceiling: 1, $isnull: 1 } }, { b: { $ceiling: 1, $isnull: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, explainRecs3 );

   var findCondition4 = { a: { $subtract: 1, $regex: '^string$', $options: 'i' } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var explainRecs4 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $subtract: 1, $regex: '^string$', $options: 'i' } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition4, null, { No: 1 }, explainRecs4 );

   var findCondition5 = { a: { $add: 1, $field: "b" } };
   var expRecs5 = [{ No: 1, a: -10, b: -9 },
   { No: 2, a: 10, b: 11 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   var explainRecs5 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $add: 1, $et: { $field: "b" } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition5, null, { No: 1 }, explainRecs5 );

   var findCondition6 = { a: { $multiply: 2, $divide: -2, $in: [10, -9] } };
   var expRecs6 = [{ No: 1, a: -10, b: -9 },
   { No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] }];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );

   var explainRecs6 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $multiply: 2, $divide: -2, $in: [10, -9] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition6, null, { No: 1 }, explainRecs6 );

   var findCondition7 = { "a.$1": { $mod: 2, $et: 0 } };
   var expRecs7 = [{ No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] }];
   checkResult( dbcl, findCondition7, null, expRecs7, { _id: 1 } );

   var explainRecs7 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ "a.$1": { $mod: 2, $et: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition7, null, { No: 1 }, explainRecs7 );

   var findCondition8 = { a: { $abs: 1, $size: 1, $et: 2 } };
   var expRecs8 = [{ No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 15, a: [" \n\r\tstrings\t\r\n ", "aString"], b: ["strings", "aString"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition8, null, expRecs8, { _id: 1 } );

   var explainRecs8 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $abs: 1, $size: 1, $et: 2 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition8, null, { No: 1 }, explainRecs8 );

   var findCondition9 = { a: { $subtract: 1, $et: -11 } };
   var expRecs9 = [{ No: 1, a: -10, b: -9 },
   { No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] }];
   checkResult( dbcl, findCondition9, null, expRecs9, { _id: 1 } );

   var explainRecs9 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $subtract: 1, $et: -11 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition9, null, { No: 1 }, explainRecs9 );

   //seqDB-10314
   var findCondition10 = { a: { $substr: [1, 5], $et: "tring" } };
   var expRecs10 = [{ No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 9, a: "strings", b: "strings" },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition10, null, expRecs10, { _id: 1 } );

   var explainRecs10 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $substr: [1, 5], $et: "tring" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition10, null, { No: 1 }, explainRecs10 );

   var findCondition11 = { a: { $strlen: 1, $in: [6, 7] } };
   var expRecs11 = [{ No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 9, a: "strings", b: "strings" },
   { No: 10, a: "aString", b: "aString" },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 15, a: [" \n\r\tstrings\t\r\n ", "aString"], b: ["strings", "aString"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition11, null, expRecs11, { _id: 1 } );

   var explainRecs11 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $strlen: 1, $in: [6, 7] } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition11, null, { No: 1 }, explainRecs11 );

   var findCondition12 = { a: { $upper: 1, $exists: 0 } };
   var expRecs12 = [{ No: 11, b: -10 },
   { No: 12, b: 10 }];
   checkResult( dbcl, findCondition12, null, expRecs12, { _id: 1 } );

   var explainRecs12 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $upper: 1, $exists: 0 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition12, null, { No: 1 }, explainRecs12 );

   var findCondition13 = { a: { $lower: 1, $isnull: 1 } };
   var expRecs13 = [{ No: 1, a: -10, b: -9 },
   { No: 2, a: 10, b: 11 },
   { No: 5, a: { 0: -10, 1: 10 }, b: { 0: -10, 1: 10 } },
   { No: 6, a: { 0: 10, 1: -10 }, b: { 0: 10, 1: -10 } },
   { No: 11, b: -10 },
   { No: 12, b: 10 },
   { No: 17, a: { 0: "string", 1: "String" }, b: { 0: "string", 1: "String" } },
   { No: 18, a: { 0: "String", 1: "string" }, b: { 0: "String", 1: "string" } },
   { No: 19, a: { 0: "strings", 1: "aString" }, b: { 0: "strings", 1: "aString" } },
   { No: 20, a: { 0: "aString", 1: "strings" }, b: { 0: "aString", 1: "strings" } }];
   checkResult( dbcl, findCondition13, null, expRecs13, { _id: 1 } );

   var explainRecs13 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $lower: 1, $isnull: 1 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition13, null, { No: 1 }, explainRecs13 );

   var findCondition14 = { a: { $lower: 1, $regex: '^string$' } };
   var expRecs14 = [{ No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] }];
   checkResult( dbcl, findCondition14, null, expRecs14, { _id: 1 } );

   var explainRecs14 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $lower: 1, $regex: '^string$', $options: "" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition14, null, { No: 1 }, explainRecs14 );

   var findCondition15 = { "a.$1": { $trim: 1, $et: "strings" } };
   var expRecs15 = [{ No: 15, a: [" \n\r\tstrings\t\r\n ", "aString"], b: ["strings", "aString"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition15, null, expRecs15, { _id: 1 } );

   var explainRecs15 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ "a.$1": { $trim: 1, $et: "strings" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition15, null, { No: 1 }, explainRecs15 );

   var findCondition16 = { a: { $ltrim: 1, $size: 1, $et: 2 } };
   var expRecs16 = [{ No: 3, a: [-10, 10], b: [-10, 10] },
   { No: 4, a: [10, -10], b: [10, -10] },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 15, a: [" \n\r\tstrings\t\r\n ", "aString"], b: ["strings", "aString"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition16, null, expRecs16, { _id: 1 } );

   var explainRecs16 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $ltrim: 1, $size: 1, $et: 2 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition16, null, { No: 1 }, explainRecs16 );

   var findCondition17 = { a: { $rtrim: 1, $field: "b" } };
   var expRecs17 = [{ No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 9, a: "strings", b: "strings" },
   { No: 10, a: "aString", b: "aString" },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition17, null, expRecs17, { _id: 1 } );

   var explainRecs17 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $rtrim: 1, $et: { $field: "b" } } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition17, null, { No: 1 }, explainRecs17 );

   var findCondition18 = { a: { $substr: 1, $et: "s" } };
   var expRecs18 = [{ No: 7, a: "string", b: "string" },
   { No: 9, a: "strings", b: "strings" },
   { No: 13, a: ["string", "String"], b: ["string", "String"] },
   { No: 14, a: ["String", "string"], b: ["String", "string"] },
   { No: 16, a: ["aString", "strings"], b: ["aString", "strings"] }];
   checkResult( dbcl, findCondition18, null, expRecs18, { _id: 1 } );

   var explainRecs18 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $substr: 1, $et: "s" } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition18, null, { No: 1 }, explainRecs18 );

   var findCondition19 = { a: { $cast: "string", $type: 1, $et: 2 } };
   var expRecs19 = [{ No: 1, a: -10, b: -9 },
   { No: 2, a: 10, b: 11 },
   { No: 5, a: { 0: -10, 1: 10 }, b: { 0: -10, 1: 10 } },
   { No: 6, a: { 0: 10, 1: -10 }, b: { 0: 10, 1: -10 } },
   { No: 7, a: "string", b: "string" },
   { No: 8, a: "String", b: "String" },
   { No: 9, a: "strings", b: "strings" },
   { No: 10, a: "aString", b: "aString" },
   { No: 17, a: { 0: "string", 1: "String" }, b: { 0: "string", 1: "String" } },
   { No: 18, a: { 0: "String", 1: "string" }, b: { 0: "String", 1: "string" } },
   { No: 19, a: { 0: "strings", 1: "aString" }, b: { 0: "strings", 1: "aString" } },
   { No: 20, a: { 0: "aString", 1: "strings" }, b: { 0: "aString", 1: "strings" } }];
   checkResult( dbcl, findCondition19, null, expRecs19, { _id: 1 } );

   var explainRecs19 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      Query: { $and: [{ a: { $cast: "string", $type: 1, $et: 2 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition19, null, { No: 1 }, explainRecs19 );
}
