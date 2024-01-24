/************************************
*@Description: decimal data use $et
*@author:      zhaoyu
*@createdate:  2016.5.3
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7762";
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
   { a: "-5.94E-325" },
   { a: "string" }];
   dbcl.insert( doc );

   //check result
   var expRecs0 = [{ a: { $decimal: "0" } }];
   checkResult( dbcl, { a: { $et: 0 } }, null, expRecs0, { _id: 1 } );
   checkResult( dbcl, { a: { $et: { $decimal: "0" } } }, null, expRecs0, { _id: 1 } );

   var expRecs1 = [{ a: { $decimal: "-0.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000594000", $precision: [1000, 330] } }];
   checkResult( dbcl, { a: { $et: { $decimal: "-5.94E-325" } } }, null, expRecs1, { _id: 1 } );

   var expRecs2 = [{ a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000694" } }];
   checkResult( dbcl, { a: { $et: { $decimal: "6.94E-330", $precision: [1000, 340] } } }, null, expRecs2, { _id: 1 } );

   var expRecs3 = [];
   checkResult( dbcl, { a: { $et: { $decimal: "1589.64", $precision: [10, 1] } } }, null, expRecs3, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

