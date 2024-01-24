/************************************
*@Description: use abs to match a field,
               value set int/double/numberLong/decimal and include negative/positive 
*@author:      zhaoyu
*@createdate:  2016.10.12
*@testlinkCase: seqDB-10243/seqDB-10246
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
   var doc = [{ No: 1, a: 0 }, { No: 2, a: { $numberLong: "0" } }, { No: 3, a: { $decimal: "0" } },
   { No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
   { No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
   { No: 18, a: { $numberLong: "1002" } }, { No: 19, a: { $numberLong: "1003" } }, { No: 20, a: { $numberLong: "1001" } },
   { No: 21, a: { $numberLong: "-1002" } }, { No: 22, a: { $numberLong: "-1003" } }, { No: 23, a: { $numberLong: "-1001" } },
   { No: 24, a: { $numberLong: "1002" } }, { No: 25, a: { $numberLong: "1003" } }, { No: 26, a: { $numberLong: "1001" } },
   { No: 27, a: { $numberLong: "-1002" } }, { No: 28, a: { $numberLong: "-1003" } }, { No: 29, a: { $numberLong: "-1001" } },
   { No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
   { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
   { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
   { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
   { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
   { No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
   { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
   { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
   { No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
   { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
   { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
   { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
   { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
   { No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
   { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
   { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
   { No: 90, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 91, a: { $date: "2000-01-01" } },
   { No: 92, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
   { No: 93, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 94, a: { $regex: "^z", "$options": "i" } },
   { No: 95, a: null },
   { No: 96, a: "abc" },
   { No: 97, a: MinKey() },
   { No: 98, a: MaxKey() },
   { No: 99, a: true }, { No: 100, a: false },
   { No: 101, a: { name: "zhang" } },
   { No: 102, a: [1, 2, 3] }];
   dbcl.insert( doc );

   //int and double,0/negative/positive
   var findCondition1 = { a: { $abs: 1, $et: 0 } };
   var expRecs1 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $abs: 1, $gt: 1002 } };
   var expRecs2 = [{ No: 5, a: 1003 }, { No: 9, a: -1003 }, { No: 12, a: 1003 }, { No: 16, a: -1003 }, { No: 19, a: 1003 },
   { No: 22, a: -1003 }, { No: 25, a: 1003 }, { No: 28, a: -1003 },
   { No: 31, a: { $decimal: "1003" } }, { No: 34, a: { $decimal: "-1003" } },
   { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
   { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } },
   { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
   { No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
   { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } },
   { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
   { No: 61, a: { $decimal: "1003" } }, { No: 64, a: { $decimal: "-1003" } },
   { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
   { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } },
   { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
   { No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
   { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
   { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition13 = { a: { $abs: 1, $gte: -1002.123 } };
   var expRecs13 = [{ No: 1, a: 0 }, { No: 2, a: 0 }, { No: 3, a: { $decimal: "0" } },
   { No: 4, a: 1002 }, { No: 5, a: 1003 }, { No: 6, a: 1001 }, { No: 7, a: 1002 }, { No: 8, a: -1002 }, { No: 9, a: -1003 }, { No: 10, a: -1001 },
   { No: 11, a: 1002 }, { No: 12, a: 1003 }, { No: 13, a: 1001 }, { No: 14, a: 1002 }, { No: 15, a: -1002 }, { No: 16, a: -1003 }, { No: 17, a: -1001 },
   { No: 18, a: 1002 }, { No: 19, a: 1003 }, { No: 20, a: 1001 },
   { No: 21, a: -1002 }, { No: 22, a: -1003 }, { No: 23, a: -1001 },
   { No: 24, a: 1002 }, { No: 25, a: 1003 }, { No: 26, a: 1001 },
   { No: 27, a: -1002 }, { No: 28, a: -1003 }, { No: 29, a: -1001 },
   { No: 30, a: { $decimal: "1002" } }, { No: 31, a: { $decimal: "1003" } }, { No: 32, a: { $decimal: "1001" } },
   { No: 33, a: { $decimal: "-1002" } }, { No: 34, a: { $decimal: "-1003" } }, { No: 35, a: { $decimal: "-1001" } },
   { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } }, { No: 38, a: { $decimal: "1001.01" } },
   { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } }, { No: 41, a: { $decimal: "-1001.01" } },
   { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 44, a: 1001.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 }, { No: 47, a: -1001.01 },
   { No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } }, { No: 50, a: { $decimal: "1001.02" } },
   { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } }, { No: 53, a: { $decimal: "-1001.02" } },
   { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 56, a: 1001.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 }, { No: 59, a: -1001.02 },
   { No: 60, a: { $decimal: "1002" } }, { No: 61, a: { $decimal: "1003" } }, { No: 62, a: { $decimal: "1001" } },
   { No: 63, a: { $decimal: "-1002" } }, { No: 64, a: { $decimal: "-1003" } }, { No: 65, a: { $decimal: "-1001" } },
   { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } }, { No: 68, a: { $decimal: "1001.01" } },
   { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } }, { No: 71, a: { $decimal: "-1001.01" } },
   { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 74, a: 1001.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 }, { No: 77, a: -1001.01 },
   { No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } }, { No: 80, a: { $decimal: "1001.02" } },
   { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } }, { No: 83, a: { $decimal: "-1001.02" } },
   { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 86, a: 1001.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }, { No: 89, a: -1001.02 },
   { No: 102, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition13, null, expRecs13, { No: 1 } );


   //decimal
   var findCondition4 = { a: { $abs: 1, $gt: { $decimal: "1002", $precision: [100, 2] } } };
   var expRecs4 = [{ No: 5, a: 1003 }, { No: 9, a: -1003 }, { No: 12, a: 1003 }, { No: 16, a: -1003 }, { No: 19, a: 1003 },
   { No: 22, a: -1003 }, { No: 25, a: 1003 }, { No: 28, a: -1003 },
   { No: 31, a: { $decimal: "1003" } }, { No: 34, a: { $decimal: "-1003" } },
   { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
   { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } },
   { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
   { No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
   { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } },
   { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
   { No: 61, a: { $decimal: "1003" } }, { No: 64, a: { $decimal: "-1003" } },
   { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
   { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } },
   { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
   { No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
   { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
   { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //numberLong
   var findCondition5 = { a: { $abs: 1, $gt: { $numberLong: "1002" } } };
   var expRecs5 = [{ No: 5, a: 1003 }, { No: 9, a: -1003 }, { No: 12, a: 1003 }, { No: 16, a: -1003 }, { No: 19, a: 1003 },
   { No: 22, a: -1003 }, { No: 25, a: 1003 }, { No: 28, a: -1003 },
   { No: 31, a: { $decimal: "1003" } }, { No: 34, a: { $decimal: "-1003" } },
   { No: 36, a: { $decimal: "1002.01" } }, { No: 37, a: { $decimal: "1003.01" } },
   { No: 39, a: { $decimal: "-1002.01" } }, { No: 40, a: { $decimal: "-1003.01" } },
   { No: 42, a: 1002.01 }, { No: 43, a: 1003.01 }, { No: 45, a: -1002.01 }, { No: 46, a: -1003.01 },
   { No: 48, a: { $decimal: "1002.02" } }, { No: 49, a: { $decimal: "1003.02" } },
   { No: 51, a: { $decimal: "-1002.02" } }, { No: 52, a: { $decimal: "-1003.02" } },
   { No: 54, a: 1002.02 }, { No: 55, a: 1003.02 }, { No: 57, a: -1002.02 }, { No: 58, a: -1003.02 },
   { No: 61, a: { $decimal: "1003" } }, { No: 64, a: { $decimal: "-1003" } },
   { No: 66, a: { $decimal: "1002.01" } }, { No: 67, a: { $decimal: "1003.01" } },
   { No: 69, a: { $decimal: "-1002.01" } }, { No: 70, a: { $decimal: "-1003.01" } },
   { No: 72, a: 1002.01 }, { No: 73, a: 1003.01 }, { No: 75, a: -1002.01 }, { No: 76, a: -1003.01 },
   { No: 78, a: { $decimal: "1002.02" } }, { No: 79, a: { $decimal: "1003.02" } },
   { No: 81, a: { $decimal: "-1002.02" } }, { No: 82, a: { $decimal: "-1003.02" } },
   { No: 84, a: 1002.02 }, { No: 85, a: 1003.02 }, { No: 87, a: -1002.02 }, { No: 88, a: -1003.02 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //seqDB-10246
   condition6 = { a: { $abs: -1, $et: 1 } };
   InvalidArgCheck( dbcl, condition6, null, SDB_INVALIDARG );

   condition7 = { a: { $abs: -1 } };
   InvalidArgCheck( dbcl, condition7, null, SDB_INVALIDARG );
}
