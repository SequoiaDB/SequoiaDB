/************************************
*@Description: decimal data use elemMatch
*@author:     zhaoyu
*@createdate:  2016.4.25
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7766";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { age: { $decimal: "26" }, weight: { $decimal: "50.56", $precision: [10, 2] } } },
   { a: { age: { $decimal: "25" }, weight: { $decimal: "50.56", $precision: [10, 2] } } }];
   dbcl.insert( doc );

   //check result
   var expRecs = [{ a: { age: { $decimal: "26" }, weight: { $decimal: "50.56", $precision: [10, 2] } } }];
   checkResult( dbcl, { a: { $elemMatch: { age: { $decimal: "26" }, weight: 50.56 } } }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { a: { $elemMatch: { age: { $decimal: "26", $precision: [10, 2] }, weight: { $decimal: "50.56", $precision: [10, 3] } } } }, null, expRecs, { _id: 1 } );
   checkResult( dbcl, { a: { $elemMatch: { age: 26, weight: 50.56 } } }, null, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}


