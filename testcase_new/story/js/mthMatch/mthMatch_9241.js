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

   //create index
   commCreateIndex( dbcl, "a", { a: -1, b: 1, c: -1 } );

   //insert data 
   var doc = [{ No: 1, a: 10, b: 100, c: 1001, d: 5001, e: 8001 },
   { No: 2, a: 20, b: 200, c: 1002, d: 5002, e: 8002 },
   { No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 },
   { No: 6, a: 60, b: 600, c: 1006, d: 5006, e: 8006 }];
   dbcl.insert( doc );

   //check result
   var findCondition1 = { a: { $gt: 20 } };
   var expRecs1 = [{ No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 },
   { No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 },
   { No: 5, a: 50, b: 500, c: 1005, d: 5005, e: 8005 },
   { No: 6, a: 60, b: 600, c: 1006, d: 5006, e: 8006 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $and: [{ a: { $gt: 20 } }, { b: { $lte: 300 } }] };
   var expRecs2 = [{ No: 3, a: 30, b: 300, c: 1003, d: 5003, e: 8003 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $and: [{ a: { $gt: 20 } }, { b: { $lte: 500 } }, { c: { $et: 1004 } }] };
   var expRecs3 = [{ No: 4, a: 40, b: 400, c: 1004, d: 5004, e: 8004 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

}

