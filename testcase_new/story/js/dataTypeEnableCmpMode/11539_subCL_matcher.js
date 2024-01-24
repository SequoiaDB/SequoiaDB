/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.20
*@testlinkCase:seqDB-11539
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   //clean environment before test
   mainCL_Name = CHANGEDPREFIX + "_maincl_11539";
   subCL_Name1 = CHANGEDPREFIX + "_subcl1_11539";
   subCL_Name2 = CHANGEDPREFIX + "_subcl2_11539";
   subCL_Name3 = CHANGEDPREFIX + "_subcl3_11539";

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

   //nin
   var findConf1 = { a: { $nin: [-2, -1, 101, 199, 2000, 1001, 1998, 1999] } };
   var expRecs1 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
      //subcl2
      { a: 100, b: -1000 },
      { a: 198, b: 0 },
      { a: [100], b: [-1000] },
      { a: [198], b: [0] },
      //subcl3
      { a: 1000, b: -1000 },
      { a: [1000], b: [-1000] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //{$isnull:0}
   var findConf2 = { a: { $isnull: 0 } };
   var expRecs2 = [//subcl1
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
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //{$exists:1}
   var findConf3 = { a: { $exists: 1 } };
   var expRecs3 = [//subcl1
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
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //field
   var findConf4 = { a: { $gte: { $field: "b" } } };
   var expRecs4 = [//subcl1
      { a: -1000, b: -1000 },
      { a: -999, b: -999 },
      { a: [-1000], b: [-1000] },
      { a: [-999], b: [-999] },
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
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}