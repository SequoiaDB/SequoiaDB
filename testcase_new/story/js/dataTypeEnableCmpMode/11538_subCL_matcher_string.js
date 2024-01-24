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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: "a" }, UpBound: { a: "b" } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: "f" }, UpBound: { a: "g" } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: "y" }, UpBound: { a: "z" } } );

   //insert data
   var doc = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] },
      //subcl2
      { a: "fa", b: -1000 },
      { a: "fb", b: -999 },
      { a: "fc", b: 0 },
      { a: "fd", b: 1 },
      { a: ["fa"], b: [-1000] },
      { a: ["fb"], b: [-999] },
      { a: ["fc"], b: [0] },
      { a: ["fd"], b: [1] },
      //subcl3
      { a: "ya", b: -1000 },
      { a: "yb", b: -999 },
      { a: "yc", b: 0 },
      { a: "yd", b: 1 },
      { a: ["ya"], b: [-1000] },
      { a: ["yb"], b: [-999] },
      { a: ["yc"], b: [0] },
      { a: ["yd"], b: [1] }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { $and: [{ a: { $gt: "fa" } }, { b: { $gt: -1000 } }] };
   var expRecs1 = [//subcl2
      { a: "fb", b: -999 },
      { a: "fc", b: 0 },
      { a: "fd", b: 1 },
      { a: ["fb"], b: [-999] },
      { a: ["fc"], b: [0] },
      { a: ["fd"], b: [1] },
      //subcl3
      { a: "yb", b: -999 },
      { a: "yc", b: 0 },
      { a: "yd", b: 1 },
      { a: ["yb"], b: [-999] },
      { a: ["yc"], b: [0] },
      { a: ["yd"], b: [1] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { $and: [{ a: { $gte: "fb" } }, { b: { $gte: -999 } }] };
   var expRecs2 = [//subcl2
      { a: "fb", b: -999 },
      { a: "fc", b: 0 },
      { a: "fd", b: 1 },
      { a: ["fb"], b: [-999] },
      { a: ["fc"], b: [0] },
      { a: ["fd"], b: [1] },
      //subcl3
      { a: "yb", b: -999 },
      { a: "yc", b: 0 },
      { a: "yd", b: 1 },
      { a: ["yb"], b: [-999] },
      { a: ["yc"], b: [0] },
      { a: ["yd"], b: [1] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lt
   var findConf3 = { $and: [{ a: { $lt: "fc" } }, { b: { $lt: 0 } }] };
   var expRecs3 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      //subcl2
      { a: "fa", b: -1000 },
      { a: "fb", b: -999 },
      { a: ["fa"], b: [-1000] },
      { a: ["fb"], b: [-999] }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lte
   var findConf4 = { $and: [{ a: { $lte: "fc" } }, { b: { $lte: 0 } }] };
   var expRecs4 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      //subcl2
      { a: "fa", b: -1000 },
      { a: "fb", b: -999 },
      { a: "fc", b: 0 },
      { a: ["fa"], b: [-1000] },
      { a: ["fb"], b: [-999] },
      { a: ["fc"], b: [0] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et
   var findConf5 = { a: { $et: "fa" } };
   var expRecs5 = [//subcl2
      { a: "fa", b: -1000 },
      { a: ["fa"], b: [-1000] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //ne
   var findConf6 = { a: { $ne: "fa" } };
   var expRecs6 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] },
      //subcl2
      { a: "fb", b: -999 },
      { a: "fc", b: 0 },
      { a: "fd", b: 1 },
      { a: ["fb"], b: [-999] },
      { a: ["fc"], b: [0] },
      { a: ["fd"], b: [1] },
      //subcl3
      { a: "ya", b: -1000 },
      { a: "yb", b: -999 },
      { a: "yc", b: 0 },
      { a: "yd", b: 1 },
      { a: ["ya"], b: [-1000] },
      { a: ["yb"], b: [-999] },
      { a: ["yc"], b: [0] },
      { a: ["yd"], b: [1] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //mod
   var findConf7 = { a: { $mod: [2, 1] } };
   var expRecs7 = [];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //in
   var findConf8 = { a: { $in: ["aa", "fa", "fc", "ya", "yf"] } };
   var expRecs8 = [//subcl1
      { a: "aa", b: -1000 },
      { a: ["aa"], b: [-1000] },
      //subcl2
      { a: "fa", b: -1000 },
      { a: "fc", b: 0 },
      { a: ["fa"], b: [-1000] },
      { a: ["fc"], b: [0] },
      //subcl3
      { a: "ya", b: -1000 },
      { a: ["ya"], b: [-1000] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //all
   var findConf9 = { a: { $all: ["aa"] } };
   var expRecs9 = [//subcl1
      { a: "aa", b: -1000 },
      { a: ["aa"], b: [-1000] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   //{$regex}, SEQUOIADBMAINSTREAM-2458
   var findConf10 = { a: { $regex: "^a", $options: "i" } };
   var expRecs10 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf10 = { a: { $regex: "^a", $options: "i", $lt: "ad" } };
   var expRecs10 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf10 = { a: { $options: "i", $regex: "^a" } };
   var expRecs10 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf10 = { a: { $options: "i", $regex: "^a", $gte: "ab" } };
   var expRecs10 = [//subcl1
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf10 = { a: { $gte: "ab", $options: "i", $regex: "^a" } };
   var expRecs10 = [//subcl1
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf10 = { a: { $regex: "^a" } };
   var expRecs10 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: "ad", b: 1 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] },
      { a: ["ad"], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   //SEQUOIADBMAINSTREAM-2449
   var findConf10 = { a: { $regex: "^a", $lt: "ad" } };
   var expRecs10 = [//subcl1
      { a: "aa", b: -1000 },
      { a: "ab", b: -999 },
      { a: "ac", b: 0 },
      { a: ["aa"], b: [-1000] },
      { a: ["ab"], b: [-999] },
      { a: ["ac"], b: [0] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}