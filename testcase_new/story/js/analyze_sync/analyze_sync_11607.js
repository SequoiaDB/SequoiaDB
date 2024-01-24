/************************************
*@Description: 收集全局统计信息  
*@author:      liuxiaoxuan
*@createdate:  2017.11.10
*@testlinkCase: seqDB-11607
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

   //get all groups
   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; ++i ) { groups.push( allGroups[i][0].GroupName ); }

   var csName1 = COMMCSNAME + "11607_1";
   var csName2 = COMMCSNAME + "11607_2";
   var maincsName = "maincs11607";
   commDropCS( db, csName1, true, "drop CS in the beginning" );
   commDropCS( db, csName2, true, "drop CS in the beginning" );
   commDropCS( db, maincsName, true, "drop CS in the beginning" );

   commCreateCS( db, csName1, false, "" );
   commCreateCS( db, csName2, false, "" );
   commCreateCS( db, maincsName, false, "" );

   //create CLs
   var clName1 = COMMCLNAME + "11607_1";
   var dbCommCL1 = commCreateCL( db, csName1, clName1 );
   var dbCommCL2 = commCreateCL( db, csName2, clName1 );

   var clOption2 = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   var clName2 = COMMCLNAME + "11607_2";
   var dbHashCL1 = commCreateCL( db, csName1, clName2, clOption2, true );
   var dbHashCL2 = commCreateCL( db, csName2, clName2, clOption2, true );

   var clOption3 = { ShardingKey: { a: 1 }, ShardingType: "range" };
   var clName3 = COMMCLNAME + "11607_3";
   var dbRangCL1 = commCreateCL( db, csName1, clName3, clOption3, true );
   var dbRangCL2 = commCreateCL( db, csName2, clName3, clOption3, true );

   //create maincl 
   var mainclName = "testmaincl11607";
   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 } };
   var maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   //create subcl
   var subclName = "subcl11607";
   var subclGroupName = allGroups[0][0].GroupName;
   var sbuclOption = { Group: subclGroupName };
   var subcl = commCreateCL( db, maincsName, subclName, sbuclOption );

   var clFullName11 = csName1 + "." + clName1;
   var clFullName12 = csName1 + "." + clName2;
   var clFullName13 = csName1 + "." + clName3;
   var clFullName21 = csName2 + "." + clName1;
   var clFullName22 = csName2 + "." + clName2;
   var clFullName23 = csName2 + "." + clName3;
   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName = maincsName + "." + subclName;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbCommCLPrimary1 = db1.getCS( csName1 ).getCL( clName1 );
   var dbCommCLPrimary2 = db1.getCS( csName2 ).getCL( clName1 );
   var dbHashCLPrimary1 = db1.getCS( csName1 ).getCL( clName2 );
   var dbHashCLPrimary2 = db1.getCS( csName2 ).getCL( clName2 );
   var dbRangCLPrimary1 = db1.getCS( csName1 ).getCL( clName3 );
   var dbRangCLPrimary2 = db1.getCS( csName2 ).getCL( clName3 );
   var dbMainCLPrimary = db1.getCS( maincsName ).getCL( mainclName );

   //attach cl
   maincl.attachCL( subclFullName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbCommCL1, insertNums );
   insertSameDatas( dbCommCL1, insertNums, sameValues );

   insertDiffDatas( dbCommCL2, insertNums );
   insertSameDatas( dbCommCL2, insertNums, sameValues );

   insertDiffDatas( dbHashCL1, insertNums );
   insertSameDatas( dbHashCL1, insertNums, sameValues );

   insertDiffDatas( dbHashCL2, insertNums );
   insertSameDatas( dbHashCL2, insertNums, sameValues );

   insertDiffDatas( dbRangCL1, insertNums );
   insertSameDatas( dbRangCL1, insertNums, sameValues );

   insertDiffDatas( dbRangCL2, insertNums );
   insertSameDatas( dbRangCL2, insertNums, sameValues );

   insertDiffDatas( maincl, insertNums );
   insertSameDatas( maincl, insertNums, sameValues );

   //create index
   commCreateIndex( dbCommCL1, "a", { a: 1 } );
   commCreateIndex( dbCommCL2, "a", { a: 1 } );
   commCreateIndex( dbHashCL1, "b", { b: 1 } );
   commCreateIndex( dbHashCL2, "b", { b: 1 } );
   commCreateIndex( dbRangCL1, "b", { b: 1 } );
   commCreateIndex( dbRangCL2, "b", { b: 1 } );
   commCreateIndex( maincl, "a", { a: 1 } );
   commCreateIndex( maincl, "b", { b: 1 } );

   //split shard cls
   ClSplitOneTimes( csName1, clName2, 50 );
   ClSplitOneTimes( csName2, clName2, 50 );
   ClSplitOneTimes( csName1, clName3, 50 );
   ClSplitOneTimes( csName2, clName3, 50 );

   //check all groups consistency
   checkConsistency( db, null, null, groups );
   //check before invoke analyze
   checkStat( db, csName1, clName1, "a", false, false );
   checkStat( db, csName2, clName1, "a", false, false );
   checkStat( db, csName1, clName2, "$shard", false, false );
   checkStat( db, csName2, clName2, "$shard", false, false );
   checkStat( db, csName1, clName3, "$shard", false, false );
   checkStat( db, csName2, clName3, "$shard", false, false );
   checkStat( db, csName1, clName2, "b", false, false );
   checkStat( db, csName2, clName2, "b", false, false );
   checkStat( db, csName1, clName3, "b", false, false );
   checkStat( db, csName2, clName3, "b", false, false );
   checkStat( db, maincsName, subclName, "a", false, false );
   checkStat( db, maincsName, subclName, "b", false, false );

   //check the query explain of master/slave nodes 
   var groupsHash1 = getSplitGroups( csName1, clName2, 1 );
   var groupsHash2 = getSplitGroups( csName2, clName2, 1 );

   var groupsRang1 = getSplitGroups( csName1, clName3, 1 );
   var groupsRang2 = getSplitGroups( csName2, clName3, 1 );

   var srcHashGroupName1 = groupsHash1[0].GroupName;
   var destHashGroupName1 = groupsHash1[1].GroupName;
   var srcHashGroupName2 = groupsHash2[0].GroupName;
   var destHashGroupName2 = groupsHash2[1].GroupName;

   var srcRangGroupName1 = groupsRang1[0].GroupName;
   var destRangGroupName1 = groupsRang1[1].GroupName;
   var srcRangGroupName2 = groupsRang2[0].GroupName;
   var destRangGroupName2 = groupsRang2[1].GroupName;

   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expCommExplains = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums }];
   var expHashExplains1 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcHashGroupName1, ReturnNum: insertNums }];
   var expHashExplains2 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcHashGroupName2, ReturnNum: insertNums }];
   var expRangExplains1 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains2 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destRangGroupName2, ReturnNum: insertNums }];

   var expHashExplains3 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcHashGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1, ReturnNum: 0 }];
   var expHashExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcHashGroupName2, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2, ReturnNum: 0 }];
   var expRangExplains3 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1, ReturnNum: 0 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2, ReturnNum: 0 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destRangGroupName2, ReturnNum: insertNums }];
   var expMainExplains1 = [{ ScanType: "ixscan", IndexName: "a", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];
   var expMainExplains2 = [{ ScanType: "ixscan", IndexName: "b", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];

   //check primary
   var actCommExplains1 = getCommonExplain( dbCommCLPrimary1, findConf1 );
   var actCommExplains2 = getCommonExplain( dbCommCLPrimary2, findConf1 );
   var actHashExplains1 = getSplitExplain( dbHashCLPrimary1, findConf1 );
   var actHashExplains2 = getSplitExplain( dbHashCLPrimary2, findConf1 );
   var actRangExplains1 = getSplitExplain( dbRangCLPrimary1, findConf1 );
   var actRangExplains2 = getSplitExplain( dbRangCLPrimary2, findConf1 );
   var actHashExplains3 = getSplitExplain( dbHashCLPrimary1, findConf2 );
   var actHashExplains4 = getSplitExplain( dbHashCLPrimary2, findConf2 );
   var actRangExplains3 = getSplitExplain( dbRangCLPrimary1, findConf2 );
   var actRangExplains4 = getSplitExplain( dbRangCLPrimary2, findConf2 );
   var actMainExplains1 = getMainclExplain( dbMainCLPrimary, findConf1 );
   var actMainExplains2 = getMainclExplain( dbMainCLPrimary, findConf2 );

   checkExplain( actCommExplains1, expCommExplains );
   checkExplain( actCommExplains2, expCommExplains );
   checkExplain( actHashExplains1, expHashExplains1 );
   checkExplain( actHashExplains2, expHashExplains2 );
   checkExplain( actRangExplains1, expRangExplains1 );
   checkExplain( actRangExplains2, expRangExplains2 );
   checkExplain( actHashExplains3, expHashExplains3 );
   checkExplain( actHashExplains4, expHashExplains4 );
   checkExplain( actRangExplains3, expRangExplains3 );
   checkExplain( actRangExplains4, expRangExplains4 );
   checkExplain( actMainExplains1, expMainExplains1 );
   checkExplain( actMainExplains2, expMainExplains2 );

   //query
   query( dbCommCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf2, null, null, insertNums );
   query( dbRangCLPrimary1, findConf1, null, null, insertNums );
   query( dbRangCLPrimary1, findConf2, null, null, insertNums );
   query( dbCommCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf2, null, null, insertNums );
   query( dbRangCLPrimary2, findConf1, null, null, insertNums );
   query( dbRangCLPrimary2, findConf2, null, null, insertNums );
   query( dbMainCLPrimary, findConf1, null, null, insertNums );
   query( dbMainCLPrimary, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption11 = { Collection: clFullName11 };
   var accessFindOption12 = { Collection: clFullName12 };
   var accessFindOption13 = { Collection: clFullName13 };
   var accessFindOption21 = { Collection: clFullName21 };
   var accessFindOption22 = { Collection: clFullName22 };
   var accessFindOption23 = { Collection: clFullName23 };
   var accessFindOptionMainCL = { Collection: mainclFullName };

   var actAccessPlans11 = getCommonAccessPlans( db, accessFindOption11 );
   var actAccessPlans12 = getSplitAccessPlans( db, accessFindOption12 );
   var actAccessPlans13 = getSplitAccessPlans( db, accessFindOption13 );
   var actAccessPlans21 = getCommonAccessPlans( db, accessFindOption21 );
   var actAccessPlans22 = getSplitAccessPlans( db, accessFindOption22 );
   var actAccessPlans23 = getSplitAccessPlans( db, accessFindOption23 );
   //   var actAccessPlansMainCL = getMainclAccessPlans(db, accessFindOptionMainCL);

   var expAccessPlans11 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans12 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcHashGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcHashGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1 }];
   var expAccessPlans13 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destRangGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destRangGroupName1 }];
   var expAccessPlans21 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans22 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcHashGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcHashGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2 }];
   var expAccessPlans23 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destRangGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destRangGroupName2 }];
   var expAccessPlansMainCL = [{ ScanType: "ixscan", IndexName: "a", GroupName: subclGroupName },
   { ScanType: "ixscan", IndexName: "b", GroupName: subclGroupName }];

   checkSnapShotAccessPlans( clFullName11, expAccessPlans11, actAccessPlans11 );
   checkSnapShotAccessPlans( clFullName12, expAccessPlans12, actAccessPlans12 );
   checkSnapShotAccessPlans( clFullName13, expAccessPlans13, actAccessPlans13 );
   checkSnapShotAccessPlans( clFullName21, expAccessPlans21, actAccessPlans21 );
   checkSnapShotAccessPlans( clFullName22, expAccessPlans22, actAccessPlans22 );
   checkSnapShotAccessPlans( clFullName23, expAccessPlans23, actAccessPlans23 );
   //   checkMainclAccessPlans(expAccessPlansMainCL, actAccessPlansMainCL);


   //analyze
   db.analyze( );

   //check all groups consistency
   checkConsistency( db, null, null, groups );
   //check after analyze
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "$shard", true, true );
   checkStat( db, csName2, clName2, "$shard", true, true );
   checkStat( db, csName1, clName3, "$shard", true, true );
   checkStat( db, csName2, clName3, "$shard", true, true );
   checkStat( db, csName1, clName2, "b", true, true );
   checkStat( db, csName2, clName2, "b", true, true );
   checkStat( db, csName1, clName3, "b", true, true );
   checkStat( db, csName2, clName3, "b", true, true );
   checkStat( db, maincsName, subclName, "a", true, true );
   checkStat( db, maincsName, subclName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption11 = { Collection: clFullName11 };
   var accessFindOption12 = { Collection: clFullName12 };
   var accessFindOption13 = { Collection: clFullName13 };
   var accessFindOption21 = { Collection: clFullName21 };
   var accessFindOption22 = { Collection: clFullName22 };
   var accessFindOption23 = { Collection: clFullName23 };
   var accessFindOptionMainCL = { Collection: mainclFullName };

   var actAccessPlans11 = getCommonAccessPlans( db, accessFindOption11 );
   var actAccessPlans12 = getSplitAccessPlans( db, accessFindOption12 );
   var actAccessPlans13 = getSplitAccessPlans( db, accessFindOption13 );
   var actAccessPlans21 = getCommonAccessPlans( db, accessFindOption21 );
   var actAccessPlans22 = getSplitAccessPlans( db, accessFindOption22 );
   var actAccessPlans23 = getSplitAccessPlans( db, accessFindOption23 );
   var actAccessPlansMainCL = getCommonAccessPlans( db, accessFindOptionMainCL );

   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName11, expAccessPlans, actAccessPlans11 );
   checkSnapShotAccessPlans( clFullName12, expAccessPlans, actAccessPlans12 );
   checkSnapShotAccessPlans( clFullName13, expAccessPlans, actAccessPlans13 );
   checkSnapShotAccessPlans( clFullName21, expAccessPlans, actAccessPlans21 );
   checkSnapShotAccessPlans( clFullName22, expAccessPlans, actAccessPlans22 );
   checkSnapShotAccessPlans( clFullName23, expAccessPlans, actAccessPlans23 );
   //   checkMainclAccessPlans(expAccessPlans, actAccessPlansMainCL);

   //check the query explain of master/slave nodes 
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expCommExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expHashExplains1 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1, ReturnNum: insertNums }];
   var expHashExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2, ReturnNum: insertNums }];
   var expRangExplains1 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2, ReturnNum: insertNums }];
   var expHashExplains3 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1, ReturnNum: 0 }];
   var expHashExplains4 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2, ReturnNum: 0 }];
   var expRangExplains3 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2, ReturnNum: insertNums }];
   var expMainExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];
   var expMainExplains2 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];

   //check primary
   var actCommExplains1 = getCommonExplain( dbCommCLPrimary1, findConf1 );
   var actCommExplains2 = getCommonExplain( dbCommCLPrimary2, findConf1 );
   var actHashExplains1 = getSplitExplain( dbHashCLPrimary1, findConf1 );
   var actHashExplains2 = getSplitExplain( dbHashCLPrimary2, findConf1 );
   var actRangExplains1 = getSplitExplain( dbRangCLPrimary1, findConf1 );
   var actRangExplains2 = getSplitExplain( dbRangCLPrimary2, findConf1 );
   var actHashExplains3 = getSplitExplain( dbHashCLPrimary1, findConf2 );
   var actHashExplains4 = getSplitExplain( dbHashCLPrimary2, findConf2 );
   var actRangExplains3 = getSplitExplain( dbRangCLPrimary1, findConf2 );
   var actRangExplains4 = getSplitExplain( dbRangCLPrimary2, findConf2 );
   var actMainExplains1 = getMainclExplain( dbMainCLPrimary, findConf1 );
   var actMainExplains2 = getMainclExplain( dbMainCLPrimary, findConf2 );

   checkExplain( actCommExplains1, expCommExplains );
   checkExplain( actCommExplains2, expCommExplains );
   checkExplain( actHashExplains1, expHashExplains1 );
   checkExplain( actHashExplains2, expHashExplains2 );
   checkExplain( actRangExplains1, expRangExplains1 );
   checkExplain( actRangExplains2, expRangExplains2 );
   checkExplain( actHashExplains3, expHashExplains3 );
   checkExplain( actHashExplains4, expHashExplains4 );
   checkExplain( actRangExplains3, expRangExplains3 );
   checkExplain( actRangExplains4, expRangExplains4 );
   checkExplain( actMainExplains1, expMainExplains1 );
   checkExplain( actMainExplains2, expMainExplains2 );

   //query
   query( dbCommCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf2, null, null, insertNums );
   query( dbRangCLPrimary1, findConf1, null, null, insertNums );
   query( dbRangCLPrimary1, findConf2, null, null, insertNums );
   query( dbCommCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf2, null, null, insertNums );
   query( dbRangCLPrimary2, findConf1, null, null, insertNums );
   query( dbRangCLPrimary2, findConf2, null, null, insertNums );
   query( dbMainCLPrimary, findConf1, null, null, insertNums );
   query( dbMainCLPrimary, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption11 = { Collection: clFullName11 };
   var accessFindOption12 = { Collection: clFullName12 };
   var accessFindOption13 = { Collection: clFullName13 };
   var accessFindOption21 = { Collection: clFullName21 };
   var accessFindOption22 = { Collection: clFullName22 };
   var accessFindOption23 = { Collection: clFullName23 };
   var accessFindOptionMainCL = { Collection: mainclFullName };

   var actAccessPlans11 = getCommonAccessPlans( db, accessFindOption11 );
   var actAccessPlans12 = getSplitAccessPlans( db, accessFindOption12 );
   var actAccessPlans13 = getSplitAccessPlans( db, accessFindOption13 );
   var actAccessPlans21 = getCommonAccessPlans( db, accessFindOption21 );
   var actAccessPlans22 = getSplitAccessPlans( db, accessFindOption22 );
   var actAccessPlans23 = getSplitAccessPlans( db, accessFindOption23 );
   //   var actAccessPlansMainCL = getMainclAccessPlans(db, accessFindOptionMainCL);

   var expAccessPlans11 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans12 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1 }];
   var expAccessPlans13 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1 }];
   var expAccessPlans21 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans22 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2 }];
   var expAccessPlans23 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2 }];
   var expAccessPlansMainCL = [{ ScanType: "tbscan", IndexName: "", GroupName: subclGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: subclGroupName }];

   checkSnapShotAccessPlans( clFullName11, expAccessPlans11, actAccessPlans11 );
   checkSnapShotAccessPlans( clFullName12, expAccessPlans12, actAccessPlans12 );
   checkSnapShotAccessPlans( clFullName13, expAccessPlans13, actAccessPlans13 );
   checkSnapShotAccessPlans( clFullName21, expAccessPlans21, actAccessPlans21 );
   checkSnapShotAccessPlans( clFullName22, expAccessPlans22, actAccessPlans22 );
   checkSnapShotAccessPlans( clFullName23, expAccessPlans23, actAccessPlans23 );
   //   checkMainclAccessPlans(expAccessPlansMainCL, actAccessPlansMainCL);


   //analyze
   db.analyze( { Mode: 2 } );

   //check all groups consistency
   checkConsistency( db, null, null, groups );
   //check after analyze
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "$shard", true, true );
   checkStat( db, csName2, clName2, "$shard", true, true );
   checkStat( db, csName1, clName3, "$shard", true, true );
   checkStat( db, csName2, clName3, "$shard", true, true );
   checkStat( db, csName1, clName2, "b", true, true );
   checkStat( db, csName2, clName2, "b", true, true );
   checkStat( db, csName1, clName3, "b", true, true );
   checkStat( db, csName2, clName3, "b", true, true );
   checkStat( db, maincsName, subclName, "a", true, true );
   checkStat( db, maincsName, subclName, "b", true, true );

   //check out snapshot access plans
   var accessFindOption11 = { Collection: clFullName11 };
   var accessFindOption12 = { Collection: clFullName12 };
   var accessFindOption13 = { Collection: clFullName13 };
   var accessFindOption21 = { Collection: clFullName21 };
   var accessFindOption22 = { Collection: clFullName22 };
   var accessFindOption23 = { Collection: clFullName23 };
   var accessFindOptionMainCL = { Collection: mainclFullName };

   var actAccessPlans11 = getCommonAccessPlans( db, accessFindOption11 );
   var actAccessPlans12 = getSplitAccessPlans( db, accessFindOption12 );
   var actAccessPlans13 = getSplitAccessPlans( db, accessFindOption13 );
   var actAccessPlans21 = getCommonAccessPlans( db, accessFindOption21 );
   var actAccessPlans22 = getSplitAccessPlans( db, accessFindOption22 );
   var actAccessPlans23 = getSplitAccessPlans( db, accessFindOption23 );
   var actAccessPlansMainCL = getCommonAccessPlans( db, accessFindOptionMainCL );

   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName11, expAccessPlans, actAccessPlans11 );
   checkSnapShotAccessPlans( clFullName12, expAccessPlans, actAccessPlans12 );
   checkSnapShotAccessPlans( clFullName13, expAccessPlans, actAccessPlans13 );
   checkSnapShotAccessPlans( clFullName21, expAccessPlans, actAccessPlans21 );
   checkSnapShotAccessPlans( clFullName22, expAccessPlans, actAccessPlans22 );
   checkSnapShotAccessPlans( clFullName23, expAccessPlans, actAccessPlans23 );
   //   checkMainclAccessPlans(expAccessPlans, actAccessPlansMainCL);

   //check the query explain of master/slave nodes 
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expCommExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums }];
   var expHashExplains1 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1, ReturnNum: insertNums }];
   var expHashExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2, ReturnNum: insertNums }];
   var expRangExplains1 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2, ReturnNum: insertNums }];
   var expHashExplains3 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1, ReturnNum: 0 }];
   var expHashExplains4 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2, ReturnNum: 0 }];
   var expRangExplains3 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1, ReturnNum: insertNums }];
   var expRangExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2, ReturnNum: insertNums }];
   var expMainExplains1 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];
   var expMainExplains2 = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNums, Name: subclFullName, GroupName: subclGroupName }];

   //check primary
   var actCommExplains1 = getCommonExplain( dbCommCLPrimary1, findConf1 );
   var actCommExplains2 = getCommonExplain( dbCommCLPrimary2, findConf1 );
   var actHashExplains1 = getSplitExplain( dbHashCLPrimary1, findConf1 );
   var actHashExplains2 = getSplitExplain( dbHashCLPrimary2, findConf1 );
   var actRangExplains1 = getSplitExplain( dbRangCLPrimary1, findConf1 );
   var actRangExplains2 = getSplitExplain( dbRangCLPrimary2, findConf1 );
   var actHashExplains3 = getSplitExplain( dbHashCLPrimary1, findConf2 );
   var actHashExplains4 = getSplitExplain( dbHashCLPrimary2, findConf2 );
   var actRangExplains3 = getSplitExplain( dbRangCLPrimary1, findConf2 );
   var actRangExplains4 = getSplitExplain( dbRangCLPrimary2, findConf2 );
   var actMainExplains1 = getMainclExplain( dbMainCLPrimary, findConf1 );
   var actMainExplains2 = getMainclExplain( dbMainCLPrimary, findConf2 );

   checkExplain( actCommExplains1, expCommExplains );
   checkExplain( actCommExplains2, expCommExplains );
   checkExplain( actHashExplains1, expHashExplains1 );
   checkExplain( actHashExplains2, expHashExplains2 );
   checkExplain( actRangExplains1, expRangExplains1 );
   checkExplain( actRangExplains2, expRangExplains2 );
   checkExplain( actHashExplains3, expHashExplains3 );
   checkExplain( actHashExplains4, expHashExplains4 );
   checkExplain( actRangExplains3, expRangExplains3 );
   checkExplain( actRangExplains4, expRangExplains4 );
   checkExplain( actMainExplains1, expMainExplains1 );
   checkExplain( actMainExplains2, expMainExplains2 );

   //query
   query( dbCommCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf1, null, null, insertNums );
   query( dbHashCLPrimary1, findConf2, null, null, insertNums );
   query( dbRangCLPrimary1, findConf1, null, null, insertNums );
   query( dbRangCLPrimary1, findConf2, null, null, insertNums );
   query( dbCommCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf1, null, null, insertNums );
   query( dbHashCLPrimary2, findConf2, null, null, insertNums );
   query( dbRangCLPrimary2, findConf1, null, null, insertNums );
   query( dbRangCLPrimary2, findConf2, null, null, insertNums );
   query( dbMainCLPrimary, findConf1, null, null, insertNums );
   query( dbMainCLPrimary, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption11 = { Collection: clFullName11 };
   var accessFindOption12 = { Collection: clFullName12 };
   var accessFindOption13 = { Collection: clFullName13 };
   var accessFindOption21 = { Collection: clFullName21 };
   var accessFindOption22 = { Collection: clFullName22 };
   var accessFindOption23 = { Collection: clFullName23 };
   var accessFindOptionMainCL = { Collection: mainclFullName };

   var actAccessPlans11 = getCommonAccessPlans( db, accessFindOption11 );
   var actAccessPlans12 = getSplitAccessPlans( db, accessFindOption12 );
   var actAccessPlans13 = getSplitAccessPlans( db, accessFindOption13 );
   var actAccessPlans21 = getCommonAccessPlans( db, accessFindOption21 );
   var actAccessPlans22 = getSplitAccessPlans( db, accessFindOption22 );
   var actAccessPlans23 = getSplitAccessPlans( db, accessFindOption23 );
   //   var actAccessPlansMainCL = getMainclAccessPlans(db, accessFindOptionMainCL);

   var expAccessPlans11 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans12 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName1 }];
   var expAccessPlans13 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName1 }];
   var expAccessPlans21 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans22 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcHashGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destHashGroupName2 }];
   var expAccessPlans23 = [{ ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcRangGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destRangGroupName2 }];
   var expAccessPlansMainCL = [{ ScanType: "tbscan", IndexName: "", GroupName: subclGroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: subclGroupName }];

   checkSnapShotAccessPlans( clFullName11, expAccessPlans11, actAccessPlans11 );
   checkSnapShotAccessPlans( clFullName12, expAccessPlans12, actAccessPlans12 );
   checkSnapShotAccessPlans( clFullName13, expAccessPlans13, actAccessPlans13 );
   checkSnapShotAccessPlans( clFullName21, expAccessPlans21, actAccessPlans21 );
   checkSnapShotAccessPlans( clFullName22, expAccessPlans22, actAccessPlans22 );
   checkSnapShotAccessPlans( clFullName23, expAccessPlans23, actAccessPlans23 );
   //   checkMainclAccessPlans(expAccessPlansMainCL, actAccessPlansMainCL);


   db1.close();
   commDropCS( db, csName1, true, "drop CS in the end" );
   commDropCS( db, csName2, true, "drop CS in the end" );
   commDropCS( db, maincsName, true, "drop CS in the end" );
}
