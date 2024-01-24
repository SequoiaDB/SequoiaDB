/************************************
*@Description: main-sub table use numberic functions,
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
   mainCL_Name = CHANGEDPREFIX + "_maincl10486";
   subCL_Name1 = CHANGEDPREFIX + "_subcl104861";
   subCL_Name2 = CHANGEDPREFIX + "_subcl104862";
   subCL_Name3 = CHANGEDPREFIX + "_subcl104863";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean main collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );


   //create maincl for range split
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: -1000 }, UpBound: { a: 1000 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 2000 }, UpBound: { a: 3000 } } );

   //insert data 
   var doc = [{ No: 1, a: -1.2, b: -1.2, c: -1.2 },
   { No: 2, a: 1200, b: 1200, c: 1200 },
   { No: 3, a: 2500, b: 2500, c: 2500 }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $abs: 1, $et: 1.2 }, b: { $abs: 1, $et: 1.2 }, c: { $abs: 1, $et: 1.2 } };
   var expRecs1 = [{ No: 1, a: -1.2, b: -1.2, c: -1.2 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $floor: 1, $et: -2 }, b: { $floor: 1, $et: -2 }, c: { $floor: 1, $et: -2 } };
   var expRecs2 = [{ No: 1, a: -1.2, b: -1.2, c: -1.2 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { a: { $ceiling: 1, $et: -1 }, b: { $ceiling: 1, $et: -1 }, c: { $ceiling: 1, $et: -1 } };
   var expRecs3 = [{ No: 1, a: -1.2, b: -1.2, c: -1.2 }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   var findCondition4 = { a: { $subtract: 1000, $et: 200 }, b: { $subtract: 1000, $et: 200 }, c: { $subtract: 1000, $et: 200 } };
   var expRecs4 = [{ No: 2, a: 1200, b: 1200, c: 1200 }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   var findCondition5 = { a: { $add: 500, $et: 3000 }, b: { $add: 500, $et: 3000 }, c: { $add: 500, $et: 3000 } };
   var expRecs5 = [{ No: 3, a: 2500, b: 2500, c: 2500 }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   var findCondition6 = { a: { $multiply: 2, $et: 2400 }, b: { $multiply: 2, $et: 2400 }, c: { $multiply: 2, $et: 2400 } };
   var expRecs6 = [{ No: 2, a: 1200, b: 1200, c: 1200 }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   var findCondition7 = { a: { $divide: 5, $et: 500 }, b: { $divide: 5, $et: 500 }, c: { $divide: 5, $et: 500 } };
   var expRecs7 = [{ No: 3, a: 2500, b: 2500, c: 2500 }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   var findCondition8 = { a: { $mod: 5, $et: 0 }, b: { $mod: 5, $et: 0 }, c: { $mod: 5, $et: 0 } };
   var expRecs8 = [{ No: 2, a: 1200, b: 1200, c: 1200 },
   { No: 3, a: 2500, b: 2500, c: 2500 }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );

   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}
