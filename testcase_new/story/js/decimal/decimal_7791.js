/************************************
*@Description: argument max precision check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7791";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "9223372036854775807198410", $precision: [1000, 2] } },
   { a: { $decimal: "2", $precision: [1, 0] } }];
   dbcl.insert( doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "9223372036854775807198410.00", $precision: [1000, 2] } },
   { a: { $decimal: "2", $precision: [1, 0] } }];

   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "2", $precision: [0, 0] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );

   var invalidDoc2 = { a: { $decimal: "9223372036854775807198410", $precision: [1001, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc2, -6 );

   var invalidDoc3 = { a: { $decimal: "2", $precision: ["a", 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc3, -6 );

   var invalidDoc4 = { a: { $decimal: "2", $precision: [-1, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc4, -6 );

   var invalidDoc5 = { a: { $decimal: "2", $precision: [3.2, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc5, -6 );
   commDropCL( db, COMMCSNAME, clName );
}

