/************************************
*@Description: 指定切分表收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.11
*@testlinkCase:seqDB-11612
**************************************/
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

   var csName = COMMCSNAME + "_hash_11612";
   var clName = COMMCLNAME + "_11612";
   var clFullName = csName + "." + clName;
   var insertNum = 4000;
   var sameValues = 9000;
   var domainName = "mydomain";

   //清理环境
   commDropCS( db, csName );
   commDropDomain( db, domainName );

   //获取组
   var groups = getGroupName( db );

   //创建域
   commCreateDomain( db, domainName, [groups[0][0], groups[1][0]], { AutoSplit: true } );

   //创建切分表
   csOption = { Domain: domainName }
   commCreateCS( db, csName, false, "", csOption );
   var clOption = { ShardingKey: { "a": 1 }, ShardingType: "hash" };
   var dbcl = commCreateCL( db, csName, clName, clOption );

   //获取切分表的组信息
   var groups = commGetCLGroups( db, csName + "." + clName );

   //创建索引
   commCreateIndex( dbcl, "a0", { a0: 1 } );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", false, false );
   checkStat( db, csName, clName, "a0", false, false );

   //在主备节点使用shard索引字段执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //在主备节点使用普通索引字段执行查询
   var findConf = { a0: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "$shard", GroupName: groups[0] },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[1] },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[0] }];
   var actAccessPlan = getSplitAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //执行统计
   db.analyze( { Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "$shard", true, true );
   checkStat( db, csName, clName, "a0", true, true );

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
   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "", GroupName: groups[0] },
   { ScanType: "ixscan", IndexName: "a0", GroupName: groups[1] },
   { ScanType: "tbscan", IndexName: "", GroupName: groups[0] }];

   var actAccessPlan = getSplitAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //清理环境
   commDropCS( db, csName );
   commDropDomain( db, domainName );
   db1.close();

}