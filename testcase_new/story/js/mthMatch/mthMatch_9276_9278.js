/************************************
*@Description: add head tree,
               one field use two times
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
   var doc = [{ No: 1, abc: 1000, abd: 100 },
   { No: 2, abc: 1001, abd: 101 },
   { No: 3, abc: 1002, abd: 102 },
   { No: 4, abc: 1001, abd: 100 }];
   dbcl.insert( doc );

   //and
   //the head tree and the match tree have the same order
   var findCondition1 = { $and: [{ abc: { $lt: 1002 } }, { abd: { $lte: 101 } }, { abd: { $gt: 100 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 103 } }, { abc: { $gte: 1000 } }] };
   var expRecs1 = [{ No: 2, abc: 1001, abd: 101 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //the head tree has different order
   var findCondition2 = { $and: [{ abd: { $lte: 101 } }, { abc: { $lt: 1002 } }, { abd: { $gt: 100 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 103 } }, { abc: { $gte: 1000 } }] };
   checkResult( dbcl, findCondition2, null, expRecs1, { No: 1 } );

   //the match tree has different order
   var findCondition3 = { $and: [{ abc: { $gte: 1000 } }, { abd: { $gt: 100 } }, { abd: { $lte: 101 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 103 } }, { abc: { $lt: 1002 } }] };
   checkResult( dbcl, findCondition3, null, expRecs1, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition4 = { $and: [{ abd: { $gt: 100 } }, { abc: { $gte: 1000 } }, { abc: { $lt: 1002 } }, { abd: { $lte: 101 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 103 } }] };
   checkResult( dbcl, findCondition4, null, expRecs1, { No: 1 } );

   //or
   //the head tree and the match tree have the same order
   var findCondition5 = { $or: [{ abc: { $gt: 1001 } }, { abd: { $gte: 101 } }, { abc: { $lte: 1000 } }, { abd: { $et: 100 } }] };
   var expRecs5 = [{ No: 1, abc: 1000, abd: 100 },
   { No: 2, abc: 1001, abd: 101 },
   { No: 3, abc: 1002, abd: 102 },
   { No: 4, abc: 1001, abd: 100 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //the head tree has different order
   var findCondition6 = { $or: [{ abd: { $gte: 101 } }, { abc: { $gt: 1001 } }, { abc: { $lte: 1000 } }, { abd: { $et: 100 } }] };
   checkResult( dbcl, findCondition6, null, expRecs5, { No: 1 } );

   //the match tree has different order
   var findCondition7 = { $or: [{ abc: { $lte: 1000 } }, { abd: { $lt: 101 } }, { abc: { $gt: 1001 } }, { abd: { $gte: 101 } }] };
   checkResult( dbcl, findCondition7, null, expRecs5, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition8 = { $or: [{ abd: { $gte: 101 } }, { abc: { $gt: 1001 } }, { abc: { $lte: 1000 } }, { abd: { $lt: 101 } }] };
   checkResult( dbcl, findCondition8, null, expRecs5, { No: 1 } );

   //not
   //the head tree and the match tree have the same order
   var findCondition9 = { $not: [{ abc: { $gt: 1000 } }, { abd: { $lte: 101 } }, { abc: { $lte: 1001 } }, { abd: { $gt: 100 } }] };
   var expRecs9 = [{ No: 1, abc: 1000, abd: 100 },
   { No: 3, abc: 1002, abd: 102 },
   { No: 4, abc: 1001, abd: 100 }];
   checkResult( dbcl, findCondition9, null, expRecs9, { No: 1 } );

   //the head tree has different order
   var findCondition10 = { $not: [{ abd: { $gt: 100 } }, { abc: { $lte: 1001 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 101 } }] };
   checkResult( dbcl, findCondition10, null, expRecs9, { No: 1 } );

   //the match tree has different order
   var findCondition11 = { $not: [{ abc: { $lte: 1001 } }, { abd: { $gt: 100 } }, { abc: { $gt: 1000 } }, { abd: { $lte: 101 } }] };
   checkResult( dbcl, findCondition11, null, expRecs9, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition12 = { $not: [{ abd: { $lte: 101 } }, { abc: { $gt: 1000 } }, { abd: { $gt: 100 } }, { abc: { $lte: 1001 } }] };
   checkResult( dbcl, findCondition12, null, expRecs9, { No: 1 } );
}
