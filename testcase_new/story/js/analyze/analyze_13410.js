/************************************
*@Description: 主子表中指定索引收集统计信息
*@author:      zhaoyu
*@createdate:  2017.12.22
*@testlinkCase:seqDB-13410
**************************************/
var maincsName = COMMCSNAME + "_maincs_13410";
var subcsName1 = COMMCSNAME + "_subcs_13410_1";
var mainclName = COMMCLNAME + "_maincl_13410";
var subclName1 = COMMCLNAME + "_subcl_13410_1";
var subclName2 = COMMCLNAME + "_subcl_13410_2";
var subclName3 = COMMCLNAME + "_subcl_13410_3";
var subclName4 = COMMCLNAME + "_subcl_13410_4";
var mainclFullName = maincsName + "." + mainclName;
var subclFullName1 = maincsName + "." + subclName1;
var subclFullName2 = maincsName + "." + subclName2;
var subclFullName3 = subcsName1 + "." + subclName3;
var subclFullName4 = subcsName1 + "." + subclName4;

var maincl;

var srcGroupName;
var desGroupName;

var insertDiffNum = 16000;
var insertSameNum = 2000;

var db1;
var dbclPrimary;

var subcl2;
var subcl4;

main( test );
function test ()
{
   //判断独立模式
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //判断1组模式
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   //清理环境
   commDropCS( db, subcsName1, true, "drop subcs before test" );
   commDropCS( db, maincsName, true, "drop maincs before test" );

   //获取数据组
   var temp = commGetGroups( db );
   if( commIsStandalone( db ) == false )
   {
      srcGroupName = temp[0][0].GroupName;
      desGroupName = temp[1][0].GroupName;
   }

   //创建主表cl
   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 }, ShardingType: "range" };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   //创建子表cl
   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: srcGroupName };
   commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: srcGroupName };
   subcl2 = commCreateCL( db, maincsName, subclName2, subclOption2 );
   commCreateCL( db, subcsName1, subclName3, subclOption1 );
   subcl4 = commCreateCL( db, subcsName1, subclName4, subclOption2 );

   //子表切分
   split( maincsName, subclName1, srcGroupName, desGroupName, { a0: 2000 }, { a0: 4000 } );
   split( maincsName, subclName2, srcGroupName, desGroupName, 50, null );
   split( subcsName1, subclName3, srcGroupName, desGroupName, { a0: 10000 }, { a0: 12000 } );
   split( subcsName1, subclName4, srcGroupName, desGroupName, 50, null );

   //attach cl
   maincl.attachCL( subclFullName2, { LowBound: { a: 4000 }, UpBound: { a: 8000 } } );
   maincl.attachCL( subclFullName4, { LowBound: { a: 12000 }, UpBound: { a: 16000 } } );
   maincl.attachCL( subclFullName1, { LowBound: { a: 0 }, UpBound: { a: 4000 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a: 8000 }, UpBound: { a: 12000 } } );

   //创建索引
   commCreateIndex( maincl, "a1", { a1: 1 } );

   //插入记录
   insertDiffDatas( maincl, insertDiffNum );
   insertSameDatas( maincl, insertSameNum, 0 );
   insertSameDatas( maincl, insertSameNum, 10000 );

   //获取主备节点
   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   dbclPrimary = db1.getCS( maincsName ).getCL( mainclName );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", false, false );
   checkStat( db, maincsName, subclName2, "$shard", false, false );
   checkStat( db, subcsName1, subclName3, "$shard", false, false );
   checkStat( db, subcsName1, subclName4, "$shard", false, false );

   checkStat( db, maincsName, subclName1, "a1", false, false );
   checkStat( db, maincsName, subclName2, "a1", false, false );
   checkStat( db, subcsName1, subclName3, "a1", false, false );
   checkStat( db, subcsName1, subclName4, "a1", false, false );

   //执行查询
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainBeforeAnalyze();

   //指定主表cl收集shard索引的统计信息
   db.analyze( { Collection: mainclFullName, Index: "$shard" } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", true, true );
   checkStat( db, maincsName, subclName2, "$shard", true, true );
   checkStat( db, subcsName1, subclName3, "$shard", true, true );
   checkStat( db, subcsName1, subclName4, "$shard", true, true );

   checkStat( db, maincsName, subclName1, "a1", true, false );
   checkStat( db, maincsName, subclName2, "a1", true, false );
   checkStat( db, subcsName1, subclName3, "a1", true, false );
   checkStat( db, subcsName1, subclName4, "a1", true, false );

   //检查访问计划快照
   var expAccessPlan = [];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMainclShardIndex();

   //指定主表cl收集shard索引的统计信息
   db.analyze( { Mode: 2, Collection: mainclFullName, Index: "$shard" } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", true, true );
   checkStat( db, maincsName, subclName2, "$shard", true, true );
   checkStat( db, subcsName1, subclName3, "$shard", true, true );
   checkStat( db, subcsName1, subclName4, "$shard", true, true );

   checkStat( db, maincsName, subclName1, "a1", true, false );
   checkStat( db, maincsName, subclName2, "a1", true, false );
   checkStat( db, subcsName1, subclName3, "a1", true, false );
   checkStat( db, subcsName1, subclName4, "a1", true, false );

   //检查访问计划快照
   var expAccessPlan = [];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMainclShardIndex();

   //生成默认统计信息
   db.analyze( { Collection: mainclFullName, Mode: 3 } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", true, false );
   checkStat( db, maincsName, subclName2, "$shard", true, false );
   checkStat( db, subcsName1, subclName3, "$shard", true, false );
   checkStat( db, subcsName1, subclName4, "$shard", true, false );

   checkStat( db, maincsName, subclName1, "a1", true, false );
   checkStat( db, maincsName, subclName2, "a1", true, false );
   checkStat( db, subcsName1, subclName3, "a1", true, false );
   checkStat( db, subcsName1, subclName4, "a1", true, false );

   //检查访问计划快照
   var expAccessPlan = [];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainBeforeAnalyze();

   //指定主表cl收集普通索引的统计信息
   db.analyze( { Collection: mainclFullName, Index: "a1" } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", true, false );
   checkStat( db, maincsName, subclName2, "$shard", true, false );
   checkStat( db, subcsName1, subclName3, "$shard", true, false );
   checkStat( db, subcsName1, subclName4, "$shard", true, false );

   checkStat( db, maincsName, subclName1, "a1", true, true );
   checkStat( db, maincsName, subclName2, "a1", true, true );
   checkStat( db, subcsName1, subclName3, "a1", true, true );
   checkStat( db, subcsName1, subclName4, "a1", true, true );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a0: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );
   var findConf = { a1: { $in: [0, 10000] } };
   query( dbclPrimary, findConf, null, null, ( insertSameNum + 1 ) * 2 );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMainclCommonIndex();

   //删除主表cs、其他cs下的子表中的a1索引
   commDropIndex( subcl2, "a1" );
   commDropIndex( subcl4, "a1" );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "a1", true, true );
   checkStat( db, maincsName, subclName2, "a1", true, false );
   checkStat( db, subcsName1, subclName3, "a1", true, true );
   checkStat( db, subcsName1, subclName4, "a1", true, false );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: srcGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat( tmp ); 
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterDropSubclCommIndex();

   //指定主表cl的普通索引执行统计
   db.analyze( { Collection: mainclFullName, Index: "a1" } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterDropSubclCommIndex();

   //指定不存在的索引执行统计
   try
   {
      db.analyze( { Collection: mainclFullName, Index: "d" } );
      throw new Error( "NEED_ERR" );
   }
   catch( e )
   {
      if( SDB_IXM_NOTEXIST != e.message )
      {
         throw e;
      }
   }

   //清理环境
   commDropCS( db, subcsName1 );
   commDropCS( db, maincsName );
   db1.close();

}

function checkExplainBeforeAnalyze ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

}

function checkExplainAfterAnalyzeMainclShardIndex ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

}

function checkExplainAfterAnalyzeMainclCommonIndex ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

}

function checkExplainAfterDropSubclCommIndex ()
{
   //索引键查询
   var findConf = { a1: { $in: [0, 10000] } };
   var expExplains = [
      {
         GroupName: srcGroupName, Name: subclFullName1,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: desGroupName, Name: subclFullName1,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName2,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName2,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: srcGroupName, Name: subclFullName3,
         ScanType: "ixscan", IndexName: "a1", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName3,
         ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
      },
      {
         GroupName: srcGroupName, Name: subclFullName4,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      },
      {
         GroupName: desGroupName, Name: subclFullName4,
         ScanType: "tbscan", IndexName: "", ReturnNum: 0
      }];

   var actExplains = getMainclExplain( dbclPrimary, findConf );
   checkExplain( actExplains, expExplains );

}