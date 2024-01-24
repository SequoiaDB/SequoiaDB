/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.22
*@testlinkCase:seqDB-11541
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl_11541";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1_11541";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2_11541";
   subCL_Name3 = CHANGEDPREFIX + "_subcl3_11541";

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
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range", ReplSize: 0 };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": -1 }, ShardingType: "range", ReplSize: 0 };
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
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      { a: [{ $numberLong: "-1000" }], b: [{ $numberLong: "-1000" }] },
      { a: [-999], b: [-999] },
      { a: [-2.1], b: [0] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: 199, b: 1 },
      { a: [100], b: [{ $numberLong: "-1000" }] },
      { a: [101.101], b: [-999] },
      { a: [{ $decimal: "198" }], b: [0] },
      { a: [199], b: [1] },
      //subcl3
      { a: { $decimal: "1999" }, b: 1 },
      { a: [1000], b: [{ $numberLong: "-1000" }] },
      { a: [{ $numberLong: "1001" }], b: [-999] },
      { a: [1998], b: [0] },
      { a: [{ $decimal: "1999" }], b: [1] }];
   dbcl.insert( doc );

   //expand
   var findConf1 = { a: { $expand: 1 } };
   var expRecs1 = [//subcl1
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      { a: -1000, b: [-1000] },
      { a: -999, b: [-999] },
      { a: -2.1, b: [0] },
      { a: { $decimal: "-1" }, b: [1] },
      //subcl2
      { a: 199, b: 1 },
      { a: 100, b: [-1000] },
      { a: 101.101, b: [-999] },
      { a: { $decimal: "198" }, b: [0] },
      { a: 199, b: [1] },
      //subcl3
      { a: { $decimal: "1999" }, b: 1 },
      { a: 1000, b: [-1000] },
      { a: 1001, b: [-999] },
      { a: 1998, b: [0] },
      { a: { $decimal: "1999" }, b: [1] }]
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //returnMatch
   var findConf1 = { a: { $returnMatch: 0, $in: [-1, -1000, -999, 100, 101.101, 1000, 1001] } };
   var expRecs1 = [//subcl1
      { a: { $decimal: "-1" }, b: { $decimal: "-1" } },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      { a: [{ $decimal: "-1" }], b: [1] },
      //subcl2
      { a: [100], b: [-1000] },
      { a: [101.101], b: [-999] },
      //subcl3
      { a: [1000], b: [-1000] },
      { a: [1001], b: [-999] }]
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}