﻿/************************************
*@Description: seqDB-12973:删除普通表cs，清空缓存功能功能验证
*@author:      zhaoyu
*@createdate:  2018.1.18
*@testlinkCase:seqDB-12973
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );
function test ()
{
   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; i++ ) { groups.push( allGroups[i][0].GroupName ); }

   var csName1 = COMMCSNAME + "_12973_1";
   var csName2 = COMMCSNAME + "_12973_2";
   var clName = COMMCLNAME + "_12973";
   var clFullName1 = csName1 + "." + clName;
   var clFullName2 = csName2 + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   //清理环境
   commDropCS( db, csName1, true, "drop cs before test" );
   commDropCS( db, csName2, true, "drop cs before test" );

   //创建cs、cl
   commCreateCS( db, csName1, true );
   commCreateCS( db, csName2, true );

   var groupName = groups[0];
   //创建2个cl在同一个数据组上
   var CLOption = { Group: groupName };
   var dbcl1 = commCreateCL( db, csName1, clName, CLOption );
   var dbcl2 = commCreateCL( db, csName2, clName, CLOption );

   //创建索引
   commCreateIndex( dbcl1, "a", { a: 1 } );
   commCreateIndex( dbcl2, "a", { a: 1 } );

   //插入记录
   insertDiffDatas( dbcl1, insertNum );
   insertSameDatas( dbcl1, insertNum, sameValues );

   insertDiffDatas( dbcl2, insertNum );
   insertSameDatas( dbcl2, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary1 = db1.getCS( csName1 ).getCL( clName );
   var dbclPrimary2 = db1.getCS( csName2 ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave1 = db2.getCS( csName1 ).getCL( clName );
   var dbclSlave2 = db2.getCS( csName2 ).getCL( clName );

   //执行统计
   db.analyze( { CollectionSpace: csName1 } );
   db.analyze( { CollectionSpace: csName2 } );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName1, clName, "a", true, true );
   checkStat( db, csName2, clName, "a", true, true );

   //cl1、cl2中执行查询
   findConf = { a: sameValues };
   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclSlave2, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );

   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );

   //drop cs
   commDropCS( db, csName1 );

   var groups = new Array();
   groups.push( groupName );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName1, clName, "a", false, false, groups );
   checkStat( db, csName2, clName, "a", true, true );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );

   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );

   //再次创建cs、cl、创建索引、插入相同数据
   commCreateCS( db, csName1 );
   var dbcl1 = commCreateCL( db, csName1, clName, CLOption );
   commCreateIndex( dbcl1, "a", { a: 1 } );

   insertDiffDatas( dbcl1, insertNum );
   insertSameDatas( dbcl1, insertNum, sameValues );

   //获取主备节点
   var dbclPrimary1 = db1.getCS( csName1 ).getCL( clName );
   var dbclSlave1 = db2.getCS( csName1 ).getCL( clName );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName1, clName, "a", false, false );
   checkStat( db, csName2, clName, "a", true, true );

   //cl1中执行查询
   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, insertNum );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );

   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );

   //清空环境
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   db1.close();
   db2.close();
}
