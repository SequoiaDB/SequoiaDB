/************************************
*@Description: 指定切分表所在cs收集统计信息
*@author:      liuxiaoxuan
*@createdate:  2017.11.08
*@testlinkCase: seqDB-11609
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

   var csName = COMMCSNAME + "11609";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create CLs
   var clOption1 = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   var clName1 = csName + "11609_1";
   var dbcl1 = commCreateCL( db, csName, clName1, clOption1, true );

   var clOption2 = { ShardingKey: { a: 1 }, ShardingType: "range" };
   var clName2 = csName + "11609_2";
   var dbcl2 = commCreateCL( db, csName, clName2, clOption2, true );

   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );

   //insert datas
   var insertNums = 3000;
   var sameValues = 9000;

   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   insertDiffDatas( dbcl2, insertNums );
   insertSameDatas( dbcl2, insertNums, sameValues );

   //create index
   commCreateIndex( dbcl1, "b", { b: 1 } );
   commCreateIndex( dbcl2, "b", { b: 1 } );

   //split cl
   ClSplitOneTimes( csName, clName1, 50 );
   ClSplitOneTimes( csName, clName2, 50 );

   //check before invoke analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", false, false );
   checkStat( db, csName, clName1, "b", false, false );
   checkStat( db, csName, clName2, "$shard", false, false );
   checkStat( db, csName, clName2, "b", false, false );

   //check the query explain of master/slave nodes
   var groups1 = getSplitGroups( csName, clName1, 1 );
   var groups2 = getSplitGroups( csName, clName2, 1 );

   var srcGroupName1 = groups1[0].GroupName;
   var destGroupName1 = groups1[1].GroupName;
   var srcGroupName2 = groups2[0].GroupName;
   var destGroupName2 = groups2[1].GroupName;

   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expExplains1 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1, ReturnNum: 0 }];
   var expExplains3 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destGroupName2, ReturnNum: insertNums }];
   var expExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2, ReturnNum: 0 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName2, ReturnNum: insertNums }];

   var actExplains1 = getSplitExplain( dbclPrimary1, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary1, findConf2 );
   var actExplains3 = getSplitExplain( dbclPrimary2, findConf1 );
   var actExplains4 = getSplitExplain( dbclPrimary2, findConf2 );

   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );
   checkExplain( actExplains3, expExplains3 );
   checkExplain( actExplains4, expExplains4 );

   //query
   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: srcGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: destGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName2 }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //invoke analyze
   var options = { CollectionSpace: csName };
   db.analyze( options );

   //check after analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //check the query explain of master/slave nodes
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1, ReturnNum: 0 }];
   var expExplains3 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2, ReturnNum: insertNums }];
   var expExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2, ReturnNum: insertNums }];

   var actExplains1 = getSplitExplain( dbclPrimary1, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary1, findConf2 );
   var actExplains3 = getSplitExplain( dbclPrimary2, findConf1 );
   var actExplains4 = getSplitExplain( dbclPrimary2, findConf2 );

   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );
   checkExplain( actExplains3, expExplains3 );
   checkExplain( actExplains4, expExplains4 );

   //query
   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //invoke analyze
   var options = { Mode: 2, CollectionSpace: csName };
   db.analyze( options );

   //check after analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //check the query explain of master/slave nodes
   var findConf1 = { a: 9000 };
   var findConf2 = { b: 9000 };

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1, ReturnNum: insertNums },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1, ReturnNum: 0 }];
   var expExplains3 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2, ReturnNum: insertNums }];
   var expExplains4 = [{ ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2, ReturnNum: 0 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2, ReturnNum: insertNums }];

   var actExplains1 = getSplitExplain( dbclPrimary1, findConf1 );
   var actExplains2 = getSplitExplain( dbclPrimary1, findConf2 );
   var actExplains3 = getSplitExplain( dbclPrimary2, findConf1 );
   var actExplains4 = getSplitExplain( dbclPrimary2, findConf2 );

   checkExplain( actExplains1, expExplains1 );
   checkExplain( actExplains2, expExplains2 );
   checkExplain( actExplains3, expExplains3 );
   checkExplain( actExplains4, expExplains4 );

   //query
   query( dbclPrimary1, findConf1, null, null, insertNums );
   query( dbclPrimary1, findConf2, null, null, insertNums );
   query( dbclPrimary2, findConf1, null, null, insertNums );
   query( dbclPrimary2, findConf2, null, null, insertNums );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName1 },
   { ScanType: "ixscan", IndexName: "b", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 },
   { ScanType: "ixscan", IndexName: "b", GroupName: srcGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 }];
   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}
