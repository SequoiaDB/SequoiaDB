/************************************
*@Description: seqDB-11608:指定普通表所在cs收集统计信息, mode:1
*@author:      zhaoyu
*@createdate:  2017.11.8
*@testlinkCase:seqDB-11608
**************************************/
main( test );
function test ()
{
   var csName1 = COMMCSNAME + "_11608_1";
   var csName2 = COMMCSNAME + "_11608_2";

   var clName1 = COMMCLNAME + "_11608_1";
   var clName2 = COMMCLNAME + "_11608_2";
   var clName3 = COMMCLNAME + "_11608_3";

   var clFullName1 = csName1 + "." + clName1;
   var clFullName2 = csName1 + "." + clName2;
   var clFullName3 = csName1 + "." + clName3;
   var clFullName4 = csName2 + "." + clName1;

   var insertNum = 2000;
   var sameValues = 9000;

   //清理环境
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );

   //创建cs
   commCreateCS( db, csName1 );
   commCreateCS( db, csName2 );

   //创建cl
   var dbcl1 = commCreateCL( db, csName1, clName1 );
   var dbcl2 = commCreateCL( db, csName1, clName2 );
   var dbcl3 = commCreateCL( db, csName1, clName3 );

   var dbcl4 = commCreateCL( db, csName2, clName1 );

   //创建索引
   commCreateIndex( dbcl1, "a", { a: 1 } );
   commCreateIndex( dbcl2, "a", { a: 1 } );
   commCreateIndex( dbcl3, "a", { a: 1 } );

   commCreateIndex( dbcl4, "a", { a: 1 } );

   //插入记录
   insertDiffDatas( dbcl1, insertNum );
   insertSameDatas( dbcl1, insertNum, sameValues );
   insertDiffDatas( dbcl2, insertNum );
   insertSameDatas( dbcl2, insertNum, sameValues );
   insertDiffDatas( dbcl3, insertNum );
   insertSameDatas( dbcl3, insertNum, sameValues );

   insertDiffDatas( dbcl4, insertNum );
   insertSameDatas( dbcl4, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName1 ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName1 ).getCL( clName2 );
   var dbclPrimary3 = db1.getCS( csName1 ).getCL( clName3 );
   var dbclPrimary4 = db1.getCS( csName2 ).getCL( clName1 );

   //检查统计信息
   checkConsistency( db, csName1, clName1 );
   checkConsistency( db, csName1, clName2 );
   checkConsistency( db, csName1, clName3 );
   checkConsistency( db, csName2, clName1 );
   checkStat( db, csName1, clName1, "a", false, false );
   checkStat( db, csName1, clName2, "a", false, false );
   checkStat( db, csName1, clName3, "a", false, false );
   checkStat( db, csName2, clName1, "a", false, false );

   //主备节点执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclPrimary3, findConf, null, null, insertNum );
   query( dbclPrimary4, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "a" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan, actAccessPlan );

   //执行统计
   db.analyze( { CollectionSpace: csName1 } );

   //检查统计信息
   checkConsistency( db, csName1, clName1 );
   checkConsistency( db, csName1, clName2 );
   checkConsistency( db, csName1, clName3 );
   checkConsistency( db, csName2, clName1 );
   checkStat( db, csName1, clName1, "a", true, true );
   checkStat( db, csName1, clName2, "a", true, true );
   checkStat( db, csName1, clName3, "a", true, true );
   checkStat( db, csName2, clName1, "a", false, false );

   //主备节点执行查询
   var findConf = { a: sameValues };
   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclPrimary3, findConf, null, null, insertNum );
   query( dbclPrimary4, findConf, null, null, insertNum );

   //检查主备节点访问计划
   var findConf = { a: sameValues };
   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName3 } );
   checkSnapShotAccessPlans( clFullName3, expAccessPlan, actAccessPlan );

   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "a" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName4 } );
   checkSnapShotAccessPlans( clFullName4, expAccessPlan, actAccessPlan );

   //清空环境
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   db1.close();

}
