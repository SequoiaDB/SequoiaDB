/************************************
*@Description: decimal data use floor
*@author:     zhaoyu
*@createdate:  2016.4.26
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "7771";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "1.7E+400", $precision: [1000, 10] } },
   { a: { $decimal: "-5.94E-400", $precision: [1000, 600] } },
   { a: { $decimal: "0", $precision: [10, 2] } },
   { a: { $decimal: "-92233720368547758071234567899.123456789" } },
   { a: { $decimal: "92233720368547758071234567899.523456789" } },
   { a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NAN" } },
   { a: { $decimal: "-INF" } },
   { a: { $decimal: "INF" } }];
   dbcl.insert( doc );

   //check result 
   var expRecs = [{ a: { $decimal: "17000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "-1" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "-92233720368547758071234567900" } },
   { a: { $decimal: "92233720368547758071234567899" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } },
   { a: { $decimal: "NaN" } }]
   checkResult( dbcl, {}, { a: { $floor: 1 } }, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

