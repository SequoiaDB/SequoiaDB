/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.22
*@testlinkCase:seqDB-11540
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl_11540";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1_11540";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2_11540";
   subCL_Name3 = CHANGEDPREFIX + "_subcl3_11540";

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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: { $numberLong: "-1000" } }, UpBound: { a: 0 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: { $decimal: "1000" } }, UpBound: { a: 2000 } } );


   //insert data
   var doc = [//subcl1
      { a: { $numberLong: "-1000" }, b: { $numberLong: "-1000" } },
      { a: -999, b: -999 },
      { a: -2.1, b: 0 },
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      { a: [{ $numberLong: "-1000" }], b: [{ $numberLong: "-1000" }] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: 100, b: { $numberLong: "-1000" } },
      { a: 101.101, b: -999 },
      { a: { $decimal: "198" }, b: 0 },
      { a: 199, b: 1 },
      { a: [100], b: [{ $numberLong: "-1000" }] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: { $numberLong: "-1000" } },
      { a: { $numberLong: "1001" }, b: -999 },
      { a: 1998, b: 0 },
      { a: { $decimal: "1999" }, b: 1 },
      { a: [1000], b: [{ $numberLong: "-1000" }] },
      { a: [{ $numberLong: "1001" }], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   dbcl.insert( doc );

   //type
   var findConf1 = { a: { $type: 1, $et: 100 } };
   var expRecs1 = [//subcl1
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      //subcl2
      { a: { $decimal: "198" }, b: 0 },
      //subcl3
      { a: { $decimal: "1999" }, b: 1 }]
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $type: 1, $gt: 16 } };
   var expRecs2 = [ //subcl1
      { a: -1000, b: -1000 },
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      //subcl2
      { a: { $decimal: "198" }, b: 0 },
      //subcl3
      { a: 1001, b: -999 },
      { a: { $decimal: "1999" }, b: 1 }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $type: 1, $gte: 18 } };
   var expRecs3 = [//subcl1
      { a: -1000, b: -1000 },
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      //subcl2
      { a: { $decimal: "198" }, b: 0 },
      //subcl3
      { a: 1001, b: -999 },
      { a: { $decimal: "1999" }, b: 1 }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $type: 1, $lt: 100 } };
   var expRecs4 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2.1, b: 0 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101.101, b: -999 },
      { a: 199, b: 1 },
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $type: 1, $lte: 4 } };
   var expRecs5 = [//subcl1
      { a: -2.1, b: 0 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: 101.101, b: -999 },
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //size
   var findConf6 = { a: { $size: 1, $et: 1 } };
   var expRecs6 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $size: 1, $gt: 0 } };
   var expRecs7 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $size: 1, $gte: 1 } };
   var expRecs8 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   var findConf9 = { a: { $size: 1, $lt: 2 } };
   var expRecs9 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   var findConf10 = { a: { $size: 1, $lte: 3 } };
   var expRecs10 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf11 = { a: { $size: 1, $ne: null } };
   var expRecs11 = [//subcl1
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );

   var findConf12 = { a: { $type: 1, $ne: 100 } };
   var expRecs12 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: -2.1, b: 0 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 101.101, b: -999 },
      { a: 199, b: 1 },
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: 1001, b: -999 },
      { a: 1998, b: 0 },
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }]
   checkResult( dbcl, findConf12, hintConf, sortConf, expRecs12 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}