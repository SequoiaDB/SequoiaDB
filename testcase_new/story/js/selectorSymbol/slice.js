/************************************
*@Description: use slice:{arr:{$slice:[value1,value2]}}
               1.only set value2, seqDB-5614/seqDB-5621/seqDB-5623
               2.set value1>0 and value1+value2<=arr.length, seqDB-5615 
               3.set value1>0 and value1+value2>arr.length, seqDB-5616 
               4.set value1>0 and value1>arr.length, seqDB-5617
               5.set value1<0 and value1+value2<=arr.length, seqDB-5618
               6.set value1<0 and value1+value2>arr.length, seqDB-5619
               7.set value1<0 and value1>arr.length, seqDB-5620
               8.value1 and value2 is float, seqDB-5622
               9.arr is Non-existent, seqDB-5624
               10.many fields combination, seqDB-8216
               11.value2 is illegal,seqDB-5842
*@author:      zhaoyu
*@createdate:  2016.7.14
*@testlinkCase: 
**************************************/
main( test );

function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, arr: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   dbcl.insert( doc );

   //only set value2, seqDB-5614/seqDB-5621/seqDB-5623
   var selectCondition1 = { arr: { $slice: 3 } };
   var expRecs1 = [{ No: 1, arr: [1, 2, 3] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   //set value1>0 and value1+value2<=arr.length, seqDB-5615 
   var selectCondition2 = { arr: { $slice: [3, 4] } };
   var expRecs2 = [{ No: 1, arr: [4, 5, 6, 7] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   //set value1>0 and value1+value2>arr.length, seqDB-5616 
   var selectCondition3 = { arr: { $slice: [3, 7] } };
   var expRecs3 = [{ No: 1, arr: [4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { No: 1 } );

   //set value1>0 and value1>arr.length, seqDB-5617
   var selectCondition4 = { arr: { $slice: [9, 1] } };
   var expRecs4 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   //set value1<0 and value1+value2<=arr.length, seqDB-5618
   var selectCondition5 = { arr: { $slice: [-7, 7] } };
   var expRecs5 = [{ No: 1, arr: [3, 4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   //set value1<0 and value1+value2>arr.length, seqDB-5619
   var selectCondition6 = { arr: { $slice: [-7, 8] } };
   var expRecs6 = [{ No: 1, arr: [3, 4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition6, expRecs6, { No: 1 } );

   //set value1<0 and value1>arr.length, seqDB-5620
   var selectCondition7 = { arr: { $slice: [-10, 2] } };
   var expRecs7 = [{ No: 1, arr: [1, 2] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition7, expRecs7, { No: 1 } );

   var selectCondition8 = { arr: { $slice: [-10, 10] } };
   var expRecs8 = [{ No: 1, arr: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition8, expRecs8, { No: 1 } );

   //value1 and value2 is float, seqDB-5622
   var selectCondition9 = { arr: { $slice: [3.5, 4.5] } };
   var expRecs9 = [{ No: 1, arr: [4, 5, 6, 7] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition9, expRecs9, { No: 1 } );

   //arr is Non-existent, seqDB-5624
   var selectCondition10 = { arr4: { $slice: [3, 4] } };
   checkResult( dbcl, null, selectCondition10, doc, { No: 1 } );

   //many fields combination, seqDB-8216
   var selectCondition11 = { arr1: { $slice: [3, 4] }, arr2: { $slice: [-7, 4] } };
   var expRecs11 = [{ No: 1, arr: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [4, 5, 6, 7], arr2: [33, 44, 55, 66] }];
   checkResult( dbcl, null, selectCondition11, expRecs11, { No: 1 } );

   //value2 is illegal,seqDB-5842
   var selectCondition12 = { arr: { $slice: [3, -4] } };
   var expRecs12 = [{ No: 1, arr: [4, 5, 6, 7, 8, 9] },
   { No: 2, arr: [] },
   { No: 3, arr1: [1, 2, 3, 4, 5, 6, 7, 8, 9], arr2: [11, 22, 33, 44, 55, 66, 77, 88, 99] }];
   checkResult( dbcl, null, selectCondition12, expRecs12, { No: 1 } );

   var selectCondition12 = { arr: { $slice: [] } };
   InvalidArgCheck( dbcl, null, selectCondition12, -6 );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );
}