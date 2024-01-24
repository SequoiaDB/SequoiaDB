/************************************
*@Description: use substr:{field:[<xx>,xx]}, 
               1.value1>0,value1+value2<=str.length;seqDB-5680
               2.value1>0,value1+value2<=str.length;seqDB-5679
               3.value1>0,value1+value2>str.length;seqDB-5681
               4.value1>str.length;seqDB-5682
               5.value1<0,|value1|+value2<=str.length;seqDB-5683
               6.value1<0,|value1|+value2>str.length;seqDB-5684
               7.value1<0,|value1|>str.length;seqDB-5685
               8.empty string;seqDB-5686/seqDB-5688
               9.field is Non-string;seqDB-5687
               10.field is Non-existent;seqDB-5689
               11.two fields use substr;seqDB-8225
               12.value is illegal,seqDB-5867
*@author:      zhaoyu
*@createdate:  2016.7.19
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
   var doc = [{ No: 1, a: "abcdefghijklmnopqrstuvwxyz", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   dbcl.insert( doc );

   //value1>0,value1+value2<=str.length;seqDB-5680
   var selectCondition1 = { a: { $substr: [2, 5] } };
   var expRecs1 = [{ No: 1, a: "cdefg", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   //value1>0,value1+value2<=str.length;seqDB-5679
   var selectCondition2 = { a: { $substr: 5 } };
   var expRecs2 = [{ No: 1, a: "abcde", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   //value1>0,value1+value2>str.length;seqDB-5681
   var selectCondition3 = { a: { $substr: [5, 22] } };
   var expRecs3 = [{ No: 1, a: "fghijklmnopqrstuvwxyz", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { No: 1 } );

   //value1>str.length;seqDB-5682
   var selectCondition4 = { a: { $substr: [26, 1] } };
   var expRecs4 = [{ No: 1, a: "", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   //value1<0,|value1|+value2<=str.length;seqDB-5683
   var selectCondition5 = { a: { $substr: [-22, 10] } };
   var expRecs5 = [{ No: 1, a: "efghijklmn", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   //value1<0,|value1|+value2>str.length;seqDB-5684
   var selectCondition6 = { a: { $substr: [-2, 10] } };
   var expRecs6 = [{ No: 1, a: "yz", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition6, expRecs6, { No: 1 } );

   //value1<0,|value1|>str.length;seqDB-5685
   var selectCondition7 = { a: { $substr: [-27, 5] } };
   var expRecs7 = [{ No: 1, a: "", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition7, expRecs7, { No: 1 } );

   //empty string;seqDB-5686/seqDB-5688
   var selectCondition8 = { b: { $substr: [1, 5] } };
   var expRecs8 = [{ No: 1, a: "abcdefghijklmnopqrstuvwxyz", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition8, expRecs8, { No: 1 } );

   //field is Non-string;seqDB-5687
   var selectCondition9 = { c: { $substr: [1, 5] } };
   var expRecs9 = [{ No: 1, a: "abcdefghijklmnopqrstuvwxyz", b: "", c: null, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition9, expRecs9, { No: 1 } );

   //field is Non-existent;seqDB-5689
   var selectCondition10 = { d: { $substr: [1, 5] } };
   var expRecs10 = [{ No: 1, a: "abcdefghijklmnopqrstuvwxyz", b: "", c: 123, e: "abcdefghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition10, expRecs10, { No: 1 } );

   //two fields use substr;seqDB-8225
   var selectCondition11 = { a: { $substr: [1, 5] }, e: { $substr: [-11, 5] } };
   var expRecs11 = [{ No: 1, a: "bcdef", b: "", c: 123, e: "pqrst" }];
   checkResult( dbcl, null, selectCondition11, expRecs11, { No: 1 } );

   //value is illegal,seqDB-5867
   var selectCondition12 = { a: { $substr: [] } };
   InvalidArgCheck( dbcl, null, selectCondition12, -6 );

   //value2 set negative;
   var selectCondition12 = { a: { $substr: [3, -4] }, e: { $substr: [3, -30] } };
   var expRecs12 = [{ No: 1, a: "defghijklmnopqrstuvwxyz", b: "", c: 123, e: "defghijklmnopqrstuvwxyz" }];
   checkResult( dbcl, null, selectCondition12, expRecs12, { No: 1 } );

   var selectCondition14 = { a: { $substr: -3 }, e: { $substr: -30 } };
   var expRecs14 = [{ No: 1, a: "xyz", b: "", c: 123, e: "" }];
   checkResult( dbcl, null, selectCondition14, expRecs14, { No: 1 } );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );

   //arr
   var doc1 = [{ No: 2, a: [" \n\t\rString\n\t "] }];
   dbcl.insert( doc1 );
   var selectCondition15 = { a: { $substr: -3 }, e: { $substr: -30 } };
   var expRecs15 = [{ No: 1, a: "xyz", b: "", c: 123, e: "" },
   { No: 2, a: ["\n\t "] }];
   checkResult( dbcl, null, selectCondition15, expRecs15, { No: 1 } );

}