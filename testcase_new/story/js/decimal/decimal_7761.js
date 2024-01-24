/************************************
*@Description: decimal data use $lte
*@author:      zhaoyu
*@createdate:  2016.5.3
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7761";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert decimal data
   var doc = [{ a: { $decimal: "-5.94E-325", $precision: [1000, 330] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "6.94E-330" } },
   { a: { $decimal: "-1.99E+310" } },
   { a: { $decimal: "2.99E+320" } },
   { a: { $decimal: "1589.64", $precision: [10, 2] } },
   { a: 123 },
   { a: "string" }];
   dbcl.insert( doc );

   //check result
   var expRecs0 = [{ a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000594000", $precision: [1000, 330] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "-19900000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } }];
   checkResult( dbcl, { a: { $lte: 0 } }, null, expRecs0, { _id: 1 } );
   checkResult( dbcl, { a: { $lte: { $decimal: "0" } } }, null, expRecs0, { _id: 1 } );

   var expRecs1 = [{ a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000594000", $precision: [1000, 330] } },
   { a: { $decimal: "-19900000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } }];
   checkResult( dbcl, { a: { $lte: { $decimal: "-5.94E-325" } } }, null, expRecs1, { _id: 1 } );

   var expRecs2 = [{ a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000594000", $precision: [1000, 330] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000694" } },
   { a: { $decimal: "-19900000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } }];
   checkResult( dbcl, { a: { $lte: { $decimal: "6.94E-330" } } }, null, expRecs2, { _id: 1 } );

   var expRecs3 = [{ a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000594000", $precision: [1000, 330] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000694" } },
   { a: { $decimal: "-19900000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "1589.64", $precision: [10, 2] } },
   { a: 123 }];
   checkResult( dbcl, { a: { $lte: { $decimal: "1589.7", $precision: [10, 3] } } }, null, expRecs3, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

