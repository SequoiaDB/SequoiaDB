/************************************
*@Description: arr use expand
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
   mainCL_Name = CHANGEDPREFIX + "_maincl10325";
   subCL_Name1 = CHANGEDPREFIX + "_subcl103251";
   subCL_Name2 = CHANGEDPREFIX + "_subcl103252";
   subCL_Name3 = CHANGEDPREFIX + "_subcl103253";

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
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { No: 4 }, UpBound: { No: 6 } } );

   //insert data 
   var doc = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: [2, 3, 4], c: [2, 3, 4] },
   { No: 3, b: [5, 6, [7, 8]], c: [5, 6, [7, 8]] },
   { No: 4, b: [] }];
   dbcl.insert( doc );

   //seqDB-10325
   var findCondition1 = { b: { $expand: 1 } };
   var expRecs1 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] },
   { No: 4, b: null }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   //seqDB-10326
   var findCondition2 = { b: { $expand: 1 }, c: { $expand: 1 } };
   InvalidArgCheck( dbcl, findCondition2, null, SDB_INVALIDARG );

   //seqDB-10327
   var findCondition3 = { d: { $expand: 1 } };
   checkResult( dbcl, findCondition3, null, doc, { No: 1 } );

   //seqDB-10328
   var findCondition4 = { b: { $expand: 0 }, c: { $expand: 1 } };
   InvalidArgCheck( dbcl, findCondition4, null, SDB_INVALIDARG );

   //seqDB-10329
   var findCondition5 = { b: { $expand: 1 } };
   var expRecs5 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] },
   { No: 4, b: null }];
   checkLimitFindResult( dbcl, findCondition5, null, expRecs5, { _id: 1 }, 8 );

   var findCondition6 = { b: { $expand: 1 } };
   checkLimitFindResult( dbcl, findCondition6, null, expRecs5, { _id: 1 }, 9 );

   var findCondition7 = { b: { $expand: 1 } };
   var expRecs7 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] }];
   checkLimitFindResult( dbcl, findCondition7, null, expRecs7, { _id: 1 }, 2 );

   var findCondition8 = { b: { $expand: 1 } };
   var expRecs8 = [];
   checkLimitFindResult( dbcl, findCondition8, null, expRecs8, { _id: 1 }, 0 );

   var findCondition9 = { b: { $expand: 1 } };
   checkLimitFindResult( dbcl, findCondition9, null, expRecs5, { _id: 1 }, -1 );

   //seqDB-10330
   var findCondition10 = { b: { $expand: 1 } };
   var expRecs10 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] },
   { No: 4, b: null }];
   checkSkipFindResult( dbcl, findCondition10, null, expRecs10, { _id: 1 }, 0 );

   var findCondition11 = { b: { $expand: 1 } };
   checkSkipFindResult( dbcl, findCondition11, null, expRecs10, { _id: 1 }, -1 );

   var findCondition12 = { b: { $expand: 1 } };
   var expRecs12 = [{ No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] },
   { No: 4, b: null }];
   checkSkipFindResult( dbcl, findCondition12, null, expRecs12, { _id: 1 }, 2 );

   var findCondition13 = { b: { $expand: 1 } };
   var expRecs13 = [];
   checkSkipFindResult( dbcl, findCondition13, null, expRecs13, { _id: 1 }, 8 );

   var findCondition14 = { b: { $expand: 1 } };
   checkSkipFindResult( dbcl, findCondition14, null, expRecs13, { _id: 1 }, 9 );

   var findCondition15 = { b: { $expand: 1, $in: [5, 10] } };
   var expRecs15 = [{ No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] }];
   checkResult( dbcl, findCondition15, null, expRecs15, { _id: 1 } );

   var findCondition16 = { b: { $expand: 1, $in: [10] } };
   var expRecs16 = [];
   checkResult( dbcl, findCondition16, null, expRecs16, { _id: 1 } );

   var findCondition17 = { b: { $expand: 1, $in: [1, 3, [7, 8]] } };
   var expRecs17 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] }];
   checkResult( dbcl, findCondition17, null, expRecs17, { _id: 1 } );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );
}


/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2016.10.17
**************************************/
function checkLimitFindResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition, num )
{
   var rc = limitFindData( dbcl, findCondition, findCondition2, sortCondition, num );
   checkRec( rc, expRecs );
}

/************************************
*@Description: find and limit data
*@author:      zhaoyu
*@createDate:  2016.10.17
**************************************/
function limitFindData ( dbcl, findCondition1, findCondition2, sortCondition, num )
{
   var limitResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition ).limit( num );
   return limitResult;
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2016.10.17
**************************************/
function checkSkipFindResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition, num )
{
   var rc = skipFindData( dbcl, findCondition, findCondition2, sortCondition, num );
   checkRec( rc, expRecs );
}

/************************************
*@Description: find and skip data
*@author:      zhaoyu
*@createDate:  2016.10.17
**************************************/
function skipFindData ( dbcl, findCondition1, findCondition2, sortCondition, num )
{
   var skipResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition ).skip( num );
   return skipResult;
}
