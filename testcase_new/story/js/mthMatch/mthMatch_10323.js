/************************************
*@Description: and/or/not combaniation use string functions
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
   var doc = [{ No: 1, a: " \t\r\nString", b: "\t\r\n string \t\n\r" },
   { No: 2, a: "string\t\r\n ", b: " \t\r\nString" },
   { No: 3, a: "\t\r\n string \t\n\r", b: "StrInG" },
   { No: 4, a: "StrInG", b: "string\t\r\n " },
   { No: 5, b: " \t\r\nString" },
   { No: 6, b: "string\t\r\n " },
   { No: 7, b: "\t\r\n string \t\n\r" },
   { No: 8, b: "StrInG" }];
   dbcl.insert( doc );

   //seqDB-10322
   var findCondition1 = { $and: [{ a: { $substr: 2, $regex: 'st', $options: 'i' } }, { a: { $strlen: 1, $et: 6 } }] };
   var expRecs1 = [{ No: 4, a: "StrInG", b: "string\t\r\n " }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { a: { $substr: 2, $regex: 'st', $options: 'i', $strlen: 1, $et: 6 } };
   var expRecs2 = [{ No: 4, a: "StrInG", b: "string\t\r\n " }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { $and: [{ a: { $upper: 1, $regex: 'STRING', $options: 'i' } }, { b: { $trim: 1, $et: "string" } }] };
   var expRecs3 = [{ No: 1, a: " \t\r\nString", b: "\t\r\n string \t\n\r" },
   { No: 4, a: "StrInG", b: "string\t\r\n " }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   //seqDB-10323
   var findCondition4 = { $or: [{ a: { $lower: 1, $regex: '^STRING$', $options: 'i' } }, { a: { $rtrim: 1, $et: "string" } }] };
   var expRecs4 = [{ No: 2, a: "string\t\r\n ", b: " \t\r\nString" },
   { No: 4, a: "StrInG", b: "string\t\r\n " }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var findCondition5 = { $or: [{ a: { $lower: 1, $regex: '^STRING$', $options: 'i' } }, { b: { $rtrim: 1, $et: "string" } }] };
   var expRecs5 = [{ No: 4, a: "StrInG", b: "string\t\r\n " },
   { No: 6, b: "string\t\r\n " },];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   //seqDB-10324
   var findCondition6 = { $not: [{ a: { $substr: 2, $regex: 'st', $options: 'i' } }, { a: { $strlen: 1, $et: 6 } }] };
   var expRecs6 = [{ No: 1, a: " \t\r\nString", b: "\t\r\n string \t\n\r" },
   { No: 2, a: "string\t\r\n ", b: " \t\r\nString" },
   { No: 3, a: "\t\r\n string \t\n\r", b: "StrInG" },
   { No: 5, b: " \t\r\nString" },
   { No: 6, b: "string\t\r\n " },
   { No: 7, b: "\t\r\n string \t\n\r" },
   { No: 8, b: "StrInG" }];
   checkResult( dbcl, findCondition6, null, expRecs6, { _id: 1 } );

   var findCondition7 = { $not: [{ a: { $upper: 1, $regex: '^STRING$', $options: 'i' } }, { b: { $trim: 1, $et: "string" } }] };
   var expRecs7 = [{ No: 1, a: " \t\r\nString", b: "\t\r\n string \t\n\r" },
   { No: 2, a: "string\t\r\n ", b: " \t\r\nString" },
   { No: 3, a: "\t\r\n string \t\n\r", b: "StrInG" },
   { No: 5, b: " \t\r\nString" },
   { No: 6, b: "string\t\r\n " },
   { No: 7, b: "\t\r\n string \t\n\r" },
   { No: 8, b: "StrInG" }];
   checkResult( dbcl, findCondition7, null, expRecs7, { _id: 1 } );
}
