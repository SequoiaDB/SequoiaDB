/************************************
*@Description: decimal data use $feild
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7768";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );
   
   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ t1: { $decimal: "12" }, t2: { $decimal: "12", $precision: [5, 2] } },
   { t1: { $decimal: "12" }, t2: 12 },
   { t1: { $decimal: "12" }, t2: { $decimal: "13", $precision: [5, 2] } }];
   dbcl.insert( doc );

   //check result
   var expRecs = [{ t1: { $decimal: "12" }, t2: { $decimal: "12.00", $precision: [5, 2] } },
   { t1: { $decimal: "12" }, t2: 12 }];
   checkResult( dbcl, { t1: { $field: "t2" } }, null, expRecs, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

