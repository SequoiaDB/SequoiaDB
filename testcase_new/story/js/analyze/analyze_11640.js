/************************************
*@Description: 指定3个参数组合生成默认统计信息并手工修改再清空
*@author:      zhaoyu
*@createdate:  2017.11.15
*@testlinkCase:seqDB-11640
**************************************/
main( test );
function test ()
{
   //独立模式及1组模式不执行该用例
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

   var clName = COMMCLNAME + "_11640";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   var findConf = { a: sameValues };
   var findConf = { a: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan3 = [];
   var expAccessPlan4 = [{ ScanType: "tbscan", IndexName: "" },
   { ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan5 = [{ ScanType: "ixscan", IndexName: "a" }];
   var expAccessPlan6 = [{ ScanType: "tbscan", IndexName: "" }];

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
   db1.setSessionAttr( { PreferedInstance: "m", PreferedPeriod: -1 } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s", PreferedPeriod: -1 } );
   var dbclSlave = db2.getCS( COMMCSNAME ).getCL( clName );

   //收集统计信息
   db.analyze( { Collection: COMMCSNAME + "." + clName } );

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

   //指定cl + index + group生成默认统计信息
   var groupName = getSrcGroup( COMMCSNAME, clName );
   db.analyze( { Mode: 3, Collection: COMMCSNAME + "." + clName, Index: "a", GroupName: groupName } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主备节点统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 9000, 500];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName, Index: "a", GroupName: groupName } );

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

   //清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, Index: "a", GroupName: groupName } );

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

   //再次更新统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 100, 9400];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

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

   //再次清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, Index: "a", GroupName: groupName } );

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
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //指定cl + index + node生成默认统计信息
   var primaryNode = db.getRG( groupName ).getMaster();
   var nodeId = parseInt( primaryNode.getNodeDetail().split( ":" )[0] );
   db.analyze( { Mode: 3, Collection: COMMCSNAME + "." + clName, Index: "a", NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主备节点统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 9000, 500];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName, Index: "a", NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, Index: "a", NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //再次更新统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 100, 9400];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //再次清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, Index: "a", NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //指定cl + group + node生成默认统计信息
   var groupName = getSrcGroup( COMMCSNAME, clName );
   db.analyze( { Mode: 3, Collection: COMMCSNAME + "." + clName, GroupName: groupName, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主备节点统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 9000, 500];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //统计信息加载至缓存
   db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName, GroupName: groupName, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, GroupName: groupName, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //再次更新统计信息
   var mcvValues = [{ a: 8000 }, { a: sameValues }, { a: 9001 }];
   var fracs = [500, 100, 9400];
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", mcvValues, fracs );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan4, actAccessPlan );

   //再次清空缓存
   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + clName, GroupName: groupName, NodeID: nodeId } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan5, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );
   query( dbclSlave, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //指定不支持的组合参数执行统计
   analyzeInvalidPara( db, { mode: 3, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, Index: "a" } );
   analyzeInvalidPara( db, { mode: 4, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, Index: "a" } );
   analyzeInvalidPara( db, { mode: 5, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, Index: "a" } );
   analyzeInvalidPara( db, { mode: 3, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 4, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 5, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 3, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 4, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 5, CollectionSpace: COMMCSNAME, Collection: COMMCSNAME + "." + clName, NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 3, CollectionSpace: COMMCSNAME, Index: "a", GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 4, CollectionSpace: COMMCSNAME, Index: "a", GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 5, CollectionSpace: COMMCSNAME, Index: "a", GroupName: groupName } );
   analyzeInvalidPara( db, { mode: 3, CollectionSpace: COMMCSNAME, Index: "a", NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 4, CollectionSpace: COMMCSNAME, Index: "a", NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 5, CollectionSpace: COMMCSNAME, Index: "a", NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 3, Index: "a", GroupName: groupName, NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 4, Index: "a", GroupName: groupName, NodeID: nodeId } );
   analyzeInvalidPara( db, { mode: 5, Index: "a", GroupName: groupName, NodeID: nodeId } );

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();
   db2.close();

}

function analyzeInvalidPara ( db, options )
{
   try
   {
      db.analyze( options );
      throw new Error( "NEED_ERR" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}
