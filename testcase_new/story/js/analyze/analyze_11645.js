/************************************
*@Description: 索引字段值为不同的数据类型，生成统计信息
*@author:      zhaoyu
*@createdate:  2017.11.15
*@testlinkCase:seqDB-11645
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11645";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;

   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];

   //字符串
   var sameValues = "a";

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   commCreateIndex( dbcl, "a", { a: 1 } );

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清除数据
   dbcl.truncate();

   //bool
   var sameValues = true;

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清除数据
   dbcl.truncate();

   //timestamp
   var sameValues = { "$timestamp": "2012-01-01-13.14.26.124233" };

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清除数据
   dbcl.truncate();

   //date
   var sameValues = { "$date": "2012-01-01" };

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清除数据
   dbcl.truncate();

   //oid
   var sameValues = { "$oid": "123abcd00ef12358902300ef" };

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //清除数据
   dbcl.truncate();

   //其他类型
   var insertNum = 500;
   var sameValues1 = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" };
   var sameValues2 = { "$regex": "^a", "$options": "i" };
   var sameValues3 = { "subobj": "value" };
   var sameValues4 = { "$minKey": 1 };
   var sameValues5 = { "$maxKey": 1 };

   //插入记录
   insertSameDatas( dbcl, insertNum, sameValues1 );
   insertSameDatas( dbcl, insertNum, sameValues2 );
   insertSameDatas( dbcl, insertNum, sameValues3 );
   insertSameDatas( dbcl, insertNum, sameValues4 );
   insertSameDatas( dbcl, insertNum, sameValues5 );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   var findConf = { a: sameValues1 };
   query( dbclPrimary, findConf, null, null, insertNum );

   var findConf = { a: { $et: sameValues2 } };
   query( dbclPrimary, findConf, null, null, insertNum );

   var findConf = { a: sameValues3 };
   query( dbclPrimary, findConf, null, null, insertNum );

   var findConf = { a: sameValues4 };
   query( dbclPrimary, findConf, null, null, insertNum );

   var findConf = { a: sameValues5 };
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }]
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();
}