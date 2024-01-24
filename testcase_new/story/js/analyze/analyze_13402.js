/************************************
*@Description: 指定固定集合空间/固定集合收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.16
*@testlinkCase:seqDB-13402
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "_13402";
   var clName = COMMCLNAME + "_13402";
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
   csOptions = { Capped: true };
   commCreateCS( db, csName, false, "create capped cs", csOptions );

   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, clOption );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", false, false );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定cs执行统计
   db.analyze( { CollectionSpace: csName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定cs执行统计
   db.analyze( { Mode: 2, CollectionSpace: csName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //生成默认统计信息
   db.analyze( { Mode: 3, Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //执行统计
   db.analyze( { Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //生成默认统计信息
   db.analyze( { Mode: 3, Collection: csName + "." + clName } );

   //检查统计信息
   checkConsistency( db, csName, clName );
   checkStat( db, csName, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //手工修改主节点统计信息
   var mcvValues = [{ a: 8000 }, { a: 9000 }, { a: 9001 }];
   var fracs = [500, 9000, 500];
   updateIndexStateInfo( db, csName, clName, "a", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { Mode: 4, Collection: csName + "." + clName } );

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

   //清空统计信息
   db.analyze( { Mode: 5, Collection: csName + "." + clName } );

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