/************************************
*@Description: find decimal data
*@author:      zhaoyu
*@createdate:  2016.4.25
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7749";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ a: { $decimal: "123" } },
   { a: { $numberLong: "123" } },
   { a: 123 }];
   dbcl.insert( doc );

   //check result
   checkResult( dbcl, { a: { $decimal: "123" } }, null, [{ a: { $decimal: "123" } }, { a: 123 }, { a: 123 }], { _id: 1 } );
   checkResult( dbcl, { a: 123 }, null, [{ a: { $decimal: "123" } }, { a: 123 }, { a: 123 }], { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

