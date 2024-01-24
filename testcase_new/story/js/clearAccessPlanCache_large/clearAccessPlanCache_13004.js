/************************************
*@Description: 主子表上创建删除索引，清空缓存功能验证
*@author:      zhaoyu
*@createdate:  2018.1.26
*@testlinkCase:seqDB-13004
**************************************/
var maincsName = COMMCSNAME + "_maincs_13004";
var subcsName1 = COMMCSNAME + "_subcs_13004_1";
var mainclName = COMMCLNAME + "_maincl_13004";
var subclName1 = COMMCLNAME + "_subcl_13004_1";
var subclName2 = COMMCLNAME + "_subcl_13004_2";
var subclName3 = COMMCLNAME + "_subcl_13004_3";
var subclName4 = COMMCLNAME + "_subcl_13004_4";
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
var db2;
var dbclPrimary;
var dbclSlave;


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

   //判断1节点模式
   if( true == isOnlyOneNodeInGroup() )
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

   //插入记录
   insertDiffDatas( maincl, insertDiffNum );
   insertSameDatas( maincl, insertSameNum, 0 );
   insertSameDatas( maincl, insertSameNum, 10000 );

   //获取主备节点
   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   dbclPrimary = db1.getCS( maincsName ).getCL( mainclName );
   db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   dbclSlave = db2.getCS( maincsName ).getCL( mainclName );

   //指定主表cl执行统计
   db.analyze( { Collection: mainclFullName } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查统计
   checkStat( db, maincsName, subclName1, "$shard", true, true );
   checkStat( db, maincsName, subclName2, "$shard", true, true );
   checkStat( db, subcsName1, subclName3, "$shard", true, true );
   checkStat( db, subcsName1, subclName4, "$shard", true, true );

   //执行查询
   var findConf = { a: 1, a1: 1 };
   query( dbclPrimary, findConf, null, null, 1 );
   query( dbclSlave, findConf, null, null, 1 );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //创建索引
   commCreateIndex( maincl, "a1", { a1: 1 } );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a: 1, a1: 1 };
   query( dbclPrimary, findConf, null, null, 1 );
   query( dbclSlave, findConf, null, null, 1 );

   //检查访问计划快照
   var tmp = [{ GroupName: desGroupName, ScanType: "ixscan", IndexName: "a1" },
   { GroupName: srcGroupName, ScanType: "ixscan", IndexName: "a1" }];
   var expAccessPlan = tmp.concat( tmp );
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //删除索引
   commDropIndex( maincl, "a1" );

   //检查主备同步
   checkConsistency( db, null, null, [srcGroupName, desGroupName] );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //执行查询
   var findConf = { a: 1, a1: 1 };
   query( dbclPrimary, findConf, null, null, 1 );
   query( dbclSlave, findConf, null, null, 1 );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getMainclAccessPlans( db, { Collection: mainclFullName } );
   checkMainclAccessPlans( expAccessPlan, actAccessPlan );

   //清理环境
   commDropCS( db, subcsName1 );
   commDropCS( db, maincsName );
   db1.close();
   db2.close();

}
