/******************************************************************************
 * @Description   : seqDB-9198:slice as function
 * @Author        : zhaoyu
 * @CreateTime    : 2016.11.01
 * @LastEditTime  : 2021.04.01
 * @LastEditors   : Li Yuanyue
 ******************************************************************************/
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
   mainCL_Name = CHANGEDPREFIX + "_maincl9198";
   subCL_Name1 = CHANGEDPREFIX + "_subcl91981";
   subCL_Name2 = CHANGEDPREFIX + "_subcl91982";
   subCL_Name3 = CHANGEDPREFIX + "_subcl91983";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

   //create maincl for range split
   var mainCLOption = { ShardingKey: { "No": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": 1 }, ShardingType: "hash" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { No: 1 }, UpBound: { No: 3 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { No: 3 }, UpBound: { No: 6 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { No: 6 }, UpBound: { No: 10 } } );

   //insert data 
   var doc = [{ No: 1, a: [1, 2, 3, 4, 5, 6] },
   { No: 2, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6 } },
   { No: 3, a: [4, 5, 6, 7, 8] },
   { No: 4, a: { 0: 4, 1: 5, 2: 6, 3: 7, 4: 8 } },
   { No: 5, a: 1 },
   { No: 6, a: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 7, a: [1, 2, 3, 7, 8, 9] },
   { No: 8, a: [11, 12, 13, 17, 18, 19] },
   { No: 9, b: [1, 2, 3, 4, 5, 6, 7] }];
   dbcl.insert( doc );

   //use et
   var findCondition1 = { a: { $slice: [2, 3], $et: [3, 4, 5] } };
   var expRecs1 = [{ No: 1, a: [1, 2, 3, 4, 5, 6] },
   { No: 6, a: [1, 2, 3, 4, 5, 6, 7, 8, 9] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //use in/nin/all
   var findCondition2 = { a: { $slice: [2, 3], $returnMatch: 0, $in: [3, 6, 20, 8, 4, 5] } };
   var expRecs2 = [{ No: 1, a: [3, 4, 5] },
   { No: 3, a: [6, 8] },
   { No: 6, a: [3, 4, 5] },
   { No: 7, a: [3, 8] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { a: { $slice: [2, 3], $returnMatch: 0, $nin: [6, 20, 5, 2, 11] } };
   var expRecs3 = [{ No: 2, a: { 0: 1, 1: 2, 2: 3, 3: 4, 4: 5, 5: 6 } },
   { No: 4, a: { 0: 4, 1: 5, 2: 6, 3: 7, 4: 8 } },
   { No: 5, a: 1 },
   { No: 7, a: [3, 7, 8] },
   { No: 8, a: [13, 17, 18] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { a: { $slice: [2, 3], $returnMatch: 0, $all: [5, 3] } };
   var expRecs4 = [{ No: 1, a: [3, 5] },
   { No: 6, a: [3, 5] }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var findCondition5 = { a: { $slice: [2, 3], $returnMatch: 0, $exists: 0 } };
   var expRecs5 = [{ No: 9, b: [1, 2, 3, 4, 5, 6, 7] }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $slice: [2, 3], $returnMatch: 0, $isnull: 1 } };
   var expRecs6 = [{ No: 9, b: [1, 2, 3, 4, 5, 6, 7] }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   var findCondition7 = { "a.$0": { $slice: [2, 3], $returnMatch: 0, $et: 1 } };
   var expRecs7 = [{ No: 1, a: [1, 2, 3, 4, 5, 6] },
   { No: 6, a: [1, 2, 3, 4, 5, 6, 7, 8, 9] },
   { No: 7, a: [1, 2, 3, 7, 8, 9] }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}

