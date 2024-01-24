/************************************
*@Description: add head tree,
               use many field to find
               use obj element
               match tree and match condition have different order.
               head tree and string comparison of the field name have different order.
*@author:      zhaoyu
*@createdate:  2016.8.8
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
   var doc = [{ No: 1, a: { a1: { a2: 1, a3: 2 } } }, { No: 2, a: { a1: { a2: 1, b3: 2 } } }, { No: 3, c: { c1: { a2: 1, a3: 2 } } }, { No: 4, a: { a1: { a2: { a3: { a4: { a2: 1, a3: 2 } } }, b1: { b2: { b3: { b5: 1, b6: 2 }, b4: { b7: 1, b8: 2 } }, c1: { c2: 1, c3: 2 } } } } },
   { No: 5, a: { a1: { a2: 2, a3: 3 } } }, { No: 6, a: { a1: { a2: 2, b3: 3 } } }, { No: 7, c: { c1: { a2: 2, a3: 3 } } }, { No: 8, a: { a1: { a2: { a3: { a4: { a2: 2, a3: 3 } } }, b1: { b2: { b3: { b5: 2, b6: 3 }, b4: { b7: 2, b8: 3 } }, c1: { c2: 2, c3: 3 } } } } },
   { No: 9, a: { a1: { a2: 3, a3: 4 } } }, { No: 10, a: { a1: { a2: 3, b3: 4 } } }, { No: 11, c: { c1: { a2: 3, a3: 4 } } }, { No: 12, a: { a1: { a2: { a3: { a4: { a2: 3, a3: 4 } } }, b1: { b2: { b3: { b5: 3, b6: 4 }, b4: { b7: 3, b8: 4 } }, c1: { c2: 3, c3: 4 } } } } },
   { No: 13, a: { a1: { a2: 5, a3: 6 } } }, { No: 14, a: { a1: { a2: 5, b3: 6 } } }, { No: 15, c: { c1: { a2: 5, a3: 6 } } }, { No: 16, a: { a1: { a2: { a3: { a4: { a2: 5, a3: 6 } } }, b1: { b2: { b3: { b5: 5, b6: 6 }, b4: { b7: 5, b8: 6 } }, c1: { c2: 5, c3: 6 } } } } }];
   dbcl.insert( doc );

   //check result
   var findCondition1 = { "a.a1.a2": 3 };
   var expRecs1 = [{ No: 9, a: { a1: { a2: 3, a3: 4 } } },
   { No: 10, a: { a1: { a2: 3, b3: 4 } } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { "a.a1.b3": 4 };
   var expRecs2 = [{ No: 10, a: { a1: { a2: 3, b3: 4 } } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { "a.a1.a2.a3.a4.a2": 3 };
   var expRecs3 = [{ No: 12, a: { a1: { a2: { a3: { a4: { a2: 3, a3: 4 } } }, b1: { b2: { b3: { b5: 3, b6: 4 }, b4: { b7: 3, b8: 4 } }, c1: { c2: 3, c3: 4 } } } } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { "a.a1.b1.c1.c2": 3 };
   var expRecs4 = [{ No: 12, a: { a1: { a2: { a3: { a4: { a2: 3, a3: 4 } } }, b1: { b2: { b3: { b5: 3, b6: 4 }, b4: { b7: 3, b8: 4 } }, c1: { c2: 3, c3: 4 } } } } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );
}
