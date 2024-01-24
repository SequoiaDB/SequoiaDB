/************************************
*@Description: argument max precision and max scale check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7793";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "123", $precision: [5, 2] } }];
   dbcl.insert( doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "123.00", $precision: [5, 2] } }];
   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );

   //invalid argument check
   var invalidDoc1 = { a: { $decimal: "2", $precision: [2, 2] } };
   invalidDataInsertCheckResult( dbcl, invalidDoc1, -6 );
   commDropCL( db, COMMCSNAME, clName );
}

