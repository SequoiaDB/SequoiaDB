/************************************
*@Description: seqDB-11732:数据有变化时执行统计
*@author:      zhaoyu
*@createdate:  2017.11.15
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "_11732";
   var clName = COMMCLNAME + "_11732";
   var clFullName = csName + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   var findConf = { a: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];

   //清理环境
   commDropCS( db, csName, true, "drop cs before test" );

   //创建cl
   var dbcl = commCreateCL( db, csName, clName );

   //创建索引
   commCreateIndex( dbcl, "a", { a: 1 } );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //执行统计
   db.analyze( { Collection: csName + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //执行查询
   query( dbclPrimary, findConf, null, null, 0 );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //执行统计
   db.analyze( { Collection: csName + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清空数据
   dbcl.truncate();

   //执行统计
   db.analyze( { Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, 0 );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //执行统计
   db.analyze( { Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清空数据
   dbcl.truncate();

   //执行统计
   db.analyze( { CollectionSpace: csName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, 0 );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //执行统计
   db.analyze( { CollectionSpace: csName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清理环境
   commDropCS( db, csName );
   db1.close();

}
