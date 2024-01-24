/************************************
*@Description: argument actual scale and max scale check for {$decimal:"xxx",$precision:[xx,xx]}
*@author:      zhaoyu
*@createdate:  2016.5.4
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7796"
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "123.12", $precision: [5, 2] } },
   { a: { $decimal: "456.45", $precision: [6, 3] } },
   { a: { $decimal: "789.12", $precision: [6, 0] } },
   { a: { $decimal: "432.55", $precision: [6, 1] } }];
   dbcl.insert( doc );

   //valid argument check
   var expRecs = [{ a: { $decimal: "123.12", $precision: [5, 2] } },
   { a: { $decimal: "456.450", $precision: [6, 3] } },
   { a: { $decimal: "789", $precision: [6, 0] } },
   { a: { $decimal: "432.6", $precision: [6, 1] } }];
   checkResult( dbcl, {}, {}, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}