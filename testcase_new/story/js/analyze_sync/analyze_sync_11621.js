/************************************
*@Description: 指定node收集统计信息
*@author:      zhaoyu
*@createdate:  2017.11.13
*@testlinkCase:seqDB-11621
**************************************/
main( test );
function test ()
{
   //判断独立模式
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //判断1节点模式
   if( true == isOnlyOneNodeInGroup() )
   {
      return;
   }

   var clName = COMMCLNAME + "_11621";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   var findConf = { a: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   commCreateIndex( dbcl, "a", { a: 1 } );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", false, false );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //指定主节点执行统计
   var groupName = getSrcGroup( COMMCSNAME, clName );
   var primaryNode = db.getRG( groupName ).getMaster();
   var nodeId = parseInt( primaryNode.getNodeDetail().split( ":" )[0] );
   db.analyze( { NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定主节点执行统计, Mode:2
   db.analyze( { Mode: 2, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定备节点执行统计
   var slaveNode = db.getRG( groupName ).getSlave();
   var nodeId = parseInt( slaveNode.getNodeDetail().split( ":" )[0] );
   try
   {
      db.analyze( { NodeID: nodeId } );
      throw new Error( "NEED_AN_ERR" );
   } catch( e )
   {
      if( e.message != SDB_CLS_NOT_PRIMARY )
      {
         throw e;
      }
   }

   //指定cata节点执行统计
   var cataNode = db.getRG( "SYSCatalogGroup" ).getMaster();
   var nodeId = parseInt( cataNode.getNodeDetail().split( ":" )[0] );
   db.analyze( { NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定cata节点执行统计,Mode:2
   db.analyze( { Mode: 2, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //指定不存在节点执行统计
   try
   {
      db.analyze( { NodeID: 2233 } );
      throw new Error( "NEED_AN_ERR" );
   } catch( e )
   {
      if( e.message != SDB_CLS_NODE_NOT_EXIST )
      {
         throw e;
      }
   }

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();
   db2.close();
}
