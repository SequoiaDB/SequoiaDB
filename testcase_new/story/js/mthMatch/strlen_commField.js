/************************************
*@Description: use strlen
*@author:      zhaoyu
*@createdate:  2016.10.14
*@testlinkCase:seqDB-10285/seqDB-10286/seqDB-10287/seqDB-10288
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: "zhangsan", major: "math", weight: 18 },
   { No: 2, name: "zhangsan1", major: "math1", weight: 18 }];
   dbcl.insert( doc );

   //seqDB-10285/seqDB-10286
   var condition1 = { name: { $strlen: 1, $et: 8 }, major: { $strlen: 1, $et: 4 } };
   var expRecs1 = [{ No: 1, name: "zhangsan", major: "math", weight: 18 }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   //seqDB-10287
   var condition2 = { name: { $strlen: 1, $et: 8 }, major: { $strlen: 1, $et: 4 }, age: { $strlen: 1, $et: 0 } };
   var expRecs2 = [];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   //seqDB-10288
   condition3 = { name: { $strlen: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition3, null, SDB_INVALIDARG );

   condition4 = { name: { $strlen: 8 } };
   InvalidArgCheck( dbcl, condition4, null, SDB_INVALIDARG );

   //Non-String
   var condition5 = { name: { $strlen: 1, $et: 8 }, major: { $strlen: 1, $et: 4 }, weight: { $strlen: 1, $et: null } };
   var expRecs5 = [{ No: 1, name: "zhangsan", major: "math", weight: 18 }];
   checkResult( dbcl, condition5, null, expRecs5, { No: 1 } );
}
