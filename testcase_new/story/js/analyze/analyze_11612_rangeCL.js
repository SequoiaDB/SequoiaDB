/************************************
*@Description: 指定切分表收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.11
*@testlinkCase:seqDB-11612
**************************************/
main( test );
function test ()
{
   //独立模式及1组模式不执行该用例
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

   var clName = COMMCLNAME + "_11612";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 4000;
   var sameValues = 9000;

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建切分表
   var clOption = { ShardingKey: { "a": 1 }, ShardingType: "range" };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, clOption );

   //执行切分
   var groups = ClSplitOneTimes( COMMCSNAME, clName, { a: 2000 }, { a: 4000 } );

   //创建索引
   commCreateIndex( dbcl, "a0", { a0: 1 } );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "$shard", false, false );
   checkStat( db, COMMCSNAME, clName, "a0", false, false );

   //在主备节点使用shard索引字段执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //在主备节点使用普通索引字段执行查询
   var findConf = { a0: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: groups[0].GroupName },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[0].GroupName },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[1].GroupName }];
   var actAccessPlan = getSplitAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "$shard", true, true );
   checkStat( db, COMMCSNAME, clName, "a0", true, true );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getSplitAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //在主备节点使用shard索引字段执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //在主备节点使用普通索引字段执行查询
   var findConf = { a0: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "", GroupName: groups[0].GroupName },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[1].GroupName },
   { ScanType: "tbscan", IndexName: "", GroupName: groups[0].GroupName }];
   var actAccessPlan = getSplitAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();

}