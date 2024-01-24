/************************************
*@Description: seqDB-12974:删除普通表cl，清空缓存功能验证
*@author:      zhaoyu
*@createdate:  2018.1.18
*@testlinkCase:seqDB-12974
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.csName = COMMCSNAME + "_12974";
main( test );
function test ()
{

   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; i++ ) { groups.push( allGroups[i][0].GroupName ); }

   var csName = COMMCSNAME + "_12974";
   var clName1 = COMMCLNAME + "_12974_1";
   var clName2 = COMMCLNAME + "_12974_2";
   var clFullName1 = csName + "." + clName1;
   var clFullName2 = csName + "." + clName2;
   var insertNum = 2000;
   var sameValues = 9000;

   var groupName = groups[0];
   //创建2个cl在同一个数据组上
   var CLOption = { Group: groupName };
   var dbcl1 = commCreateCL( db, csName, clName1, CLOption );
   var dbcl2 = commCreateCL( db, csName, clName2, CLOption );

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
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( csName ).getCL( clName2 );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave1 = db2.getCS( csName ).getCL( clName1 );
   var dbclSlave2 = db2.getCS( csName ).getCL( clName2 );

   //执行统计
   db.analyze( { CollectionSpace: csName } );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName, clName1, "a", true, true );
   checkStat( db, csName, clName2, "a", true, true );

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

   //drop cl
   commDropCL( db, csName, clName1 );

   var groups = new Array();
   groups.push( groupName );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName, clName1, "a", false, false, groups );
   checkStat( db, csName, clName2, "a", true, true );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName1 } );
   checkSnapShotAccessPlans( clFullName1, expAccessPlan, actAccessPlan );

   var expAccessPlan = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName2 } );
   checkSnapShotAccessPlans( clFullName2, expAccessPlan, actAccessPlan );

   //再次创建cl、创建索引、插入相同数据
   var dbcl1 = commCreateCL( db, csName, clName1, CLOption );
   commCreateIndex( dbcl1, "a", { a: 1 } );

   insertDiffDatas( dbcl1, insertNum );
   insertSameDatas( dbcl1, insertNum, sameValues );

   //获取主备节点
   var dbclPrimary1 = db1.getCS( csName ).getCL( clName1 );
   var dbclSlave1 = db2.getCS( csName ).getCL( clName1 );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, csName, clName1, "a", false, false );
   checkStat( db, csName, clName2, "a", true, true );

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

   db1.close();
   db2.close();
}
