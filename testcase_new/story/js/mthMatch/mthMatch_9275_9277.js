/************************************
*@Description: add head tree,
               one field ues one times
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
   var doc = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 2, abc: "lisi", abd: 101 },
   { No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   dbcl.insert( doc );

   //and
   //the head tree and the match tree have the same order
   var findCondition1 = { abc: { $type: 1, $et: 2 }, abd: { $lte: 102 } };
   var expRecs1 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 2, abc: "lisi", abd: 101 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //the head tree has different order
   var findCondition2 = { abd: { $gt: 101 }, abc: { $exists: 1 } };
   var expRecs2 = [{ No: 3, abc: [1, 2, 3], abd: 102 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //the match tree has different order
   var findCondition3 = { abc: { $type: 1, $et: 2 }, abd: { $et: 101 } };
   var expRecs3 = [{ No: 2, abc: "lisi", abd: 101 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition4 = { abd: { $ne: 101 }, abc: { $exists: 1 } };
   var expRecs4 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 3, abc: [1, 2, 3], abd: 102 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //or
   //the head tree and the match tree have the same order
   var findCondition5 = { $or: [{ abc: { $type: 1, $et: 2 } }, { abd: { $lte: 102 } }] };
   var expRecs5 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 2, abc: "lisi", abd: 101 },
   { No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //the head tree has different order
   var findCondition6 = { $or: [{ abd: { $gt: 101 } }, { abc: { $exists: 0 } }] };
   var expRecs6 = [{ No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   //the match tree has different order
   var findCondition7 = { $or: [{ abc: { $type: 1, $et: 2 } }, { abd: { $et: 101 } }] };
   var expRecs7 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 2, abc: "lisi", abd: 101 }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition8 = { $or: [{ abd: { $ne: 101 } }, { abc: { $exists: 0 } }] };
   var expRecs8 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );

   //not
   //the head tree and the match tree have the same order
   var findCondition9 = { $not: [{ abc: { $type: 1, $et: 2 } }, { abd: { $lte: 102 } }] };
   var expRecs9 = [{ No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition9, null, expRecs9, { No: 1 } );

   //the head tree has different order
   var findCondition10 = { $not: [{ abd: { $gt: 101 } }, { abc: { $exists: 1 } }] };
   var expRecs10 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 2, abc: "lisi", abd: 101 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition10, null, expRecs10, { No: 1 } );

   //the match tree has different order
   var findCondition11 = { $not: [{ abc: { $type: 1, $et: 2 } }, { abd: { $et: 101 } }] };
   var expRecs11 = [{ No: 1, abc: "zhangsan", abd: 100 },
   { No: 3, abc: [1, 2, 3], abd: 102 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition11, null, expRecs11, { No: 1 } );

   //the head tree and the match tree have different order
   var findCondition12 = { $not: [{ abd: { $ne: 101 } }, { abc: { $exists: 1 } }] };
   var expRecs12 = [{ No: 2, abc: "lisi", abd: 101 },
   { No: 4, abd: 102 }];
   checkResult( dbcl, findCondition12, null, expRecs12, { No: 1 } );
}
