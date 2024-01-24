/************************************
*@Description: use abs to match a field,
               value set boundary of int/double/numberLong/decimal
*@author:      zhaoyu
*@createdate:  2016.10.12
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

   //int
   //insert int data 
   var doc1 = [{ No: 1, a: -2147483648 },
   { No: 2, a: -2147483649 },
   { No: 3, a: -2147483650 },
   { No: 4, a: 2147483647 },
   { No: 5, a: 2147483648 },
   { No: 6, a: 21474836549 }];
   dbcl.insert( doc1 );

   //$lt:2147483647
   var findCondition1 = { a: { $abs: 1, $lt: 2147483649 } };
   var expRecs1 = [{ No: 1, a: -2147483648 },
   { No: 4, a: 2147483647 },
   { No: 5, a: 2147483648 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   //remove all data and insert double data
   dbcl.remove();

   var doc2 = [{ No: 1, a: -1.7E+307 },
   { No: 2, a: -1.7E+308 },
   { No: 3, a: -1.7E+309 },
   { No: 4, a: 1.7E+307 },
   { No: 5, a: 1.7E+308 },
   { No: 6, a: 1.7E+309 }];
   dbcl.insert( doc2 );

   //$lt:1.7E+308
   var findCondition2 = { a: { $abs: 1, $lt: 1.7E+308 } };
   var expRecs2 = [{ No: 1, a: -1.7E+307 },
   { No: 4, a: 1.7E+307 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   //remove all data and insert decimal data
   dbcl.remove();

   var doc3 = [{ No: 1, a: { $decimal: "-1.7E+308", $precision: [1000, 2] } },
   { No: 2, a: { $decimal: "-1.7E+311", $precision: [1000, 2] } },
   { No: 3, a: { $decimal: "-1.7E+312", $precision: [1000, 2] } },
   { No: 4, a: { $decimal: "1.7E+308", $precision: [1000, 2] } },
   { No: 5, a: { $decimal: "1.7E+311", $precision: [1000, 2] } },
   { No: 6, a: { $decimal: "1.7E+312", $precision: [1000, 2] } }];
   dbcl.insert( doc3 );

   //$lt:{$decimal:"1.7E+311"}
   var findCondition3 = { a: { $abs: 1, $lt: { $decimal: "1.7E+311" } } };
   var expRecs3 = [{ No: 1, a: { $decimal: "-170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.00", $precision: [1000, 2] } },
   { No: 4, a: { $decimal: "170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.00", $precision: [1000, 2] } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   //remove all data and insert numberLong data
   dbcl.remove();

   var doc4 = [{ No: 1, a: { $numberLong: "-9223372036854775807" } },
   { No: 2, a: { $numberLong: "-9223372036854775808" } },
   { No: 3, a: { $numberLong: "-9223372036854775809" } },
   { No: 4, a: { $numberLong: "9223372036854775806" } },
   { No: 5, a: { $numberLong: "9223372036854775807" } },
   { No: 6, a: { $numberLong: "9223372036854775808" } }];
   dbcl.insert( doc4 );

   //$lt:{$numberLong:"9223372036854775807"}
   var findCondition4 = { a: { $abs: 1, $lt: { $numberLong: "9223372036854775807" } } };
   var expRecs4 = [{ No: 4, a: { $numberLong: "9223372036854775806" } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

}
