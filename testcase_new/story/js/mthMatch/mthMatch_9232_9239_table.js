/************************************
*@Description: use and/or, three layer combination,table scan.
*@author:      zhaoyu
*@createdate:  2016.8.11
*@testlinkCase: 
**************************************/
main( test );
function test ()
{



   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );



   //create cl for hash split
   var ClOption = { ShardingKey: { "_id": 1 }, ShardingType: "hash" };
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, ClOption, true, true );

   //split cl
   startCondition = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, COMMCLNAME, startCondition, null );

   //insert data 
   var doc = [{ No: 1, a: 10, b: 100, c: 1001, d: 5001, e: 8001 },
   { No: 2, a: 20, b: 200, c: 1002, d: 5002, e: 8002 },
   { No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 },
   { No: 6, a: 60, b: 600, c: 1006, d: 5006, e: 8006 }];
   dbcl.insert( doc );

   //and-and-and
   var findCondition1 = { $and: [{ $and: [{ $and: [{ a: { $gte: 30 } }, { a: { $lte: 50 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1005 } }] };
   var expRecs1 = [{ No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //and-and-or
   var findCondition2 = { $and: [{ $and: [{ $or: [{ a: { $gte: 50 } }, { a: { $lte: 30 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1005 } }] };
   var expRecs2 = [{ No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //and-or-and
   var findCondition3 = { $and: [{ $or: [{ $and: [{ a: { $gte: 30 } }, { a: { $lte: 50 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $gte: 1002 } }] };
   var expRecs3 = [{ No: 2, a: 20, b: 200, c: 1002, d: 5002, e: 8002 },
   { No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //and-or-or
   var findCondition4 = { $and: [{ $or: [{ $or: [{ a: { $gte: 50 } }, { a: { $lte: 30 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1005 } }] };
   var expRecs4 = [{ No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //or-and-and
   var findCondition5 = { $or: [{ $and: [{ $and: [{ a: { $gte: 30 } }, { a: { $lte: 50 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1003 } }] };
   var expRecs5 = [{ No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //or-and-or
   var findCondition6 = { $or: [{ $and: [{ $or: [{ a: { $gte: 50 } }, { a: { $lte: 30 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1003 } }] };
   var expRecs6 = [{ No: 1, a: 10, b: 100, c: 1001, d: 5001, e: 8001 },
   { No: 2, a: 20, b: 200, c: 1002, d: 5002, e: 8002 },
   { No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   //or-or-and
   var findCondition7 = { $or: [{ $or: [{ $and: [{ a: { $gte: 30 } }, { a: { $lte: 50 } }] }, { b: { $in: [100, 400, 200, 500] } }] }, { c: { $et: 1006 } }] };
   var expRecs7 = [{ No: 1, a: 10, b: 100, c: 1001, d: 5001, e: 8001 },
   { No: 2, a: 20, b: 200, c: 1002, d: 5002, e: 8002 },
   { No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 },
   { No: 6, a: 60, b: 600, c: 1006, d: 5006, e: 8006 }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //or-or-or
   var findCondition8 = { $or: [{ $or: [{ $or: [{ a: { $lte: 10 } }, { a: { $gte: 60 } }] }, { b: { $in: [400] } }] }, { c: { $et: 1005 } }] };
   var expRecs8 = [{ No: 1, a: 10, b: 100, c: 1001, d: 5001, e: 8001 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 },
   { No: 6, a: 60, b: 600, c: 1006, d: 5006, e: 8006 }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );
}

