/************************************
*@Description: use multiply to match a field,
               value set int/double/numberLong/decimal and include negative/positive 
*@author:      zhaoyu
*@createdate:  2016.10.12
*@testlinkCase: seqDB-10255/seqDB-10266
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
   var doc = [{ No: 1, a: -1.2 }, { No: 2, a: 1.2 }, { No: 3, a: 10.2 },
   { No: 4, a: { $numberLong: "2" } }, { No: 5, a: { $numberLong: "-2" } }, { No: 6, a: { $numberLong: "20" } },
   { No: 7, a: { $decimal: "-1.2" } }, { No: 8, a: { $decimal: "1.2" } }, { No: 9, a: { $decimal: "10.2" } },
   { No: 10, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 11, a: { $date: "2000-01-01" } },
   { No: 12, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
   { No: 13, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 14, a: { $regex: "^z", "$options": "i" } },
   { No: 15, a: null },
   { No: 16, a: "abc" },
   { No: 17, a: MinKey() },
   { No: 18, a: MaxKey() },
   { No: 19, a: true },
   { No: 20, a: { name: "zhang" } },
   { No: 21, a: ["a", "b", "c"] }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $multiply: 1.2, $et: -1.44 } };
   var expRecs1 = [{ No: 1, a: -1.2 },
   { No: 7, a: { $decimal: "-1.2" } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $multiply: -5, $et: { $numberLong: "-6" } } };
   var expRecs2 = [{ No: 2, a: 1.2 },
   { No: 8, a: { $decimal: "1.2" } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { a: { $multiply: -1.2, $et: { $decimal: "-1.44", $precision: [10, 2] } } };
   var expRecs3 = [{ No: 2, a: 1.2 },
   { No: 8, a: { $decimal: "1.2" } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { a: { $multiply: { $decimal: "5" }, $et: { $numberLong: "-6" } } };
   var expRecs4 = [{ No: 1, a: -1.2 },
   { No: 7, a: { $decimal: "-1.2" } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var findCondition5 = { a: { $multiply: { $numberLong: "2" }, $et: 2.4 } };
   var expRecs5 = [{ No: 2, a: 1.2 },
   { No: 8, a: { $decimal: "1.2" } }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $multiply: { $decimal: "-1.2", $precision: [100, 3] }, $et: { $decimal: "-1.44", $precision: [10, 2] } } };
   var expRecs6 = [{ No: 2, a: 1.2 },
   { No: 8, a: { $decimal: "1.2" } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   condition7 = { a: { $multiply: -1 } };
   InvalidArgCheck( dbcl, condition7, null, -6 );

   condition8 = { a: { $multiply: "a" }, $et: 1 };
   InvalidArgCheck( dbcl, condition8, null, -6 );
}
