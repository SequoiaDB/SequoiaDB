/************************************
*@Description: index scan, field is Non-exists,use gt/gte/lt/lte to comapare 
*@author:      zhaoyu
*@createdate:  2016.8.4
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
   commCreateIndex( dbcl, "b", { b: 1 } );

   hintCondition = { '': '' };

   //insert data 
   var doc = [{ No: 1, a: 0 }, { No: 2, a: { $numberLong: "0" } }, { No: 3, a: { $decimal: "0" } },
   { No: 4, a: -2147483648 }, { No: 5, a: 2147483647 },
   { No: 6, a: -1.7E+308 }, { No: 7, a: 1.7E+308 },
   { No: 8, a: { $numberLong: "-9223372036854775808" } }, { No: 9, a: { $numberLong: "9223372036854775807" } },
   { No: 10, a: { $decimal: "-92233720368547758089223372036854775807" } }, { No: 11, a: { $decimal: "92233720368547758079223372036854775808" } },
   { No: 12, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 13, a: { $date: "2000-01-01" } },
   { No: 14, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
   { No: 15, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 16, a: { $regex: "^z", "$options": "i" } },
   { No: 17, a: null },
   { No: 18, a: "abc" },
   { No: 19, a: MinKey() },
   { No: 20, a: MaxKey() },
   { No: 21, a: true }, { No: 22, a: false },
   { No: 23, a: { name: "zhang" } },
   { No: 24, a: [1, 2, 3] }];
   dbcl.insert( doc );

   //$gt:0
   var findCondition1 = { b: { $gt: 0 } };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //$gte:0
   var findCondition2 = { b: { $gte: 0 } };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //$lt:0
   var findCondition3 = { b: { $lt: 0 } };
   var expRecs3 = [];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //$lte:0
   var findCondition4 = { b: { $lte: 0 } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

}

