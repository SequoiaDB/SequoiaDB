/************************************
*@Description: 主子表指定Group参数执行统计
*@author:      zhaoyu
*@createdate:  2018.2.5
*@testlinkCase:seqDB-14419
**************************************/
var maincsName = COMMCSNAME + "_maincs_14419";
var subcsName1 = COMMCSNAME + "_subcs_14419_1";
var mainclName = COMMCLNAME + "_maincl_14419";
var subclName1 = COMMCLNAME + "_subcl_14419_1";
var subclName2 = COMMCLNAME + "_subcl_14419_2";
var subclName3 = COMMCLNAME + "_subcl_14419_3";
var subclName4 = COMMCLNAME + "_subcl_14419_4";
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
   commCreateCL( db, maincsName, subclName2, subclOption2 );
   commCreateCL( db, subcsName1, subclName3, subclOption1 );
   commCreateCL( db, subcsName1, subclName4, subclOption2 );

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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //指定group执行统计
   db.analyze( { GroupName: srcGroupName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat(tmp);
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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincl();

   //指定group执行统计
   db.analyze( { Mode: 2, GroupName: srcGroupName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat(tmp);
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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAfterAnalyzeMaincl();

   //指定主表cl并指定group生成默认统计信息
   db.analyze( { Collection: mainclFullName, Mode: 3, GroupName: srcGroupName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat(tmp);
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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAnalyzeSetMode3();

   //手工修改主节点$shard索引统计信息
   var mcvValues = [{ a0: 0 }, { a0: 10000 }, { a0: 10001 }];
   var fracs = [5000, 5000, 50];
   updateIndexStateInfo( db, maincsName, subclName1, "$shard", mcvValues, fracs );
   var mcvValues = [{ a0: 0 }, { a0: 10000 }, { a0: 10001 }];
   var fracs = [5000, 5000, 50];
   updateIndexStateInfo( db, subcsName1, subclName3, "$shard", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { GroupName: srcGroupName, Mode: 4 } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat(tmp);
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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAnalyzeModeSet4();

   //手工修改统计统计信息
   var mcvValues = [{ a0: 0 }, { a0: 10000 }, { a0: 10001 }];
   var fracs = [50, 50, 50];
   updateIndexStateInfo( db, maincsName, subclName1, "$shard", mcvValues, fracs );
   var mcvValues = [{ a0: 0 }, { a0: 10000 }, { a0: 10001 }];
   var fracs = [50, 50, 50];
   updateIndexStateInfo( db, subcsName1, subclName3, "$shard", mcvValues, fracs );

   //清空统计信息
   db.analyze( { GroupName: srcGroupName, Mode: 5 } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "$shard" },
   { GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" }];
   //var expAccessPlan = tmp.concat(tmp);
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
   //var expAccessPlan = tmp.concat(tmp);
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //主备节点上检查访问计划
   checkExplainAnalyzeSetMode3();

   //清理环境
   commDropCS( db, subcsName1 );
   commDropCS( db, maincsName );
   db1.close();

}

function checkExplainAnalyzeSetMode3 ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [{
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
   var expExplains = [{
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


function checkExplainAfterAnalyzeMaincl ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [{
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
   var expExplains = [{
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

function checkExplainAnalyzeModeSet4 ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [{
      GroupName: srcGroupName, Name: subclFullName1,
      ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
   },
   {
      GroupName: srcGroupName, Name: subclFullName2,
      ScanType: "ixscan", IndexName: "$shard", ReturnNum: 0
   },
   {
      GroupName: srcGroupName, Name: subclFullName3,
      ScanType: "tbscan", IndexName: "", ReturnNum: 0
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
   var expExplains = [{
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

function checkExplainAfterModifyCommonIndexStat ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [{
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
   var expExplains = [{
      GroupName: srcGroupName, Name: subclFullName1,
      ScanType: "tbscan", IndexName: "", ReturnNum: insertSameNum + 1
   },
   {
      GroupName: desGroupName, Name: subclFullName1,
      ScanType: "tbscan", IndexName: "", ReturnNum: 0
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
      ScanType: "tbscan", IndexName: "", ReturnNum: 0
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

function checkExplainAnalyzeCommonIndex ()
{
   //子表分区键查询
   var findConf = { a0: { $in: [0, 10000] } };
   var expExplains = [{
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
   var expExplains = [{
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
