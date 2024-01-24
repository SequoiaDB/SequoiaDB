/************************************
*@Description: decimal data use +$
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7767";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: [{ $decimal: "1" }, { $decimal: "2" }, { $decimal: "3" }, { $decimal: "4" }, { $decimal: "5" }] },
   { a: [{ $decimal: "1" }, { $decimal: "4" }, { $decimal: "5", $precision: [5, 2] }] },
   { a: [{ $decimal: "4" }, { $decimal: "2" }, { $decimal: "1" }] }];
   dbcl.insert( doc );

   //check result
   var expRecs = [{ a: [{ $decimal: "1" }, { $decimal: "2" }, { $decimal: "3" }, { $decimal: "4" }, { $decimal: "5" }] },
   { a: [{ $decimal: "1" }, { $decimal: "4" }, { $decimal: "5.00", $precision: [5, 2] }] }];
   checkResult( dbcl, { "a.$1": 5 }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { "a.$1": { $decimal: "5" } }, null, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

