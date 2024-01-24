/************************************
*@Description: 指定唯一索引收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.9
*@testlinkCase:seqDB-11615
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11615";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 4000;

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   commCreateIndex( dbcl, "a", { a: 1 }, { Unique: true }, true );

   //插入记录
   insertDiffDatas( dbcl, insertNum );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //主备节点执行查询
   var findConf = { a: 1000 };
   query( dbclPrimary, findConf, null, null, 1 );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "a" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "a" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var expAccessPlan = [];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //检查主备节点访问计划
   var findConf = { a: 1000 };
   query( dbclPrimary, findConf, null, null, 1 );

   //检查访问计划快照
   var expAccessPlan = [{ ScanType: "ixscan", IndexName: "a" }];
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan, actAccessPlan );

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();

}
