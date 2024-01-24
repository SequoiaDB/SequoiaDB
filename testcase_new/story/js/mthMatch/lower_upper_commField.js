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
   var doc = [{ No: 1, name: "ZhaNgsan", major: "cHINeSE", weight: 51 },
   { No: 2, name: "ZHANGSAN", major: "CHINESE", weight: 51 },
   { No: 3, name: "zhangsan", major: "chinese", weight: 51 },
   { No: 4, name: "lisi", major: "math", weight: 51 }];
   dbcl.insert( doc );

   //seqDB-10289/seqDB-10290
   var condition1 = { name: { $lower: 1, $et: "zhangsan" }, major: { $lower: 1, $et: "chinese" } };
   var expRecs1 = [{ No: 1, name: "ZhaNgsan", major: "cHINeSE", weight: 51 },
   { No: 2, name: "ZHANGSAN", major: "CHINESE", weight: 51 },
   { No: 3, name: "zhangsan", major: "chinese", weight: 51 }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   //seqDB-10291
   var condition2 = { name: { $lower: 1, $et: "zhangsan" }, major: { $lower: 1, $et: "chinese" }, age: { $lower: 1, $et: "age" } };
   var expRecs2 = [];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   //seqDB-10292
   condition3 = { name: { $lower: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition3, null, SDB_INVALIDARG );

   condition4 = { name: { $lower: 1 } };
   InvalidArgCheck( dbcl, condition4, null, SDB_INVALIDARG );

   //seqDB-10293/seqDB-10294
   var condition5 = { name: { $upper: 1, $et: "ZHANGSAN" }, major: { $upper: 1, $et: "CHINESE" } };
   var expRecs5 = [{ No: 1, name: "ZhaNgsan", major: "cHINeSE", weight: 51 },
   { No: 2, name: "ZHANGSAN", major: "CHINESE", weight: 51 },
   { No: 3, name: "zhangsan", major: "chinese", weight: 51 }];
   checkResult( dbcl, condition5, null, expRecs5, { No: 1 } );

   //seqDB-10291
   var condition6 = { name: { $upper: 1, $et: "zhangsan" }, major: { $upper: 1, $et: "chinese" }, age: { $upper: 1, $et: "age" } };
   var expRecs6 = [];
   checkResult( dbcl, condition6, null, expRecs6, { No: 1 } );

   //seqDB-10292
   condition7 = { name: { $upper: "a", $et: 8 } };
   InvalidArgCheck( dbcl, condition7, null, SDB_INVALIDARG );

   condition8 = { name: { $upper: 1 } };
   InvalidArgCheck( dbcl, condition8, null, SDB_INVALIDARG );

   //Non-String
   var condition9 = { name: { $lower: 1, $et: "zhangsan" }, major: { $lower: 1, $et: "chinese" }, weight: { $lower: 1, $et: null } };
   var expRecs9 = [{ No: 1, name: "ZhaNgsan", major: "cHINeSE", weight: 51 },
   { No: 2, name: "ZHANGSAN", major: "CHINESE", weight: 51 },
   { No: 3, name: "zhangsan", major: "chinese", weight: 51 }];
   checkResult( dbcl, condition9, null, expRecs9, { No: 1 } );

   var condition10 = { name: { $upper: 1, $et: "ZHANGSAN" }, major: { $upper: 1, $et: "CHINESE" }, weight: { $upper: 1, $et: null } };
   var expRecs10 = [{ No: 1, name: "ZhaNgsan", major: "cHINeSE", weight: 51 },
   { No: 2, name: "ZHANGSAN", major: "CHINESE", weight: 51 },
   { No: 3, name: "zhangsan", major: "chinese", weight: 51 }];
   checkResult( dbcl, condition10, null, expRecs10, { No: 1 } );
}

