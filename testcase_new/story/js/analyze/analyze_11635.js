/************************************
*@Description: 将统计信息修改为非法值后加载至缓存
*@author:      zhaoyu
*@createdate:  2017.11.14
*@testlinkCase:seqDB-11635
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11635";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;
   var sameValues = 9000;

   var findConf = { a: sameValues };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "a" }];
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

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan1, actAccessPlan );

   //生成默认统计信息
   db.analyze( { Mode: 3, Collection: COMMCSNAME + "." + clName } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, false );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan3, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主节点统计信息
   var mcvValues = [{ a: 8000 }, { a: 9000 }, { a: 9001 }];
   var fracs = [50, 5000, 50];
   var rule = { "$set": { "KeyPattern": { a: -1 }, "MCV": { "Values": mcvValues, "Frac": fracs } } };
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", rule );

   //统计信息加载至缓存
   try
   {
      db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName } );
      throw new Error( "NEED_ERR" );
   }
   catch( e )
   {
      if( e.message != SDB_COORD_NOT_ALL_DONE && e.message != SDB_IXM_NOTEXIST )
      {
         throw e;
      }
   }

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //手工修改主节点统计信息
   var mcvValues = [{ a: 8000 }, { a: 9000 }, { a: 9001 }];
   var fracs = [50, 5000, 50];
   var rule = { "$set": { "KeyPattern": { a: 1, b: -1 }, "MCV": { "Values": mcvValues, "Frac": fracs } } };
   updateIndexStateInfo( db, COMMCSNAME, clName, "a", rule );

   //统计信息加载至缓存
   try
   {
      db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + clName } );
      throw new Error( "NEED_ERR" );
   }
   catch( e )
   {
      if( e.message != SDB_COORD_NOT_ALL_DONE && e.message != SDB_IXM_NOTEXIST )
      {
         throw e;
      }
   }

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "a", true, true );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //再次执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName } );

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

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
   db1.close();

}

/************************************
*@Description: 手工修改索引统计表
*@author:      liuxiaoxuan
*@createDate:  2017.11.13
**************************************/
function updateIndexStateInfo ( db, csName, clName, indexName, rule )
{
   var dataDB = new Array();
   if( commIsStandalone( db ) )
   {
      dataDB[0] = db;
   }
   else
   {
      var groupNames = commGetCLGroups( db, csName + "." + clName );
      var groupDetail = commGetGroups( db, false, groupNames[0] );
      for( var j = 1; j < groupDetail[0].length; j++ )
      {
         var hostName = groupDetail[0][j].HostName;
         var svcName = groupDetail[0][j].svcname;
         dataDB[j - 1] = new Sdb( hostName, svcName );
      }
   }

   for( var i in dataDB )
   {
      var rec = dataDB[i].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find().toArray();

      if( 0 < rec.length )
      {
         var matcher = {
            "$and": [{ "CollectionSpace": csName },
            { "Collection": clName },
            { "Index": indexName }]
         };

         dataDB[i].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).upsert( rule, matcher );
      }
   }
}
