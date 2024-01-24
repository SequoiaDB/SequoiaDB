/************************************
*@Description: 索引字段不存在时生成统计信息
*@author:      zhaoyu
*@createdate:  2017.11.9
*@testlinkCase:seqDB-11644
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11644";
   var clFullName = COMMCSNAME + "." + clName;
   var insertNum = 2000;
   var sameValues = 1000;

   var findConf = { d: { $exists: 0 } };
   var expAccessPlan1 = [{ ScanType: "tbscan", IndexName: "" }];
   var expAccessPlan2 = [{ ScanType: "ixscan", IndexName: "d" }];
   var expAccessPlan3 = [];

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   commCreateIndex( dbcl, "d", { d: 1 } );

   //插入记录
   insertDiffDatas( dbcl, insertNum );
   insertSameDatas( dbcl, insertNum, sameValues );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( COMMCSNAME ).getCL( clName );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "d", false, false );

   //执行查询
   query( dbclPrimary, findConf, null, null, insertNum );

   //检查访问计划快照
   var actAccessPlan = getCommonAccessPlans( db, { Collection: clFullName } );
   checkSnapShotAccessPlans( clFullName, expAccessPlan2, actAccessPlan );

   //执行统计
   db.analyze( { Collection: COMMCSNAME + "." + clName, Index: "d" } );

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   checkStat( db, COMMCSNAME, clName, "d", true, true );

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
*@Description: 插入不同记录, 数据页超过10页
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function insertDiffDatas ( dbcl, insertNum )
{
   //插入不同记录
   var doc = [];
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push(
         {
            a: i, a0: i, a1: i, a2: i, a3: i, a4: i, a5: i, a6: i, a7: i, a8: i, a9: i,
            a10: i, a11: i, a12: i, a13: i, a14: i, a15: i, a16: i, a17: i, a18: i, a19: i,
            a20: i, a21: i, a22: i, a23: i, a24: i, a25: i, a26: i, a27: i, a28: i, a29: i,
            a30: i, a31: i, a32: i, a33: i, a34: i, a35: i, a36: i, a37: i, a38: i, a39: i,
            a40: i, a41: i, a42: i, a43: i, a44: i, a45: i, a46: i, a47: i, a48: i, a49: i,
            a50: i, a51: i, a52: i, a53: i, a54: i, a55: i, a56: i, a57: i, a58: i, a59: i,
            a60: i, a61: i, a62: i, a63: i, a64: i, a65: i, a66: i, a67: i, a68: i, a69: i,
            b: i, c: "test" + i, d: i
         } );
   }
   dbcl.insert( doc );

}