/************************************
*@Description: index scan,use gt/gte/lt/lte to comapare,
               type is arr/obj,value set typical data;
               use arr/obj element to compare
*@author:      zhaoyu
*@createdate:  2016.8.6
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //create index
   commCreateIndex( dbcl, "a1", { "a.0": 1 } );

   hintCondition = { '': '' };

   //insert data 
   var doc = [{ No: 1, a: [1] },
   { No: 2, a: [2] },
   { No: 3, a: [3] },
   { No: 4, a: [{ 0: 1 }] },
   { No: 5, a: [{ 0: 2 }] },
   { No: 6, a: [{ 0: 3 }] },
   { No: 7, a: { 0: 1 } },
   { No: 8, a: { 0: 2 } },
   { No: 9, a: { 0: 3 } }];
   dbcl.insert( doc );

   //arr or obj element query
   //$gt:2
   var findCondition1 = { "a.0": { $gt: 2 } };
   var expRecs1 = [{ No: 6, a: [{ 0: 3 }] },
   { No: 9, a: { 0: 3 } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //$gte:2
   var findCondition2 = { "a.0": { $gte: 2 } };
   var expRecs2 = [{ No: 5, a: [{ 0: 2 }] },
   { No: 6, a: [{ 0: 3 }] },
   { No: 8, a: { 0: 2 } },
   { No: 9, a: { 0: 3 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //$lt:2
   var findCondition3 = { "a.0": { $lt: 2 } };
   var expRecs3 = [{ No: 4, a: [{ 0: 1 }] },
   { No: 7, a: { 0: 1 } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //$lte:2
   var findCondition4 = { "a.0": { $lte: 2 } };
   var expRecs4 = [{ No: 4, a: [{ 0: 1 }] },
   { No: 5, a: [{ 0: 2 }] },
   { No: 7, a: { 0: 1 } },
   { No: 8, a: { 0: 2 } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );
}

