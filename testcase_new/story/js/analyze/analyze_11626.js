/************************************
*@Description: 指定普通索引生成默认统计信息并手工修改统计信息再清空
*@author:      zhaoyu
*@createdate:  2017.11.13
*@testlinkCase:seqDB-11626
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11626";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   var findConf1 = { a: sameValues };
   var findConf2 = { a1: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   commCreateIndex( dbcl, "a", { a: 1 } );
   commCreateIndex( dbcl, "a1", { a1: 1 } );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName } );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );
   checkStat( db, COMMCSNAME, clName, "a1", true, true );

   //执行查询
   query( dbclPrimary, findConf1, null, null, insertNum );
   query( dbclPrimary, findConf2, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //生成默认统计信息
   db.analyze( { Mode: 3, Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );
   checkStat( db, COMMCSNAME, clName, "a1", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf1, null, null, insertNum );
   query( dbclPrimary, findConf2, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主节点统计信息
   var mcvValues = [{ a: 8000 }, { a: 9000 }, { a: 9001 }];
   var fracs = [50, 5000, 50];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );
   checkStat( db, COMMCSNAME, clName, "a1", true, true );

   //检查访问计划快照信息
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //统计信息加载至缓存
   db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );
   checkStat( db, COMMCSNAME, clName, "a1", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf1, null, null, insertNum );
   query( dbclPrimary, findConf2, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //再次更新统计信息
   var mcvValues = [{ a: 8000 }, { a: 9000 }, { a: 9001 }];
   var fracs = [50, 50, 50];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //再次清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );
   checkStat( db, COMMCSNAME, clName, "a1", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf1, null, null, insertNum );
   query( dbclPrimary, findConf2, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();
}
