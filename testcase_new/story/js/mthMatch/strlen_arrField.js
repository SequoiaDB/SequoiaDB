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
   var doc = [{ No: 1, name: ["zhangsan"], major: ["math"] },
   { No: 2, name: ["zhangsan1"], major: ["math1"] },
   { No: 1, name: [{ 0: "zhangsan" }], major: [{ 0: "math" }] },
   { No: 2, name: [{ 0: "zhangsan1" }], major: [{ 0: "math1" }] },
   { No: 1, name: { 0: "zhangsan" }, major: { 0: "math" } },
   { No: 2, name: { 0: "zhangsan1" }, major: { 0: "math1" } }];
   dbcl.insert( doc );

   //seqDB-10285/seqDB-10286
   var condition1 = { name: { $strlen: 1, $et: 8 }, major: { $strlen: 1, $et: 4 } };
   var expRecs1 = [{ No: 1, name: ["zhangsan"], major: ["math"] }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   var condition2 = { "name.0": { $strlen: 1, $et: 8 }, "major.0": { $strlen: 1, $et: 4 } };
   var expRecs2 = [{ No: 1, name: ["zhangsan"], major: ["math"] },
   { No: 1, name: [{ 0: "zhangsan" }], major: [{ 0: "math" }] },
   { No: 1, name: { 0: "zhangsan" }, major: { 0: "math" } }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );
}
