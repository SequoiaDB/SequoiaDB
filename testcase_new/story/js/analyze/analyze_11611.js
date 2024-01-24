/************************************
*@Description: 指定普通cl收集统计信息
*@author:      liuxiaoxuan
*@createdate:  2017.11.09
*@testlinkCase: seqDB-11611
**************************************/
main( test );
function test ()
{
   //get all groups
   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; ++i ) { groups.push( allGroups[i][0].GroupName ); }

   var csName = COMMCSNAME + "11611";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create CLs
   var clName1 = COMMCLNAME + "11611_1";
   var dbcl1 = commCreateCL( db, csName, clName1 );

   var clName2 = COMMCLNAME + "11611_2";
   var dbcl2 = commCreateCL( db, csName, clName2 );

   var clName3 = COMMCLNAME + "11611_3";
   var dbcl3 = commCreateCL( db, csName, clName3 );

   var clName4 = COMMCLNAME + "11611_4";
   var dbcl4 = commCreateCL( db, csName, clName4 );

   var clName5 = COMMCLNAME + "11611_5";
   var dbcl5 = commCreateCL( db, csName, clName5 );

   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;
   var clFullName3 = csName + "." + clName3;
   var clFullName4 = csName + "." + clName4;
   var clFullName5 = csName + "." + clName5;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );
   var dbclPrimary3 = db1.getCS( csName ).getCL( clName3 );
   var dbclPrimary4 = db1.getCS( csName ).getCL( clName4 );
   var dbclPrimary5 = db1.getCS( csName ).getCL( clName5 );

   //create index
   commCreateIndex( dbcl3, "a", { a: 1 } );
   commCreateIndex( dbcl4, "a", { a: 1 } );
   commCreateIndex( dbcl5, "a", { a: 1 } );

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   // include datas, but no index
   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   // include datas and index
   insertDiffDatas( dbcl3, insertNums );
   insertSameDatas( dbcl3, insertNums, sameValues );
   insertDiffDatas( dbcl5, insertNums );
   insertSameDatas( dbcl5, insertNums, sameValues );

   //check all groups consistency
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName, clName1, "", false, false );
   checkStat( db, csName, clName2, "", false, false );
   checkStat( db, csName, clName3, "a", false, false );
   checkStat( db, csName, clName4, "a", false, false );
   checkStat( db, csName, clName5, "a", false, false );

   //check the query explain of master/slave nodes
   var findConf = { a: 9000 };

   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: 0 }];
   var expExplains3 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];
   var expExplains4 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: 0 }];
   var expExplains5 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains1 = getCommonExplain( dbclPrimary1, findConf );
   checkExplain( actExplains1, expExplains1 );
   var actExplains2 = getCommonExplain( dbclPrimary2, findConf );
   checkExplain( actExplains2, expExplains2 );
   var actExplains3 = getCommonExplain( dbclPrimary3, findConf );
   checkExplain( actExplains3, expExplains3 );
   var actExplains4 = getCommonExplain( dbclPrimary4, findConf );
   checkExplain( actExplains4, expExplains4 );
   var actExplains5 = getCommonExplain( dbclPrimary5, findConf );
   checkExplain( actExplains5, expExplains5 );

   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, 0 );
   query( dbclPrimary3, findConf, null, null, insertNums );
   query( dbclPrimary4, findConf, null, null, 0 );
   query( dbclPrimary5, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };
   var accessFindOption3 = { Collection: clFullName3 };
   var accessFindOption4 = { Collection: clFullName4 };
   var accessFindOption5 = { Collection: clFullName5 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var actAccessPlans3 = getCommonAccessPlans( db, accessFindOption3 );
   var actAccessPlans4 = getCommonAccessPlans( db, accessFindOption4 );
   var actAccessPlans5 = getCommonAccessPlans( db, accessFindOption5 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans3 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans4 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans5 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );
   checkSnapShotAccessPlans( clFullName3, expAccessPlans3, actAccessPlans3 );
   checkSnapShotAccessPlans( clFullName4, expAccessPlans4, actAccessPlans4 );
   checkSnapShotAccessPlans( clFullName5, expAccessPlans5, actAccessPlans5 );

   //invoke analyze
   var options = [{ Collection: csName + "." + clName1 },
   { Collection: csName + "." + clName2 },
   { Collection: csName + "." + clName3 },
   { Collection: csName + "." + clName4 }];

   for( var i in options )
   {
      db.analyze( options[i] );
   }

   //check all groups consistency
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName, clName1, "", true, false );
   checkStat( db, csName, clName2, "", false, false );
   checkStat( db, csName, clName3, "a", true, true );
   checkStat( db, csName, clName4, "c", false, false );
   checkStat( db, csName, clName5, "a", false, false );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };
   var accessFindOption3 = { Collection: clFullName3 };
   var accessFindOption4 = { Collection: clFullName4 };
   var accessFindOption5 = { Collection: clFullName5 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var actAccessPlans3 = getCommonAccessPlans( db, accessFindOption3 );
   var actAccessPlans4 = getCommonAccessPlans( db, accessFindOption4 );
   var actAccessPlans5 = getCommonAccessPlans( db, accessFindOption5 );

   var expAccessPlans = [];
   var expAccessPlans5 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );
   checkSnapShotAccessPlans( clFullName3, expAccessPlans, actAccessPlans3 );
   checkSnapShotAccessPlans( clFullName4, expAccessPlans, actAccessPlans4 );
   checkSnapShotAccessPlans( clFullName5, expAccessPlans5, actAccessPlans5 );

   //check the query explain of master/slave nodes
   var findConf = { a: 9000 };

   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: 0 }];
   var expExplains3 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains4 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: 0 }];
   var expExplains5 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains1 = getCommonExplain( dbclPrimary1, findConf );
   checkExplain( actExplains1, expExplains1 );
   var actExplains2 = getCommonExplain( dbclPrimary2, findConf );
   checkExplain( actExplains2, expExplains2 );
   var actExplains3 = getCommonExplain( dbclPrimary3, findConf );
   checkExplain( actExplains3, expExplains3 );
   var actExplains4 = getCommonExplain( dbclPrimary4, findConf );
   checkExplain( actExplains4, expExplains4 );
   var actExplains5 = getCommonExplain( dbclPrimary5, findConf );
   checkExplain( actExplains5, expExplains5 );

   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, 0 );
   query( dbclPrimary3, findConf, null, null, insertNums );
   query( dbclPrimary4, findConf, null, null, 0 );
   query( dbclPrimary5, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };
   var accessFindOption3 = { Collection: clFullName3 };
   var accessFindOption4 = { Collection: clFullName4 };
   var accessFindOption5 = { Collection: clFullName5 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var actAccessPlans3 = getCommonAccessPlans( db, accessFindOption3 );
   var actAccessPlans4 = getCommonAccessPlans( db, accessFindOption4 );
   var actAccessPlans5 = getCommonAccessPlans( db, accessFindOption5 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans3 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans4 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans5 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );
   checkSnapShotAccessPlans( clFullName3, expAccessPlans3, actAccessPlans3 );
   checkSnapShotAccessPlans( clFullName4, expAccessPlans4, actAccessPlans4 );
   checkSnapShotAccessPlans( clFullName5, expAccessPlans5, actAccessPlans5 );

   //mode 2
   var options = { Mode: 2, Collection: csName + "." + clName5 };
   db.analyze( options );

   //check after analyze
   checkConsistency( db, csName, clName5 );
   checkStat( db, csName, clName5, "a", true, true );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName5 };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName5, expAccessPlans, actAccessPlans );

   //check the query explain of master/slave nodes
   var findConf = { a: 9000 };

   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains = getCommonExplain( dbclPrimary5, findConf );
   checkExplain( actExplains, expExplains );

   //query
   query( dbclPrimary5, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption = { Collection: clFullName5 };

   var actAccessPlans = getCommonAccessPlans( db, accessFindOption );
   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName5, expAccessPlans, actAccessPlans );

   //check analyze SYSSTAT cl
   var expectErrCode = -6;
   checkAnalyzeInvalidResult( { Collection: "SYSSTAT.SYSCOLLECTIONSTAT" }, expectErrCode );
   checkAnalyzeInvalidResult( { Collection: "SYSSTAT.SYSINDEXSTAT" }, expectErrCode );

   //check analyze exist cs but non exist cl
   var expectErrCode = SDB_DMS_NOTEXIST;
   checkAnalyzeInvalidResult( { Collection: csName + ".non_exist_cl" }, expectErrCode );

   //check analyze not exist cs and cl
   var expectErrCode = SDB_DMS_CS_NOTEXIST;
   var cs_null = "non_exist_cl.non_exist_cl"
   checkAnalyzeInvalidResult( { Collection: cs_null }, expectErrCode );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeInvalidResult ( options, expectErrCode )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( expectErrCode != e.message )
      {
         throw e;
      }
   }
}
