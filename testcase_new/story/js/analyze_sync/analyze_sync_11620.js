/************************************
*@Description: 指定group收集统计信息  
*@author:      liuxiaoxuan
*@createdate:  2017.11.10
*@testlinkCase: seqDB-11620
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( 2 > commGetGroupsNum( db ) )
   {
      return;
   }

   var csName = COMMCSNAME + "11620";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { PageSize: 4096 };
   commCreateCS( db, csName, false, "", csOption );

   //create cl	
   var allGroups = commGetGroups( db );
   var groupName1 = allGroups[0][0].GroupName;
   var groupName2 = allGroups[1][0].GroupName;
   //get used groups
   var groups = [groupName1, groupName2];

   var clOption1 = { Group: groupName1 };
   var clName1 = COMMCLNAME + "11620_1";
   var dbcl1 = commCreateCL( db, csName, clName1, clOption1, true );

   var clOption2 = { Group: groupName2 };
   var clName2 = COMMCLNAME + "11620_2";
   var dbcl2 = commCreateCL( db, csName, clName2, clOption2, true );

   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );

   //create index
   commCreateIndex( dbcl1, "a", { a: 1 } );
   commCreateIndex( dbcl2, "a", { a: 1 } );

   //insert
   var insertNums = 3000;
   var sameValues = 9000;
   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   insertDiffDatas( dbcl2, insertNums );
   insertSameDatas( dbcl2, insertNums, sameValues );

   //check specify groups
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName, clName1, "a", false, false );
   checkStat( db, csName, clName2, "a", false, false );

   //check the query explain of master/slave nodes 
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains1 = getCommonExplain( dbclPrimary1, findConf );
   var actExplains2 = getCommonExplain( dbclPrimary2, findConf );
   checkExplain( actExplains1, expExplains );
   checkExplain( actExplains2, expExplains );

   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );


   //invoke analyze
   var options = { GroupName: groupName1 };
   db.analyze( options );

   //check specify groups
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName, clName1, "a", true, true );
   checkStat( db, csName, clName2, "a", false, false );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //check the query explain of master/slave nodes 
   var findConf = { a: 9000 };
   var expExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];

   var actExplains1 = getCommonExplain( dbclPrimary1, findConf );
   var actExplains2 = getCommonExplain( dbclPrimary2, findConf );
   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );

   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, insertNums );

   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //analyze invalid groups
   var options1 = { GroupName: "SYSCoord" };
   checkAnalyzeInvalidGroup( options1 );

   var options2 = { GroupName: "NotExistGroup" };
   checkAnalyzeInvalidGroup( options2 );

   //check catalog
   var options3 = { GroupName: "SYSCatalogGroup" };
   checkAnalyzeCataGroup( options3 );


   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, insertNums );

   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //invoke analyze
   var options = { Mode: 2, GroupName: groupName2 };
   db.analyze( options );

   //check specify groups
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName, clName1, "a", true, true );
   checkStat( db, csName, clName2, "a", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //check the query explain of master/slave nodes 
   var findConf = { a: 9000 };
   var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];

   var actExplains1 = getCommonExplain( dbclPrimary1, findConf );
   var actExplains2 = getCommonExplain( dbclPrimary2, findConf );
   checkExplain( actExplains1, expExplains );
   checkExplain( actExplains2, expExplains );

   //query
   query( dbclPrimary1, findConf, null, null, insertNums );
   query( dbclPrimary2, findConf, null, null, insertNums );

   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans = [{ ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );


   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}

function checkAnalyzeInvalidGroup ( options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED ANALYZE FAILED" );
   }
   catch( e )
   {
      if( SDB_RTN_CMD_NO_NODE_AUTH != e.message && SDB_CLS_GRP_NOT_EXIST != e.message )
      {
         throw e;
      }
   }
}

function checkAnalyzeCataGroup ( options )
{
   db.analyze( options );

   //get and connect to master node
   var cataRG = db.getCatalogRG();
   var priNode = cataRG.getMaster();
   var cataDB = priNode.connect();

   //check analyze stat info
   var sysStatCLName = "SYSSTAT.SYSCOLLECTIONSTAT";
   var sysStatIndexName = "SYSSTAT.SYSINDEXSTAT";

   var count = 0;
   var cursor = cataDB.listCollections();
   while( cursor.next() )
   {
      var name = cursor.current().toObj().Name;
      if( sysStatCLName == name || sysStatIndexName == name )
      {
         count++;
      }
   }

   if( count > 0 )
   {
      throw new Error( 'CHECK CATAGROUP FAIL' );
   }
}
