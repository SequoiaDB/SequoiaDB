/************************************
*@Description: alter cl更新统计信息
*@author:      liuxiaoxuan
*@createdate:  2017.11.09
*@testlinkCase: seqDB-12981
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.skipOneDuplicatePerGroup = true;
testConf.csName = COMMCSNAME + "_12981";

main( test );

function test ()
{
   var groups = commGetDataGroupNames( db );

   var csName = COMMCSNAME + "_12981";

   //create CLs
   var clName1 = COMMCLNAME + "_12981_1";
   var clName2 = COMMCLNAME + "_12981_2";
   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;

   commDropCL( db, csName, clName1 );
   commDropCL( db, csName, clName2 );

   var dbcl1 = commCreateCL( db, csName, clName1 );
   var dbcl2 = commCreateCL( db, csName, clName2 );

   //get master datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m", PreferedPeriod: -1 } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );

   //create index
   commCreateIndex( dbcl1, "b", { b: 1 } );
   commCreateIndex( dbcl2, "c", { c: 1 } );

   //insert data
   var insertNums = 5000;
   var sameValues = 9000;
   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   insertDiffDatas( dbcl2, insertNums );
   insertSameDatas( dbcl2, insertNums, sameValues );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check before invoke analyze
   checkStat( db, csName, clName1, "$shard", false, false );
   checkStat( db, csName, clName2, "$shard", false, false );
   checkStat( db, csName, clName1, "b", false, false );
   checkStat( db, csName, clName2, "c", false, false );

   //query from primary node
   var findConf1 = { a0: 9000 };
   var findConf2 = { a1: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

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

   //invoke analyze
   var options = { CollectionSpace: csName };
   db.analyze( options );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check after analyze before alter
   checkStat( db, csName, clName1, "$shard", true, false );
   checkStat( db, csName, clName2, "$shard", true, false );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "c", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //query from primary node
   var findConf1 = { a0: 9000 };
   var findConf2 = { a1: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

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

   //alter CLs
   var alterOption1 = { ShardingKey: { a0: 1 }, ShardingType: 'hash' };
   var alterOption2 = { ShardingKey: { a1: 1 }, ShardingType: 'range' };
   dbcl1.alter( alterOption1 );
   dbcl2.alter( alterOption2 );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //check alter before analyze
   checkStat( db, csName, clName1, "$shard", true, false );
   checkStat( db, csName, clName2, "$shard", true, false );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "c", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //query from primary node
   var findConf1 = { a0: 9000 };
   var findConf2 = { a1: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "$shard" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "$shard" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //split CLs
   var group1 = ClSplitOneTimes( csName, clName1, 50 );
   var group2 = ClSplitOneTimes( csName, clName2, 50 );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   var srcGroupName1 = group1[0].GroupName;
   var destGroupName1 = group1[1].GroupName;
   var srcGroupName2 = group2[0].GroupName;
   var destGroupName2 = group2[1].GroupName;

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ GroupName: srcGroupName1, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName1, ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ GroupName: srcGroupName2, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName2, ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //query from primary/slave node
   var findConf1 = { a0: 9000 };
   var findConf2 = { a1: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ GroupName: srcGroupName1, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName1, ScanType: "tbscan", IndexName: "" },
   { GroupName: destGroupName1, ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ GroupName: srcGroupName2, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName2, ScanType: "tbscan", IndexName: "" },
   { GroupName: destGroupName2, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: destGroupName2, ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //check alter after analyze
   var options = { CollectionSpace: csName };
   db.analyze( options );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "c", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //query from primary node
   var findConf1 = { a0: 9000 };
   var findConf2 = { a1: 9000 };

   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ GroupName: srcGroupName1, ScanType: "tbscan", IndexName: "" },
   { GroupName: srcGroupName1, ScanType: "tbscan", IndexName: "" },
   { GroupName: destGroupName1, ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ GroupName: srcGroupName2, ScanType: "tbscan", IndexName: "" },
   { GroupName: destGroupName2, ScanType: "tbscan", IndexName: "" },
   { GroupName: destGroupName2, ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

}
