/************************************
*@Description: 指定切分cl生成默认统计信息并手工修改统计信息再清空
*@author:      liuxiaoxuan
*@createdate:  2017.11.10
*@testlinkCase: seqDB-11624
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

   var csName = COMMCSNAME + "11624";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   commCreateCS( db, csName, false, "" );

   //create CLs
   var clOption1 = { ShardingKey: { a: 1 }, ShardingType: "hash" };
   var clName1 = COMMCLNAME + "11624_1";
   var dbcl1 = commCreateCL( db, csName, clName1, clOption1, true );

   var clOption2 = { ShardingKey: { a: 1 }, ShardingType: "range" };
   var clName2 = COMMCLNAME + "11624_2";
   var dbcl2 = commCreateCL( db, csName, clName2, clOption2, true );

   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;

   //get master/slave datanode
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );

   commCreateIndex( dbcl1, "b", { b: 1 } );
   commCreateIndex( dbcl2, "b", { b: -1 } );

   //insert datas
   var insertNums = 10000;
   var sameValues = 90002;

   insertDiffDatas( dbcl1, insertNums );
   insertSameDatas( dbcl1, insertNums, sameValues );
   insertDiffDatas( dbcl2, insertNums );
   insertSameDatas( dbcl2, insertNums, sameValues );

   //split cl
   var groups1 = ClSplitOneTimes( csName, clName1, 50 );
   var groups2 = ClSplitOneTimes( csName, clName2, 50 );

   //check before invoke analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", false, false );
   checkStat( db, csName, clName1, "b", false, false );
   checkStat( db, csName, clName2, "$shard", false, false );
   checkStat( db, csName, clName2, "b", false, false );

   //get split group
   var srcGroupName1 = groups1[0].GroupName;
   var destGroupName1 = groups1[1].GroupName;
   var srcGroupName2 = groups2[0].GroupName;
   var destGroupName2 = groups2[1].GroupName;

   //analyze
   var options1 = { Collection: csName + "." + clName1 };
   db.analyze( options1 );
   var options2 = { Collection: csName + "." + clName2 };
   db.analyze( options2 );

   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
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
   var findConf1 = { a: sameValues };
   var findConf2 = { b: sameValues };

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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

   //analyze default statistics infomation
   var options1 = { Mode: 3, Collection: csName + "." + clName1 };
   db.analyze( options1 );
   var options2 = { Mode: 3, Collection: csName + "." + clName2 };
   db.analyze( options2 );

   //check analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, false );
   checkStat( db, csName, clName2, "$shard", true, false );
   checkStat( db, csName, clName1, "b", true, false );
   checkStat( db, csName, clName2, "b", true, false );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //check the query explain of master/slave nodes
   var expExplains1 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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

   //modify SYSSTAT info
   var mcvValues1 = [{ a: 0 }, { a: 1 }, { a: sameValues }];
   var mcvValues2 = [{ b: 0 }, { b: 1 }, { b: sameValues }];
   var fracs = [500, 500, 9000];

   updateIndexStateInfo( db, csName, clName1, "$shard", mcvValues1, fracs );
   updateIndexStateInfo( db, csName, clName2, "$shard", mcvValues1, fracs );
   updateIndexStateInfo( db, csName, clName1, "b", mcvValues2, fracs );
   updateIndexStateInfo( db, csName, clName2, "b", mcvValues2, fracs );

   //reload analyze
   var options1 = { Mode: 4, Collection: csName + "." + clName1 };
   db.analyze( options1 );
   var options2 = { Mode: 4, Collection: csName + "." + clName2 };
   db.analyze( options2 );

   //check analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //truncate invalidate
   var options1 = { Mode: 5, Collection: csName + "." + clName1 };
   db.analyze( options1 );
   var options2 = { Mode: 5, Collection: csName + "." + clName2 };
   db.analyze( options2 );

   //check analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //modify SYSSTAT info again
   var mcvValues1 = [{ a: 0 }, { a: 1 }, { a: sameValues }];
   var mcvValues2 = [{ b: 0 }, { b: 1 }, { b: sameValues }];
   var fracs = [500, 500, 500];

   updateIndexStateInfo( db, csName, clName1, "$shard", mcvValues1, fracs );
   updateIndexStateInfo( db, csName, clName2, "$shard", mcvValues1, fracs );
   updateIndexStateInfo( db, csName, clName1, "b", mcvValues2, fracs );
   updateIndexStateInfo( db, csName, clName2, "b", mcvValues2, fracs );

   //check analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   var expExplains1 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "tbscan", IndexName: "",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName1 }];

   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: srcGroupName2 },
   { ScanType: "tbscan", IndexName: "", GroupName: destGroupName2 }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //reload analyze
   var options1 = { Mode: 5, Collection: csName + "." + clName1 };
   db.analyze( options1 );
   var options2 = { Mode: 5, Collection: csName + "." + clName2 };
   db.analyze( options2 );

   //check analyze
   checkConsistency( db, csName, clName1 );
   checkConsistency( db, csName, clName2 );
   checkStat( db, csName, clName1, "$shard", true, true );
   checkStat( db, csName, clName2, "$shard", true, true );
   checkStat( db, csName, clName1, "b", true, true );
   checkStat( db, csName, clName2, "b", true, true );

   //check out snapshot access plans
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getSplitAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getSplitAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   var expExplains1 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: srcGroupName1, ReturnNum: insertNums
      }];
   var expExplains2 = [
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: srcGroupName1, ReturnNum: insertNums
      },
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: destGroupName1, ReturnNum: 0
      }];
   var expExplains3 = [
      {
         ScanType: "ixscan", IndexName: "$shard",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];
   var expExplains4 = [
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: srcGroupName2, ReturnNum: 0
      },
      {
         ScanType: "ixscan", IndexName: "b",
         GroupName: destGroupName2, ReturnNum: insertNums
      }];

   //check primary
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

   db1.close();
   commDropCS( db, csName, true, "drop CS in the end" );
}
