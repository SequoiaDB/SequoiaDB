/************************************
*@Description: use elemMatch:{arr:{$elemMatch:{fieldName:xx}}}, 
               1.arr is empty
               2.Non-arr
               3.arr Non-existent
               4.field in arr is Non-existent
               5.value is Non-existent
               6.many fields combination
*@author:      zhaoyu
*@createdate:  2016.7.14
*@testlinkCase: seqDB-5601/seqDB-5602/seqDB-5603/seqDB-5604/seqDB-5605/seqDB-5605
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "int", value: 1 }] },
   { No: 3, arr: [{ type: "int", value: 2 }], arr1: [{ type: "float", value: 2.23 }] }];
   dbcl.insert( doc );

   //arr is empty,seqDB-5601
   var selectCondition1 = { arr: { $elemMatch: { value: 1 } } };
   var expRecs1 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "int", value: 1 }] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   //Non-arr,seqDB-5602
   var selectCondition2 = { No: { $elemMatch: { No: 1 } } };
   var expRecs2 = [{ arr: [] },
   { arr: [{ type: "int", value: 1 }] },
   { arr: [{ type: "int", value: 2 }], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   //arr Non-existent,seqDB-5603
   var selectCondition3 = { arr2: { $elemMatch: { No: 1 } } };
   checkResult( dbcl, null, selectCondition3, doc, { No: 1 } );

   //field in arr is Non-existent,seqDB-5604
   var selectCondition4 = { arr: { $elemMatch: { No: 1 } } };
   var expRecs4 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   //value is Non-existent,seqDB-5605
   var selectCondition5 = { arr: { $elemMatch: { value: 3 } } };
   var expRecs5 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   //many fields combination,seqDB-8214
   var selectCondition6 = { arr: { $elemMatch: { value: 1 } }, arr1: { $elemMatch: { value: 2.23 } } };
   var expRecs6 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "int", value: 1 }] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition6, expRecs6, { No: 1 } );

   var selectCondition7 = { arr: { $elemMatch: { value: 1, type: "int" } } };
   var expRecs7 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "int", value: 1 }] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition7, expRecs7, { No: 1 } );

   var selectCondition8 = { arr: { $elemMatch: { value: 1, type: "float" } } };
   var expRecs8 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr: [], arr1: [{ type: "float", value: 2.23 }] }];
   checkResult( dbcl, null, selectCondition8, expRecs8, { No: 1 } );

}