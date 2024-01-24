/************************************
*@Description: use lower
*@author:      zhaoyu
*@createdate:  2016.10.14
*@testlinkCase:
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: ["ZhaNgsan"], major: ["cHINeSE"] },
   { No: 2, name: ["ZHANGSAN"], major: ["CHINESE"] },
   { No: 3, name: ["zhangsan"], major: ["chinese"] },
   { No: 4, name: ["lisi"], major: ["math"] },
   { No: 5, name: [{ 0: "ZhaNgsan" }], major: [{ 0: "cHINeSE" }] },
   { No: 6, name: [{ 0: "ZHANGSAN" }], major: [{ 0: "CHINESE" }] },
   { No: 7, name: [{ 0: "zhangsan" }], major: [{ 0: "chinese" }] },
   { No: 8, name: [{ 0: "lisi" }], major: [{ 0: "math" }] },
   { No: 9, name: { 0: "ZhaNgsan" }, major: { 0: "cHINeSE" } },
   { No: 10, name: { 0: "ZHANGSAN" }, major: { 0: "CHINESE" } },
   { No: 11, name: { 0: "zhangsan" }, major: { 0: "chinese" } },
   { No: 12, name: { 0: "lisi" }, major: { 0: "math" } }];
   dbcl.insert( doc );

   //seqDB-10289/seqDB-10290
   var condition1 = { name: { $lower: 1, $et: "zhangsan" }, major: { $lower: 1, $et: "chinese" } };
   var expRecs1 = [{ No: 1, name: ["ZhaNgsan"], major: ["cHINeSE"] },
   { No: 2, name: ["ZHANGSAN"], major: ["CHINESE"] },
   { No: 3, name: ["zhangsan"], major: ["chinese"] }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   var condition2 = { "name.0": { $lower: 1, $et: "zhangsan" }, "major.0": { $lower: 1, $et: "chinese" } };
   var expRecs2 = [{ No: 1, name: ["ZhaNgsan"], major: ["cHINeSE"] },
   { No: 2, name: ["ZHANGSAN"], major: ["CHINESE"] },
   { No: 3, name: ["zhangsan"], major: ["chinese"] },
   { No: 5, name: [{ 0: "ZhaNgsan" }], major: [{ 0: "cHINeSE" }] },
   { No: 6, name: [{ 0: "ZHANGSAN" }], major: [{ 0: "CHINESE" }] },
   { No: 7, name: [{ 0: "zhangsan" }], major: [{ 0: "chinese" }] },
   { No: 9, name: { 0: "ZhaNgsan" }, major: { 0: "cHINeSE" } },
   { No: 10, name: { 0: "ZHANGSAN" }, major: { 0: "CHINESE" } },
   { No: 11, name: { 0: "zhangsan" }, major: { 0: "chinese" } }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   //seqDB-10293
   var condition3 = { name: { $upper: 1, $et: "ZHANGSAN" }, major: { $upper: 1, $et: "CHINESE" } };
   var expRecs3 = [{ No: 1, name: ["ZhaNgsan"], major: ["cHINeSE"] },
   { No: 2, name: ["ZHANGSAN"], major: ["CHINESE"] },
   { No: 3, name: ["zhangsan"], major: ["chinese"] }];
   checkResult( dbcl, condition3, null, expRecs3, { No: 1 } );

   var condition4 = { "name.0": { $upper: 1, $et: "ZHANGSAN" }, "major.0": { $upper: 1, $et: "CHINESE" } };
   var expRecs4 = [{ No: 1, name: ["ZhaNgsan"], major: ["cHINeSE"] },
   { No: 2, name: ["ZHANGSAN"], major: ["CHINESE"] },
   { No: 3, name: ["zhangsan"], major: ["chinese"] },
   { No: 5, name: [{ 0: "ZhaNgsan" }], major: [{ 0: "cHINeSE" }] },
   { No: 6, name: [{ 0: "ZHANGSAN" }], major: [{ 0: "CHINESE" }] },
   { No: 7, name: [{ 0: "zhangsan" }], major: [{ 0: "chinese" }] },
   { No: 9, name: { 0: "ZhaNgsan" }, major: { 0: "cHINeSE" } },
   { No: 10, name: { 0: "ZHANGSAN" }, major: { 0: "CHINESE" } },
   { No: 11, name: { 0: "zhangsan" }, major: { 0: "chinese" } }];
   checkResult( dbcl, condition4, null, expRecs4, { No: 1 } );
}

