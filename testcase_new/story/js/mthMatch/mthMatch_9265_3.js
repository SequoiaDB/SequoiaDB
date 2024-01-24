/************************************
*@Description: index scan,use gt/gte/lt/lte to comapare,
               arr field use many times,use and/or/not
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
   var doc = [{ No: 1, a: [500] }, { No: 2, a: [1000] }, { No: 3, a: [1500] }, { No: 4, a: [2000] }, { No: 5, a: [2500] }, { No: 6, a: [3000] }, { No: 7, a: [3500] },
   { No: 8, a: { 0: 500 } }, { No: 9, a: { 0: 1000 } }, { No: 10, a: { 0: 1500 } }, { No: 11, a: { 0: 2000 } }, { No: 12, a: { 0: 2500 } }, { No: 13, a: { 0: 3000 } }, { No: 14, a: { 0: 3500 } },
   { No: 15, a: [{ 0: 500 }] }, { No: 16, a: [{ 0: 1000 }] }, { No: 17, a: [{ 0: 1500 }] }, { No: 18, a: [{ 0: 2000 }] }, { No: 19, a: [{ 0: 2500 }] }, { No: 20, a: [{ 0: 3000 }] }, { No: 21, a: [{ 0: 3500 }] }];
   dbcl.insert( doc );

   //and
   //gt
   var findCondition1 = { $and: [{ "a.0": { $gt: 1000 } }, { "a.0": { $gt: 2000 } }, { "a.0": { $gt: 3000 } }] };
   var expRecs1 = [{ No: 14, a: { 0: 3500 } },
   { No: 21, a: [{ 0: 3500 }] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $and: [{ "a.0": { $gt: 1000 } }, { "a.0": { $gt: 3000 } }, { "a.0": { $gt: 2000 } }] };
   checkResult( dbcl, findCondition2, null, expRecs1, { No: 1 } );

   var findCondition3 = { $and: [{ "a.0": { $gt: 2000 } }, { "a.0": { $gt: 3000 } }, { "a.0": { $gt: 1000 } }] };
   checkResult( dbcl, findCondition3, null, expRecs1, { No: 1 } );

   var findCondition4 = { $and: [{ "a.0": { $gt: 2000 } }, { "a.0": { $gt: 1000 } }, { "a.0": { $gt: 3000 } }] };
   checkResult( dbcl, findCondition4, null, expRecs1, { No: 1 } );

   var findCondition5 = { $and: [{ "a.0": { $gt: 3000 } }, { "a.0": { $gt: 2000 } }, { "a.0": { $gt: 1000 } }] };
   checkResult( dbcl, findCondition5, null, expRecs1, { No: 1 } );

   var findCondition6 = { $and: [{ "a.0": { $gt: 3000 } }, { "a.0": { $gt: 1000 } }, { "a.0": { $gt: 2000 } }] };
   checkResult( dbcl, findCondition6, null, expRecs1, { No: 1 } );

   //gte and
   var findCondition7 = { $and: [{ "a.0": { $gte: 3000 } }, { "a.0": { $gte: 2000 } }, { "a.0": { $gte: 1000 } }] };
   var expRecs2 = [{ No: 13, a: { 0: 3000 } }, { No: 14, a: { 0: 3500 } },
   { No: 20, a: [{ 0: 3000 }] }, { No: 21, a: [{ 0: 3500 }] }];
   checkResult( dbcl, findCondition7, null, expRecs2, { No: 1 } );

   var findCondition8 = { $and: [{ "a.0": { $gte: 1000 } }, { "a.0": { $gte: 2000 } }, { "a.0": { $gte: 3000 } }] };
   checkResult( dbcl, findCondition8, null, expRecs2, { No: 1 } );

   var findCondition9 = { $and: [{ "a.0": { $gte: 1000 } }, { "a.0": { $gte: 3000 } }, { "a.0": { $gte: 2000 } }] };
   checkResult( dbcl, findCondition9, null, expRecs2, { No: 1 } );

   var findCondition10 = { $and: [{ "a.0": { $gte: 2000 } }, { "a.0": { $gte: 1000 } }, { "a.0": { $gte: 3000 } }] };
   checkResult( dbcl, findCondition10, null, expRecs2, { No: 1 } );

   var findCondition11 = { $and: [{ "a.0": { $gte: 2000 } }, { "a.0": { $gte: 3000 } }, { "a.0": { $gte: 1000 } }] };
   checkResult( dbcl, findCondition11, null, expRecs2, { No: 1 } );

   var findCondition12 = { $and: [{ "a.0": { $gte: 3000 } }, { "a.0": { $gte: 1000 } }, { "a.0": { $gte: 2000 } }] };
   checkResult( dbcl, findCondition12, null, expRecs2, { No: 1 } );

   //lt and
   var findCondition13 = { $and: [{ "a.0": { $lt: 1000 } }, { "a.0": { $lt: 2000 } }, { "a.0": { $lt: 3000 } }] };
   var expRecs3 = [{ No: 8, a: { 0: 500 } },
   { No: 15, a: [{ 0: 500 }] }];
   checkResult( dbcl, findCondition13, null, expRecs3, { No: 1 } );

   var findCondition14 = { $and: [{ "a.0": { $lt: 1000 } }, { "a.0": { $lt: 3000 } }, { "a.0": { $lt: 2000 } }] };
   checkResult( dbcl, findCondition14, null, expRecs3, { No: 1 } );

   var findCondition15 = { $and: [{ "a.0": { $lt: 2000 } }, { "a.0": { $lt: 3000 } }, { "a.0": { $lt: 1000 } }] };
   checkResult( dbcl, findCondition15, null, expRecs3, { No: 1 } );

   var findCondition16 = { $and: [{ "a.0": { $lt: 2000 } }, { "a.0": { $lt: 1000 } }, { "a.0": { $lt: 3000 } }] };
   checkResult( dbcl, findCondition16, null, expRecs3, { No: 1 } );

   var findCondition17 = { $and: [{ "a.0": { $lt: 3000 } }, { "a.0": { $lt: 2000 } }, { "a.0": { $lt: 1000 } }] };
   checkResult( dbcl, findCondition17, null, expRecs3, { No: 1 } );

   var findCondition18 = { $and: [{ "a.0": { $lt: 3000 } }, { "a.0": { $lt: 1000 } }, { "a.0": { $lt: 2000 } }] };
   checkResult( dbcl, findCondition18, null, expRecs3, { No: 1 } );

   //lte and
   var findCondition19 = { $and: [{ "a.0": { $lte: 1000 } }, { "a.0": { $lte: 2000 } }, { "a.0": { $lte: 3000 } }] };
   var expRecs4 = [{ No: 8, a: { 0: 500 } }, { No: 9, a: { 0: 1000 } },
   { No: 15, a: [{ 0: 500 }] }, { No: 16, a: [{ 0: 1000 }] }];
   checkResult( dbcl, findCondition19, null, expRecs4, { No: 1 } );

   var findCondition20 = { $and: [{ "a.0": { $lte: 1000 } }, { "a.0": { $lte: 3000 } }, { "a.0": { $lte: 2000 } }] };
   checkResult( dbcl, findCondition20, null, expRecs4, { No: 1 } );

   var findCondition21 = { $and: [{ "a.0": { $lte: 2000 } }, { "a.0": { $lte: 1000 } }, { "a.0": { $lte: 3000 } }] };
   checkResult( dbcl, findCondition21, null, expRecs4, { No: 1 } );

   var findCondition22 = { $and: [{ "a.0": { $lte: 2000 } }, { "a.0": { $lte: 3000 } }, { "a.0": { $lte: 1000 } }] };
   checkResult( dbcl, findCondition22, null, expRecs4, { No: 1 } );

   var findCondition23 = { $and: [{ "a.0": { $lte: 3000 } }, { "a.0": { $lte: 2000 } }, { "a.0": { $lte: 1000 } }] };
   checkResult( dbcl, findCondition23, null, expRecs4, { No: 1 } );

   var findCondition24 = { $and: [{ "a.0": { $lte: 3000 } }, { "a.0": { $lte: 1000 } }, { "a.0": { $lte: 2000 } }] };
   checkResult( dbcl, findCondition24, null, expRecs4, { No: 1 } );
}

