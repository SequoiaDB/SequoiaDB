/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.22
*@testlinkCase:seqDB-11542
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl_11542";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1_11542";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2_11542";
   subCL_Name3 = CHANGEDPREFIX + "_subcl3_11542";

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
   var mainCLOption = { ShardingKey: { "a": 1, "b": 1, "c": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "d": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "d": -1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { d: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { d: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: -1000, b: -1000, c: -1000 }, UpBound: { a: 0, b: 0, c: 0 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100, b: 100, c: 100 }, UpBound: { a: 200, b: 200, c: 200 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 1000, b: 1000, c: 1000 }, UpBound: { a: 2000, b: 2000, c: 2000 } } );

   //insert data
   var doc = [//subcl1
      { a: -1000, b: -1000, c: -1000, d: -1 },
      { a: -501, b: -501, c: -501, d: 0 },
      //subcl2
      { a: 100, b: 100, c: 100, d: 1 },
      { a: 151, b: 151, c: 151, d: 0 },
      //subcl3
      { a: 1000, b: 1000, c: 1000, d: -1 },
      { a: 1501, b: 1501, c: 1501, d: 0 }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { $and: [{ a: { $gt: 150 } }, { b: { $gt: 150 } }, { c: { $gt: 150 } }] };
   var expRecs1 = [//subcl2
      { a: 151, b: 151, c: 151, d: 0 },
      //subcl3
      { a: 1000, b: 1000, c: 1000, d: -1 },
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { $and: [{ a: { $gte: 151 } }, { b: { $gte: 151 } }, { c: { $gte: 151 } }] };
   var expRecs2 = [//subcl2
      { a: 151, b: 151, c: 151, d: 0 },
      //subcl3
      { a: 1000, b: 1000, c: 1000, d: -1 },
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lt
   var findConf3 = { $and: [{ a: { $lt: 150 } }, { b: { $lt: 150 } }, { c: { $lt: 150 } }] };
   var expRecs3 = [//subcl1
      { a: -1000, b: -1000, c: -1000, d: -1 },
      { a: -501, b: -501, c: -501, d: 0 },
      //subcl2
      { a: 100, b: 100, c: 100, d: 1 }]
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lte
   var findConf4 = { $and: [{ a: { $lte: 151 } }, { b: { $lte: 151 } }, { c: { $lte: 151 } }] };
   var expRecs4 = [//subcl1
      { a: -1000, b: -1000, c: -1000, d: -1 },
      { a: -501, b: -501, c: -501, d: 0 },
      //subcl2
      { a: 100, b: 100, c: 100, d: 1 },
      { a: 151, b: 151, c: 151, d: 0 }]
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et
   var findConf5 = { $and: [{ a: { $et: 151 } }, { b: { $et: 151 } }, { c: { $et: 151 } }] };
   var expRecs5 = [//subcl2
      { a: 151, b: 151, c: 151, d: 0 }]
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //mod
   var findConf6 = { $and: [{ a: { $mod: [2, 1] } }, { c: { $mod: [2, 1] } }, { b: { $mod: [2, 1] } }] };
   var expRecs6 = [//subcl2
      { a: 151, b: 151, c: 151, d: 0 },
      //subcl3
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //ne
   var findConf7 = { $and: [{ a: { $ne: 151 } }, { c: { $ne: 1000 } }, { b: { $ne: -501 } }] };
   var expRecs7 = [//subcl1
      { a: -1000, b: -1000, c: -1000, d: -1 },
      //subcl2
      { a: 100, b: 100, c: 100, d: 1 },
      //subcl3
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //in
   var findConf8 = {
      $and: [{ a: { $in: [-1000, -501, 100, 1000, 1501, 2000] } },
      { b: { $in: [2000, 100, 1000, 1501] } },
      { c: { $in: [2000, 1000, 1501] } }]
   };
   var expRecs8 = [//subcl3
      { a: 1000, b: 1000, c: 1000, d: -1 },
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //nin
   var findConf9 = {
      $and: [{ a: { $nin: [-1000, 2000, 151] } },
      { b: { $nin: [100, 2000, 151] } },
      { c: { $nin: [2000, 1000, 151] } }]
   };
   var expRecs9 = [//subcl1
      { a: -501, b: -501, c: -501, d: 0 },
      //subcl3
      { a: 1501, b: 1501, c: 1501, d: 0 }]
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   //all
   var findConf10 = { $and: [{ a: { $all: [-1000] } }, { b: { $all: [-1000] } }, { c: { $all: [-1000] } }] };
   var expRecs10 = [//subcl1
      { a: -1000, b: -1000, c: -1000, d: -1 }]
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}