/************************************
*@Description: main-sub table use size,
*@author:      zhaoyu
*@createdate:  2016.11.18
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
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl10489";
   subCL_Name1 = CHANGEDPREFIX + "_subcl104891";
   subCL_Name2 = CHANGEDPREFIX + "_subcl104892";
   subCL_Name3 = CHANGEDPREFIX + "_subcl104893";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );





   //create maincl for range split
   var mainCLOption = { ShardingKey: { "No": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "No": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "No": 1 }, ShardingType: "hash" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { No: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { No: 1 }, UpBound: { No: 3 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { No: 3 }, UpBound: { No: 6 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { No: 6 }, UpBound: { No: 11 } } );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert data 
   var doc = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   dbcl.insert( doc );

   //gt
   var findCondition1 = { a: { $size: 1, $gt: 0 }, b: { $size: 1, $mod: [2, 1] } };
   var expRecs1 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //use in/nin/all
   var findCondition2 = { a: { $size: 1, $in: [0, 1, 10, 2, 3, 4, 5] }, b: { $size: 1, $in: [0, 2, 4] } };
   var expRecs2 = [{ No: 2, a: [1, 2], b: [1, 2] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { a: { $size: 1, $nin: [7, 8, 2] }, b: { $size: 1, $nin: [5, 3, -1] } };
   var expRecs3 = [{ No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { a: { $size: 1, $all: [16, 2] } };
   var expRecs4 = [];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //exists/isnull
   var findCondition5 = { a: { $size: 1, $exists: 1 } };
   var expRecs5 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 6, a: 1, b: 1 },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $size: 1, $isnull: 0 } };
   var expRecs6 = [{ No: 1, a: [1, 2, 3], b: [1, 2, 3] },
   { No: 2, a: [1, 2], b: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5], b: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4], b: [1, 2, 3, 4] },
   { No: 5, a: [], b: [] },
   { No: 7, a: { 0: 1, 1: 2, 2: 3 }, b: { 0: 1, 1: 2, 2: 3 } },
   { No: 8, a: { 0: 1, 1: 2 }, b: { 0: 1, 1: 2 } },
   { No: 9, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 }, b: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5 } },
   { No: 10, a: { 0: 1, 1: 2, 2: 3, 3: 4 }, b: { 0: 1, 1: 2, 2: 3, 3: 4 } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

}
