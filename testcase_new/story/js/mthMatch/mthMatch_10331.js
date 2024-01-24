/************************************
*@Description: arr use returnMatch
*@author:      zhaoyu
*@createdate:  2016.10.17
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
   mainCL_Name = CHANGEDPREFIX + "_maincl10331";
   subCL_Name1 = CHANGEDPREFIX + "_subcl103311";
   subCL_Name2 = CHANGEDPREFIX + "_subcl103312";
   subCL_Name3 = CHANGEDPREFIX + "_subcl103313";

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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { No: 1 }, UpBound: { No: 2 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { No: 2 }, UpBound: { No: 4 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { No: 4 }, UpBound: { No: 7 } } );

   //insert data 
   var doc = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 3, b: 4 },
   { No: 4, b: [1, 2, 3, 4, 5] },
   { No: 5, b: [1, [[1, 2, 3], 3, 4], 3, 5, 2, 6, 1, 5, 7] },
   { No: 6, b: [] }];
   dbcl.insert( doc );

   //seqDB-10331
   var findCondition1 = { b: { $returnMatch: 0, $in: [1, 2, 3] } };
   var expRecs1 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   var findCondition2 = { b: { $returnMatch: 2, $in: [1, 2, 3] } };
   var expRecs2 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

   var findCondition3 = { b: { $returnMatch: -2, $in: [1, 2, 3] } };
   var expRecs3 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [2, 3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   var findCondition4 = { b: { $returnMatch: 4, $in: [1, 2, 3] } };
   var expRecs4 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: null }];
   checkResult( dbcl, findCondition4, null, expRecs4, { _id: 1 } );

   var findCondition5 = { b: { $returnMatch: -4, $in: [1, 2, 3] } };
   var expRecs5 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition5, null, expRecs5, { _id: 1 } );

   //seqDB-10332
   var findCondition6 = { b: { $returnMatch: "a", $in: [1, 2, 3] } };
   InvalidArgCheck( dbcl, findCondition6, null, SDB_INVALIDARG );

   //seqDB-10333
   var findCondition7 = { b: { $returnMatch: 0, $in: [1, 2, 3] }, c: { $returnMatch: 0, $in: [1, 2, 3] } };
   InvalidArgCheck( dbcl, findCondition7, null, SDB_INVALIDARG );

   //seqDB-10334
   var findCondition8 = { c: { $returnMatch: 0, $in: [1, 2, 3] } };
   var expRecs8 = [];
   checkResult( dbcl, findCondition8, null, expRecs8, { _id: 1 } );

   //seqDB-10335
   var findCondition9 = { b: { $returnMatch: [-3, 2], $in: [1, 2, 3] } };
   var expRecs9 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2] },
   { No: 5, b: [3, 2] }];
   checkResult( dbcl, findCondition9, null, expRecs9, { _id: 1 } );

   var findCondition10 = { b: { $returnMatch: [-3, 4], $in: [1, 2, 3] } };
   var expRecs10 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition10, null, expRecs10, { _id: 1 } );

   var findCondition11 = { b: { $returnMatch: [2, 2], $in: [1, 2, 3] } };
   var expRecs11 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition11, null, expRecs11, { _id: 1 } );

   var findCondition12 = { b: { $returnMatch: [-3, -2], $in: [1, 2, 3] } };
   var expRecs12 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition12, null, expRecs12, { _id: 1 } );

   var findCondition13 = { b: { $returnMatch: [-3, -4], $in: [1, 2, 3] } };
   var expRecs13 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition13, null, expRecs13, { _id: 1 } );

   var findCondition13 = { b: { $returnMatch: [2, 2], $in: [1, 2, 3] } };
   var expRecs13 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition13, null, expRecs13, { _id: 1 } );

   var findCondition14 = { b: { $returnMatch: [3, 4], $in: [1, 2, 3] } };
   var expRecs14 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1] }];
   checkResult( dbcl, findCondition14, null, expRecs14, { _id: 1 } );

   var findCondition15 = { b: { $returnMatch: [-4, 4], $in: [1, 2, 3] } };
   var expRecs15 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition15, null, expRecs15, { _id: 1 } );

   var findCondition16 = { b: { $returnMatch: [0, 4], $in: [1, 2, 3] } };
   var expRecs16 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition16, null, expRecs16, { _id: 1 } );

   var findCondition17 = { b: { $returnMatch: [0, -4], $in: [1, 2, 3] } };
   var expRecs17 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition17, null, expRecs17, { _id: 1 } );

   //seqDB-10336
   var findCondition18 = { b: { $returnMatch: ["a", 2], $in: [1, 2, 3] } };
   InvalidArgCheck( dbcl, findCondition18, null, SDB_INVALIDARG );

   var findCondition19 = { b: { $returnMatch: [2, "a"], $in: [1, 2, 3] } };
   InvalidArgCheck( dbcl, findCondition19, null, SDB_INVALIDARG );

   //seqDB-10337
   var findCondition19 = { b: { $returnMatch: [2, 2], $in: [1, 2, 3] }, c: { $returnMatch: [2, 2], $in: [1, 2, 3] } };
   InvalidArgCheck( dbcl, findCondition19, null, SDB_INVALIDARG );

   //seqDB-10338
   var findCondition20 = { c: { $returnMatch: [0, 1], $in: [1, 2, 3] } };
   var expRecs20 = [];
   checkResult( dbcl, findCondition20, null, expRecs20, { _id: 1 } );

   //empty arr
   var findCondition21 = { c: { $returnMatch: [0, 1], $in: [] } };
   var expRecs21 = [];
   checkResult( dbcl, findCondition21, null, expRecs21, { _id: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}
