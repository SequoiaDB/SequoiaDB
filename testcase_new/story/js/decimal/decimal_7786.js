/************************************
*@Description: insert decimal data and query by index when the index is decimal type;
*@author:      zhaoyu
*@createdate:  2016.4.28,
*@update:      assign the find condition (2016.7.9 by zhaoyu)
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7786";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert decimal data
   var doc = [{ a: { $decimal: "9223372036854775807198410" } },
   { a: { $decimal: "-9223372036854775808197101" } },
   { a: { $decimal: "-1.7E+398" } },
   { a: { $decimal: "1.7E+378" } },
   { a: { $decimal: "-4.94065645841246544E-380" } },
   { a: { $decimal: "4.94065645841246544E-390" } },
   { a: { $decimal: "9223372036854775807198411", $precision: [1000, 100] } },
   { a: { $decimal: "-9223372036854775808197102", $precision: [1000, 100] } },
   { a: { $decimal: "-1.71E+398", $precision: [1000, 100] } },
   { a: { $decimal: "1.71E+378", $precision: [1000, 100] } },
   { a: { $decimal: "-4.964065645841246544E-380", $precision: [1000, 999] } },
   { a: { $decimal: "4.964065645841246544E-390", $precision: [1000, 999] } },
   { a: 123 }];
   dbcl.insert( doc );

   //create index 
   commCreateIndex( dbcl, "aIndex", { a: 1 }, { Unique: true } );

   //find and check result
   var expRecs = [{ a: { $decimal: "9223372036854775807198410" } }];
   checkResult( dbcl, { a: { $decimal: "9223372036854775807198410" } }, null, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

