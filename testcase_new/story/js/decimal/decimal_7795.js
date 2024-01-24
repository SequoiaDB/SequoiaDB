/************************************
*@Description: argument actual precision and max precision check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7795"
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "123", $precision: [5, 2] } },
   { a: { $decimal: "456", $precision: [3, 0] } }];
   dbcl.insert( doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "123.00", $precision: [5, 2] } },
   { a: { $decimal: "456", $precision: [3, 0] } }];
   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "789", $precision: [5, 3] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );
   commDropCL( db, COMMCSNAME, clName );
}