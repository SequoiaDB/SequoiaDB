/************************************
*@Description:  普通表上truncate，清空缓存功能验证
*@author:      zhaoyu
*@createdate:  2017.11.8
*@testlinkCase:seqDB-12975
**************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) ) 
   {
      return;
   }

   //判断1节点模式
   if( true == isOnlyOneNodeInGroup() )
   {
      return;
   }

   var allGroups = commGetGroups( db );
   var groups = new Array();
   for( var i = 0; i < allGroups.length; i++ ) { groups.push( allGroups[i][0].GroupName ); }

   var clName1 = COMMCLNAME + "_12975_1";
   var clName2 = COMMCLNAME + "_12975_2";
   var insertNum = 2000;
   var sameValues = 9000;

   var clFullName1 = COMMCSNAME + "." + clName1;
   var clFullName2 = COMMCSNAME + "." + clName2;

   //清理环境
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2 );

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
   var dbclPrimary1 = db1.getCS( COMMCSNAME ).getCL( clName1 );
   var dbclPrimary2 = db1.getCS( COMMCSNAME ).getCL( clName2 );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave1 = db2.getCS( COMMCSNAME ).getCL( clName1 );
   var dbclSlave2 = db2.getCS( COMMCSNAME ).getCL( clName2 );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, COMMCSNAME, clName1, "a", false, false );
   checkStat( db, COMMCSNAME, clName2, "a", false, false );

   //分别在主备节点执行查询
   var findConf = { a: sameValues };

   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, insertNum );
   query( dbclSlave2, findConf, null, null, insertNum );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans2 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName1 } );
   db.analyze( { Collection: COMMCSNAME + "." + clName2 } );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, COMMCSNAME, clName1, "a", true, true );
   checkStat( db, COMMCSNAME, clName2, "a", true, true );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );
   var expAccessPlans = [];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans, actAccessPlans2 );

   //分别在主备节点执行查询
   var findConf = { a: sameValues };

   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, insertNum );
   query( dbclSlave2, findConf, null, null, insertNum );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //truncate cl
   dbcl1.truncate();

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, COMMCSNAME, clName1, "a", false, false );
   checkStat( db, COMMCSNAME, clName2, "a", true, true );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );

   //分别在主备节点执行查询
   var findConf = { a: sameValues };

   query( dbclPrimary1, findConf, null, null, 0 );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, 0 );
   query( dbclSlave2, findConf, null, null, insertNum );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //再次插入相同数据
   insertDiffDatas( dbcl1, insertNum );
   insertSameDatas( dbcl1, insertNum, sameValues );

   //检查主备同步
   checkConsistency( db, null, null, groups );

   //检查统计信息
   checkStat( db, COMMCSNAME, clName1, "a", false, false );
   checkStat( db, COMMCSNAME, clName2, "a", true, true );

   //分别在主备节点执行查询
   var findConf = { a: sameValues };

   query( dbclPrimary1, findConf, null, null, insertNum );
   query( dbclPrimary2, findConf, null, null, insertNum );
   query( dbclSlave1, findConf, null, null, insertNum );
   query( dbclSlave2, findConf, null, null, insertNum );

   //检查主备节点访问计划快照
   var accessFindOption1 = { Collection: clFullName1 };
   var accessFindOption2 = { Collection: clFullName2 };

   var actAccessPlans1 = getCommonAccessPlans( db, accessFindOption1 );
   var actAccessPlans2 = getCommonAccessPlans( db, accessFindOption2 );

   var expAccessPlans1 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlans2 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];

   checkSnapShotAccessPlans( clFullName1, expAccessPlans1, actAccessPlans1 );
   checkSnapShotAccessPlans( clFullName2, expAccessPlans2, actAccessPlans2 );


   //清空环境
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the end" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the end" );
   db1.close();
   db2.close();

}
