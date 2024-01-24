/************************************
*@Description: 64个索引执行统计
*@author:      zhaoyu
*@createdate:  2017.11.9
*@testlinkCase:seqDB-11406
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11406";
   var insertNum = 2000;
   var sameValues = 9000;
   var indexNum = 63;

   //清理环境
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //创建cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //创建索引
   for( var i = 0; i < indexNum; i++ )
   {
      var obj = {};
      obj["a" + i] = 1;
      commCreateIndex( dbcl, "a" + i, obj );
   }

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
   for( var i = 0; i < indexNum; i++ )
   {
      checkStat( db, COMMCSNAME, clName, "a" + i, true, true );
   }

   //检查主备节点访问计划
   for( var i = 0; i < indexNum; i++ )
   {
      var findConf = {};
      findConf["a" + i] = sameValues;
      var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNum }];

      var actExplains = getCommonExplain( dbclPrimary, findConf );
      checkExplain( actExplains, expExplains );
   }

   //删除索引
   for( var i = 0; i < indexNum; i++ )
   {
      commDropIndex( dbcl, "a" + i );
   }

   //检查统计信息
   checkConsistency( db, COMMCSNAME, clName );
   for( var i = 0; i < indexNum; i++ )
   {
      checkStat( db, COMMCSNAME, clName, "a" + i, true, false );
   }

   //检查主备节点访问计划
   for( var i = 0; i < indexNum; i++ )
   {
      var findConf = {};
      findConf["a" + i] = sameValues;
      var expExplains = [{ ScanType: "tbscan", IndexName: "", ReturnNum: insertNum }];

      var actExplains = getCommonExplain( dbclPrimary, findConf );
      checkExplain( actExplains, expExplains );
   }

   //清理环境
   commDropCL( db, COMMCSNAME, clName );
   db1.close();
}
