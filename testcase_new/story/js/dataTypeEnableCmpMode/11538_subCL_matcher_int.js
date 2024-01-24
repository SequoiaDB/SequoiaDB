/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.20
*@testlinkCase:seqDB-11538
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl_11538";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1_11538";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2_11538";
   subCL_Name3 = CHANGEDPREFIX + "_subcl3_11538";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups, can not split
   var allGroupName = getGroupName( db );
   if( 1 >= allGroupName.length )
   {
      return;
   }
   //create maincl for range split
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": -1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: -1000 }, UpBound: { a: 0 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   //insert data
   var doc = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2, b: 0 },
      { a: -1, b: 1 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2], b: [0] },
      { a: [-1], b: [1] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: 199, b: 1 },
      { a: [100], b: [-1000] },
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: 1999, b: 1 },
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { $and: [{ b: { $gt: -1000 } }, { a: { $gt: 100 } }] };
   var expRecs1 = [//subcl2
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: 199, b: 1 },
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: 1999, b: 1 },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf1 = { $and: [{ b: { $gt: [-1000] } }, { a: { $gt: [100] } }] };
   var expRecs1 = [//subcl2
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { $and: [{ b: { $gte: -999 } }, { a: { $gte: 101 } }] };
   var expRecs2 = [//subcl2
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: 199, b: 1 },
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: 1999, b: 1 },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf2 = { $and: [{ b: { $gte: [-999] } }, { a: { $gte: [101] } }] };
   var expRecs2 = [//subcl2
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lt
   var findConf3 = { $and: [{ b: { $lt: 0 } }, { a: { $lt: 102 } }] };
   var expRecs3 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101, b: -999 },
      { a: [100], b: [-1000] },
      { a: [101], b: [-999] }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf3 = { $and: [{ b: { $lt: [0] } }, { a: { $lt: [102] } }] };
   var expRecs3 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101], b: [-999] }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lte
   var findConf4 = { $and: [{ b: { $lte: 0 } }, { a: { $lte: 198 } }] };
   var expRecs4 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2, b: 0 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2], b: [0] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: [100], b: [-1000] },
      { a: [101], b: [-999] },
      { a: [198], b: [0] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf4 = { $and: [{ b: { $lte: [0] } }, { a: { $lte: [198] } }] };
   var expRecs4 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2], b: [0] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101], b: [-999] },
      { a: [198], b: [0] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et
   var findConf5 = { a: { $et: 100 } };
   var expRecs5 = [//subcl2
      { a: 100, b: -1000 },
      { a: [100], b: [-1000] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf5 = { a: { $et: [100] } };
   var expRecs5 = [//subcl2
      { a: [100], b: [-1000] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //ne
   var findConf6 = { a: { $ne: 100 } };
   var expRecs6 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2, b: 0 },
      { a: -1, b: 1 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2], b: [0] },
      { a: [-1], b: [1] },
      //subcl2
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: 199, b: 1 },
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: 1999, b: 1 },
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf6 = { a: { $ne: [100] } };
   var expRecs6 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2, b: 0 },
      { a: -1, b: 1 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2], b: [0] },
      { a: [-1], b: [1] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101, b: -999 },
      { a: 198, b: 0 },
      { a: 199, b: 1 },
      { a: [101], b: [-999] },
      { a: [198], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: 1999, b: 1 },
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [1999], b: [1] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //mod
   var findConf7 = { a: { $mod: [2, 1] } };
   var expRecs7 = [{ a: 101, b: -999 },
   { a: 199, b: 1 },
   { a: [101], b: [-999] },
   { a: [199], b: [1] },
   //subcl3
   { a: 1001, b: -999 },
   { a: 1999, b: 1 },
   { a: [1001], b: [-999] },
   { a: [1999], b: [1] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //in
   var findConf8 = { a: { $in: [100, -1000, 1000, 198, 2000] } };
   var expRecs8 = [//subcl1
      { a: -1000, b: -1000 },
      { a: [-1000], b: [-1000] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 198, b: 0 },
      { a: [100], b: [-1000] },
      { a: [198], b: [0] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: [1000], b: [-1000] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //all
   var findConf9 = { a: { $all: [-1000] } };
   var expRecs9 = [//subcl1
      { a: -1000, b: -1000 },
      { a: [-1000], b: [-1000] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   //{isnull:1}
   var findConf10 = { a: { $isnull: 1 } };
   var expRecs10 = [];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   //{exists:0}
   var findConf11 = { a: { $exists: 0 } };
   var expRecs11 = [];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}