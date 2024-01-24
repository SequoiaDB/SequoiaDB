/************************************
*@Description: decimal data use $mod
*@author:      zhaoyu
*@createdate:  2016.5.3
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7764";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert decimal data
   var doc = [{ a: { $decimal: "-9223372036854775808456", $precision: [1000, 2] } },
   { a: { $decimal: "9223372036854775807123" } },
   { a: { $decimal: "-18446744073709551616911" } },
   { a: { $decimal: "18446744073709551614245" } },
   { a: 123 }];
   dbcl.insert( doc );

   //check result
   var expRecs0 = [{ a: { $decimal: "-9223372036854775808456.00", $precision: [1000, 2] } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "-2147483648" }, { $decimal: "-456.000", $precision: [10, 3] }] } }, null, expRecs0, { _id: 1 } );
   checkResult( dbcl, { a: { $mod: [{ $decimal: "-2147483648" }, -456] } }, null, expRecs0, { _id: 1 } );

   var expRecs1 = [{ a: { $decimal: "-9223372036854775808456.00", $precision: [1000, 2] } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "2147483648" }, { $decimal: "-456.00", $precision: [10, 2] }] } }, null, expRecs1, { _id: 1 } );
   checkResult( dbcl, { a: { $mod: [{ $decimal: "2147483648" }, -456] } }, null, expRecs1, { _id: 1 } );

   var expRecs2 = [{ a: { $decimal: "9223372036854775807123" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "2147483647" }, { $decimal: "1123", $precision: [10, 3] }] } }, null, expRecs2, { _id: 1 } );
   checkResult( dbcl, { a: { $mod: [2147483647, { $decimal: "1123", $precision: [10, 3] }] } }, null, expRecs2, { _id: 1 } );

   var expRecs3 = [{ a: { $decimal: "9223372036854775807123" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "-2147483647" }, { $decimal: "1123", $precision: [10, 3] }] } }, null, expRecs3, { _id: 1 } );
   checkResult( dbcl, { a: { $mod: [-2147483647, { $decimal: "1123", $precision: [10, 3] }] } }, null, expRecs3, { _id: 1 } );

   var expRecs4 = [{ a: 123 }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "30" }, { $decimal: "3", $precision: [10, 3] }] } }, null, expRecs4, { _id: 1 } );
   checkResult( dbcl, { a: { $mod: [30, { $decimal: "3", $precision: [10, 3] }] } }, null, expRecs4, { _id: 1 } );

   var expRecs5 = [{ a: { $decimal: "-18446744073709551616911" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "9223372036854775808456" }, { $decimal: "-9223372036854775808455" }] } }, null, expRecs5, { _id: 1 } );

   var expRecs6 = [{ a: { $decimal: "-18446744073709551616911" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "-9223372036854775808456", $precision: [100, 2] }, { $decimal: "-9223372036854775808455.000", $precision: [100, 3] }] } }, null, expRecs6, { _id: 1 } );

   var expRecs7 = [{ a: { $decimal: "18446744073709551614245" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "-9223372036854775807123", $precision: [100, 2] }, { $decimal: "9223372036854775807122.000", $precision: [100, 3] }] } }, null, expRecs7, { _id: 1 } );

   var expRecs8 = [{ a: { $decimal: "18446744073709551614245" } }];
   checkResult( dbcl, { a: { $mod: [{ $decimal: "9223372036854775807123", $precision: [100, 2] }, { $decimal: "9223372036854775807122" }] } }, null, expRecs8, { _id: 1 } );

   commDropCL( db, COMMCSNAME, clName );
}

