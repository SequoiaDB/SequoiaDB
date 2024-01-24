/************************************
*@Description: match MainCL sharding,use et/gt/gte/lt/lte/in/all/isnull/exists
               data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null
*@author:      liuxiaoxuan
*@createdate:  2017.5.22
*@testlinkCase: seqDB-11543
**************************************/

main( test );

function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = "maincl_11543";
   subCL_Name1 = "subcl1_11543";
   subCL_Name2 = "subcl2_11543";
   subCL_Name3 = "subcl3_11543";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 100 }, UpBound: { a: 1000 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 1000 }, UpBound: { a: 5000 } } );

   //insert data 
   var doc = [//subcl1
      { a: 0, b: 100, c: { $minKey: 1 } },
      { a: 50, b: 1000 },
      { a: 65, b: -999, c: null },
      { a: 67, b: -998, c: 1 },
      { a: [0], b: [100], c: "abc" },
      { a: [50], b: [1000], c: { a: 1 } },
      { a: [65], b: [-999], c: [1, 2, 3] },
      { a: [67], b: [-998], c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      //subcl2
      { a: 100, b: 99, c: { $oid: "591cf397a54fe50425000000" } },
      { a: 101, b: 100, c: false },
      { a: 198, b: -99, c: { $date: "2017-05-15" } },
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [101], b: [100], c: { $maxKey: 1 } },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      { a: [199], b: [-98] },
      //subcl3
      { a: 1000, b: 101, c: "ba" },
      { a: 4998, b: 0, c: "ac" },
      { a: 4999, b: 1, c: { $date: "2017-05-20" } },
      { a: [1000], b: [-101], c: [4, 3, 2, 1] },
      { a: [1001], b: [999], c: [4, 3, 2] },
      { a: [4998], b: [0], c: { $regex: "^a", $options: "i" } },
      { a: [4999], b: [1] }];

   dbcl.insert( doc );

   //$et
   var findCondition1 = { c: { $et: 1 } };
   var expRecs1 = [//subcl1
      { a: 67, b: -998, c: 1 },
      { a: [65], b: [-999], c: [1, 2, 3] },
      //subcl2
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: [1000], b: [-101], c: [4, 3, 2, 1] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gt
   var findCondition4 = { c: { $gt: 10 } };
   var expRecs4 = [//subcl1
      { a: [0], b: [100], c: "abc" },
      { a: [50], b: [1000], c: { a: 1 } },
      { a: [67], b: [-998], c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      //subcl2
      { a: 100, b: 99, c: { $oid: "591cf397a54fe50425000000" } },
      { a: 101, b: 100, c: false },
      { a: 198, b: -99, c: { $date: "2017-05-15" } },
      { a: [101], b: [100], c: { $maxKey: 1 } },
      //subcl3
      { a: 1000, b: 101, c: "ba" },
      { a: 4998, b: 0, c: "ac" },
      { a: 4999, b: 1, c: { $date: "2017-05-20" } },
      { a: [4998], b: [0], c: { $regex: "^a", $options: "i" } }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //$gte
   var findCondition7 = { c: { $gte: 10 } };
   var expRecs7 = [//subcl1
      { a: [0], b: [100], c: "abc" },
      { a: [50], b: [1000], c: { a: 1 } },
      { a: [67], b: [-998], c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      //subcl2
      { a: 100, b: 99, c: { $oid: "591cf397a54fe50425000000" } },
      { a: 101, b: 100, c: false },
      { a: 198, b: -99, c: { $date: "2017-05-15" } },
      { a: [101], b: [100], c: { $maxKey: 1 } },
      //subcl3
      { a: 1000, b: 101, c: "ba" },
      { a: 4998, b: 0, c: "ac" },
      { a: 4999, b: 1, c: { $date: "2017-05-20" } },
      { a: [4998], b: [0], c: { $regex: "^a", $options: "i" } }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   //$lt
   var findCondition8 = { c: { $lt: 10 } };
   var expRecs8 = [//subcl1
      { a: 0, b: 100, c: { $minKey: 1 } },
      { a: 65, b: -999, c: null },
      { a: 67, b: -998, c: 1 },
      { a: [65], b: [-999], c: [1, 2, 3] },
      //subcl2
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: [1000], b: [-101], c: [4, 3, 2, 1] },
      { a: [1001], b: [999], c: [4, 3, 2] }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //$lte
   var findCondition11 = { c: { $lte: 10 } };
   var expRecs11 = [//subcl1
      { a: 0, b: 100, c: { $minKey: 1 } },
      { a: 65, b: -999, c: null },
      { a: 67, b: -998, c: 1 },
      { a: [65], b: [-999], c: [1, 2, 3] },
      //subcl2
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: [1000], b: [-101], c: [4, 3, 2, 1] },
      { a: [1001], b: [999], c: [4, 3, 2] }];
   checkResult( dbcl, findCondition11, hintConf, sortConf, expRecs11 );

   //$ne
   var findCondition12 = { c: { $ne: "abc" } };
   var expRecs12 = [//subcl1
      { a: 0, b: 100, c: { $minKey: 1 } },
      { a: 65, b: -999, c: null },
      { a: 67, b: -998, c: 1 },
      { a: [50], b: [1000], c: { a: 1 } },
      { a: [65], b: [-999], c: [1, 2, 3] },
      { a: [67], b: [-998], c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      //subcl2
      { a: 100, b: 99, c: { $oid: "591cf397a54fe50425000000" } },
      { a: 101, b: 100, c: false },
      { a: 198, b: -99, c: { $date: "2017-05-15" } },
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [101], b: [100], c: { $maxKey: 1 } },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: 1000, b: 101, c: "ba" },
      { a: 4998, b: 0, c: "ac" },
      { a: 4999, b: 1, c: { $date: "2017-05-20" } },
      { a: [1000], b: [-101], c: [4, 3, 2, 1] },
      { a: [1001], b: [999], c: [4, 3, 2] },
      { a: [4998], b: [0], c: { $regex: "^a", $options: "i" } }];
   checkResult( dbcl, findCondition12, hintConf, sortConf, expRecs12 );

   //$in
   var findCondition13 = { c: { $in: [1, 10] } };
   var expRecs13 = [//subcl1
      { a: 67, b: -998, c: 1 },
      { a: [65], b: [-999], c: [1, 2, 3] },
      //subcl2
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: [1000], b: [-101], c: [4, 3, 2, 1] }];
   checkResult( dbcl, findCondition13, hintConf, sortConf, expRecs13 );

   //$all
   var findCondition14 = { c: { $all: [1, 2, 3] } };
   var expRecs14 = [//subcl1
      { a: [65], b: [-999], c: [1, 2, 3] },
      //subcl2
      { a: 199, b: -98, c: [2, 3, 1] },
      { a: [100], b: [99], c: [3, 2, 1] },
      { a: [998], b: [-99], c: [3, 2, 1, 4] },
      //subcl3
      { a: [1000], b: [-101], c: [4, 3, 2, 1] }];
   checkResult( dbcl, findCondition14, hintConf, sortConf, expRecs14 );

   //{$isnull:1}
   var findCondition15 = { c: { $isnull: 1 } };
   var expRecs15 = [//subcl1
      { a: 50, b: 1000 },
      { a: 65, b: -999, c: null },
      //subcl2
      { a: [199], b: [-98] },
      //subcl3
      { a: [4999], b: [1] }];
   checkResult( dbcl, findCondition15, hintConf, sortConf, expRecs15 );

   //{$exists:0}
   var findCondition15 = { c: { $exists: 0 } };
   var expRecs15 = [//subcl1
      { a: 50, b: 1000 },
      //subcl2
      { a: [199], b: [-98] },
      //subcl3
      { a: [4999], b: [1] }];
   checkResult( dbcl, findCondition15, hintConf, sortConf, expRecs15 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}

