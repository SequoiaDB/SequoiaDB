/************************************
*@Description: 将所有统计信息加载至缓存再清空 
*@author:      zhaoyu
*@createdate:  2017.11.13
*@testlinkCase:seqDB-11630
**************************************/
main( test );
function test ()
{
   //get all groups
   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; ++i ) { groups.push( allGroups[i][0].GroupName ); }

   var csName1 = COMMCSNAME + "_11630_1";
   var clName1 = COMMCLNAME + "_11630_1";
   var csName2 = COMMCSNAME + "_11630_2";
   var clName2 = COMMCLNAME + "_11630_2";
   var insertNum = 2000;
   var sameValues = 9000;

   var clFullName1 = csName1 + "." + clName1;
   var clFullName2 = csName1 + "." + clName2;
   var clFullName3 = csName2 + "." + clName1;
   var clFullName4 = csName2 + "." + clName2;

   var findConf = { a: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];
   //清理环境
   commDropCS( db, csName1, true, "drop cs before test" );
   commDropCS( db, csName2, true, "drop cs before test" );

   //创建cl
   commCreateCS( db, csName1, true );
   var dbcl11 = commCreateCL( db, csName1, clName1 );
   var dbcl12 = commCreateCL( db, csName1, clName2 );

   commCreateCS( db, csName2, true );
   var dbcl21 = commCreateCL( db, csName2, clName1 );
   var dbcl22 = commCreateCL( db, csName2, clName2 );

   //创建索引
   commCreateIndex( dbcl11, "a", { a: 1 } );
   commCreateIndex( dbcl12, "a", { a: 1 } );
   commCreateIndex( dbcl21, "a", { a: 1 } );
   commCreateIndex( dbcl22, "a", { a: 1 } );

   //插入记录
   insertDiffDatas( dbcl11, insertNum );
   insertSameDatas( dbcl11, insertNum, sameValues );

   insertDiffDatas( dbcl12, insertNum );
   insertSameDatas( dbcl12, insertNum, sameValues );

   insertDiffDatas( dbcl21, insertNum );
   insertSameDatas( dbcl21, insertNum, sameValues );

   insertDiffDatas( dbcl22, insertNum );
   insertSameDatas( dbcl22, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary11 = db1.getCS( csName1 ).getCL( clName1 );
   var dbclPrimary12 = db1.getCS( csName1 ).getCL( clName2 );
   var dbclPrimary21 = db1.getCS( csName2 ).getCL( clName1 );
   var dbclPrimary22 = db1.getCS( csName2 ).getCL( clName2 );

   //执行统计
   db.analyze( );

   //检查所有组主备是否一致
   checkConsistency( db, null, null, groups );
   //检查统计信息
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName2, clName2, "a", true, true );

   //执行查询
   query( dbclPrimary11, findConf, null, null, insertNum );
   query( dbclPrimary12, findConf, null, null, insertNum );
   query( dbclPrimary21, findConf, null, null, insertNum );
   query( dbclPrimary22, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan1, actAccessPlan );

   //生成默认统计信息
   db.analyze( { Mode: 3, Collection: clFullName1 } );

   //检查所有组主备是否一致
   checkConsistency( db, null, null, groups );
   //检查统计信息
   checkStat( db, csName1, clName1, "a", true, false );
   checkStat( db, csName1, clName2, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName2, clName2, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan1, actAccessPlan );

   //执行查询
   query( dbclPrimary11, findConf, null, null, insertNum );
   query( dbclPrimary12, findConf, null, null, insertNum );
   query( dbclPrimary21, findConf, null, null, insertNum );
   query( dbclPrimary22, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan2, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan1, actAccessPlan );

   //手工修改主备节点统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 9000, 500];
   updateIndexStateInfo( db, csName1, clName1, "a", mcvValues, fracs );

   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 100, 9400];
   updateIndexStateInfo( db, csName2, clName2, "a", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { Mode: 4 } );

   //检查所有组主备是否一致
   checkConsistency( db, null, null, groups );
   //检查统计信息
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName2, clName2, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary11, findConf, null, null, insertNum );
   query( dbclPrimary12, findConf, null, null, insertNum );
   query( dbclPrimary21, findConf, null, null, insertNum );
   query( dbclPrimary22, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan2, actAccessPlan );

   //清空统计信息
   db.analyze( { Mode: 5 } );

   //检查所有组主备是否一致
   checkConsistency( db, null, null, groups );
   //检查统计信息
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "a", true, true );
   checkStat( db, csName2, clName1, "a", true, true );
   checkStat( db, csName2, clName2, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan3, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary11, findConf, null, null, insertNum );
   query( dbclPrimary12, findConf, null, null, insertNum );
   query( dbclPrimary21, findConf, null, null, insertNum );
   query( dbclPrimary22, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan1, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan2, actAccessPlan );

   //清理环境
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   db1.close();
}
