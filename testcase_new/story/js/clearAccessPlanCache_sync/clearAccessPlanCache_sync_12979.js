/************************************
*@Description: 普通表rename cs，清空缓存功能验证
*@author:      liuxiaoxuan
*@createdate:  2017.11.09
*@testlinkCase: seqDB-12979
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) ) 
   {
      return;
   }

   //判断1节点模式
   if( true == isOnlyOneNodeInGroup() )
   {
      return;
   }

   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; i++ ) { groups.push( allGroups[i][0].GroupName ); }

   var csName = COMMCSNAME + "12979";
   var csName_new = "newCsName"
   commDropCS( db, csName, true, "drop CS in the beginning" );
   commDropCS( db, csName_new, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create CLs
   var clName1 = COMMCLNAME + "12979_1";
   var dbcl1 = commCreateCL( db, csName, clName1 );

   var clName2 = COMMCLNAME + "12979_2";
   var dbcl2 = commCreateCL( db, csName, clName2 );

   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;

   commCreateIndex( dbcl1, "a", { a: 1 } );
   commCreateIndex( dbcl2, "b", { b: 1 } );

   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   insertDiffDatas( dbcl2, insertNums );
   insertSameDatas( dbcl2, insertNums, sameValues );

   //get primary/slave node
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );

   db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave1 = db2.getCS( csName ).getCL( clName1 );
   var dbclSlave2 = db2.getCS( csName ).getCL( clName2 );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check before invoke analyze
   checkStat( db, csName, clName1, "a", false, false );
   checkStat( db, csName, clName2, "b", false, false );

   //query from primary/slave node
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );
   query( dbclSlave1, findConf1, null, null, insertNums );
   query( dbclSlave2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "b" },
   { ScanType: "ixscan", IndexName: "b" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //invoke analyze
   var options = { CollectionSpace: csName };
   db.analyze( options );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check after analyze
   checkStat( db, csName, clName1, "a", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //query from primary/slave node
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );
   query( dbclSlave1, findConf1, null, null, insertNums );
   query( dbclSlave2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //rename CS
   var oldCsName = csName;
   var newCsName = csName_new;
   db.renameCS( oldCsName, newCsName );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check newCL's anaylze info
   checkStat( db, newCsName, clName1, "a", true, true );
   checkStat( db, newCsName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //get new primary/slave node
   var newclPrimary1 = db1.getCS( newCsName ).getCL( clName1 );
   var newclPrimary2 = db1.getCS( newCsName ).getCL( clName2 );
   var newclSlave1 = db2.getCS( newCsName ).getCL( clName1 );
   var newclSlave2 = db2.getCS( newCsName ).getCL( clName2 );

   //query from primary/slave node
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   query( newclPrimary1, findConf1, null, null, insertNums );
   query( newclPrimary2, findConf2, null, null, insertNums );
   query( newclSlave1, findConf1, null, null, insertNums );
   query( newclSlave2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var clNewFullName1 = newCsName + "." + clName1;
   var clNewFullName2 = newCsName + "." + clName2;

   var accessFindOption1 = { Collection: clNewFullName1 };
   var accessFindOption2 = { Collection: clNewFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   commDropCS( db, csName, true, "drop CS in the end" );
   commDropCS( db, csName_new, true, "drop CS in the end" );
   db1.close();
   db2.close();
}