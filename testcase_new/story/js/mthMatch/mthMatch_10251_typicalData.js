/************************************
*@Description: use floor to match a field,
               value set int/double/numberLong/decimal 
*@author:      zhaoyu
*@createdate:  2016.10.13
*@testlinkCase: seqDB-10251/seqDB-10254
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
   var doc = [{ No: 1, a: 2.3 }, { No: 2, a: 2.5 }, { No: 3, a: -2.3 }, { No: 4, a: -2.5 },
   { No: 5, a: { $numberLong: "2" } }, { No: 6, a: { $numberLong: "-2" } }, { No: 7, a: { $numberLong: "3" } }, { No: 8, a: { $numberLong: "-3" } },
   { No: 9, a: { $decimal: "2.3" } }, { No: 10, a: { $decimal: "2.5" } }, { No: 11, a: { $decimal: "-2.3" } }, { No: 12, a: { $decimal: "-2.5" } },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: { $date: "2000-01-01" } },
   { No: 15, a: { $timestamp: "2000-01-01-15.32.18.000000" } },
   { No: 16, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 17, a: { $regex: "^z", "$options": "i" } },
   { No: 18, a: null },
   { No: 19, a: "abc" },
   { No: 20, a: MinKey() },
   { No: 21, a: MaxKey() },
   { No: 22, a: true }, { No: 100, a: false },
   { No: 23, a: { name: "zhang" } },
   { No: 24, a: [1, 2, 3] }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $abs: 1, $floor: 1, $et: 2 } };
   var expRecs1 = [{ No: 1, a: 2.3 }, { No: 2, a: 2.5 }, { No: 3, a: -2.3 }, { No: 4, a: -2.5 },
   { No: 5, a: 2 }, { No: 6, a: -2 },
   { No: 9, a: { $decimal: "2.3" } }, { No: 10, a: { $decimal: "2.5" } }, { No: 11, a: { $decimal: "-2.3" } }, { No: 12, a: { $decimal: "-2.5" } },
   { No: 24, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { a: { $floor: 1, $abs: 1, $et: { $numberLong: "2" } } };
   var expRecs2 = [{ No: 1, a: 2.3 }, { No: 2, a: 2.5 },
   { No: 5, a: 2 }, { No: 6, a: -2 },
   { No: 9, a: { $decimal: "2.3" } }, { No: 10, a: { $decimal: "2.5" } },
   { No: 24, a: [1, 2, 3] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   //numberLong
   var findCondition3 = { $and: [{ a: { $abs: 1, $et: { $decimal: "2.3", $precision: [10, 2] } } }, { a: { $floor: 1, $et: { $numberLong: "2" } } }] };
   var expRecs3 = [{ No: 1, a: 2.3 }, { No: 9, a: { $decimal: "2.3" } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //seqDB-10250
   condition4 = { a: { $floor: -1, $et: 1 } };
   InvalidArgCheck( dbcl, condition4, null, -6 );

   condition5 = { a: { $floor: -1 } };
   InvalidArgCheck( dbcl, condition5, null, -6 );
}
