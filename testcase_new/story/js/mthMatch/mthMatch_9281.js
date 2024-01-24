/************************************
*@Description: add head tree,
               one field
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
   var doc = [{ No: 1, a: { a1: { a2: 1, a3: 2 } } },
   { No: 2, a: { a1: { a2: 2, a3: 3 } } },
   { No: 3, a: { a1: { a2: 3, a3: 4 } } },
   { No: 4, a: { a1: { a2: 5, a3: 6 } } }];
   dbcl.insert( doc );

   //and
   //the head tree and the match tree have the same order
   var findCondition1 = { $and: [{ "a.a1.a2": { $in: [1, 8, 2] } }, { No: { $nin: [2, 3, 4] } }] };
   var expRecs1 = [{ No: 1, a: { a1: { a2: 1, a3: 2 } } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //the head tree has different order
   var findCondition2 = { $and: [{ No: { $gt: 2 } }, { "a.a1.a2": { $in: [1, 8, 3] } }] };
   var expRecs2 = [{ No: 3, a: { a1: { a2: 3, a3: 4 } } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //the match tree has different order
   var findCondition3 = { $and: [{ "a.a1.a3": { $nin: [1] } }, { No: { $in: [2, 3] } }] };
   var expRecs3 = [{ No: 2, a: { a1: { a2: 2, a3: 3 } } },
   { No: 3, a: { a1: { a2: 3, a3: 4 } } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition4 = { $and: [{ No: { $nin: [2, 5, 1] } }, { "a.a1.a3": { $in: [5, 6, 8] } }] };
   var expRecs4 = [{ No: 4, a: { a1: { a2: 5, a3: 6 } } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );
}
