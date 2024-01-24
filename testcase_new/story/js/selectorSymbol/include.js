/************************************
*@Description: use include:{field:<$include:Value>}, 
               1.field exists or not
               2.Value set 0 or !0  
               3.argument is illegal
               4.many fields combination
*@author:      zhaoyu
*@createdate:  2016.7.13
*@testlinkCase:seqDB-5589/seqDB-5590/seqDB-5591/seqDB-5592/seqDB-5593/seqDB-8212
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: "zhangsan", age: 18 },
   { No: 2, name: "lisi", age: 19 },
   { No: 3, name: "wangwu", age: 20 }];
   dbcl.insert( doc );

   //field exists,value set 1 or -1,check result,eqDB-5589
   var findCondition1 = { No: 1 };
   var selectCondition1 = { name: { $include: 1 } };
   var expRecs1 = [{ name: "zhangsan" }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { No: 1 } );

   var selectCondition2 = { name: { $include: -1 } };
   checkResult( dbcl, findCondition1, selectCondition2, expRecs1, { No: 1 } );

   //field exists, value set 0,check result,seqDB-5590
   var selectCondition3 = { name: { $include: 0 } };
   var expRecs3 = [{ No: 1, age: 18 }];
   checkResult( dbcl, findCondition1, selectCondition3, expRecs3, { No: 1 } );

   //field is non-existent,value set 1 or -1,check result,seqDB-5591
   var selectCondition4 = { sex: { $include: 1 } };
   var expRecs4 = [{}, {}, {}];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   var selectCondition5 = { sex: { $include: -1 } };
   checkResult( dbcl, null, selectCondition5, expRecs4, { No: 1 } );

   //field is non-existent, value set 0,check result,seqDB-5592
   var selectCondition6 = { sex: { $include: 0 } };
   checkResult( dbcl, null, selectCondition6, doc, { No: 1 } );

   //argument is illegal,check result,seqDB-5593
   var selectCondition7 = { No: { $include: "test" } };
   InvalidArgCheck( dbcl, null, selectCondition7, -6 );

   var selectCondition8 = { No: { $include: true } };
   InvalidArgCheck( dbcl, null, selectCondition8, -6 );

   //many fields combination,check result,seqDB-8212
   var findCondition9 = { age: 18 };
   var selectCondition9 = { name: { $include: 1 }, age: { $include: 1 } };
   var expRecs9 = [{ name: "zhangsan", age: 18 }];
   checkResult( dbcl, findCondition9, selectCondition9, expRecs9, { No: 1 } );

   var selectCondition10 = { name: { $include: 0 }, age: { $include: 0 } };
   var expRecs10 = [{ No: 1 }];
   checkResult( dbcl, findCondition9, selectCondition10, expRecs10, { No: 1 } );

   var selectCondition11 = { name: { $include: 0 }, age: { $include: 1 } };
   InvalidArgCheck( dbcl, null, selectCondition11, -6 );

   var selectCondition12 = { name: { $include: 1 }, age: { $include: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition12, -6 );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );
}
