/******************************************************************************
 * @Description   : seqDB-28221:catalog上事务提交与回滚
 * @Author        : liuli
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ()
{
   var csName = "cs_28221";
   var clName = "cl_28221";

   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28221";
   db.setSessionAttr( { Source: source } );
   commDropCS( db, csName );

   // 获取数据库快照非聚合结果，指定catalog节点
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, Role: "catalog" }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source, Role: "catalog" }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 执行创建CS，CL
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName );
   // 写数据
   dbcl.insert( { a: 1 } );

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, Role: "catalog" }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["TotalTransCommit"] > actDataBastInfos[i]["TotalTransCommit"] )
      {
         throw new Error( "actual:" + actDataBastInfos[i]["TotalTransCommit"] + "\nexpected:" + dataBastInfos[i]["TotalTransCommit"] +
            "\nNodeName:" + dataBastInfos[i]["NodeName"] + "\nthe expected result is less than the actual result" );
      }
      if( dataBastInfos[i]["TotalTransRollback"] > actDataBastInfos[i]["TotalTransRollback"] )
      {
         throw new Error( "actual:" + actDataBastInfos[i]["TotalTransRollback"] + "\nexpected:" + dataBastInfos[i]["TotalTransRollback"] +
            "\nNodeName:" + dataBastInfos[i]["NodeName"] + "\nthe expected result is less than the actual result" );
      }
   }

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source, Role: "catalog" }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   for( var i in sessionInfos )
   {
      if( sessionInfos[i]["TotalTransCommit"] > actSessionInfos[i]["TotalTransCommit"] )
      {
         throw new Error( "actual:" + actSessionInfos[i]["TotalTransCommit"] + "\nexpected:" + sessionInfos[i]["TotalTransCommit"] +
            "\nNodeName:" + sessionInfos[i]["NodeName"] + "\nthe expected result is less than the actual result" );
      }
      if( sessionInfos[i]["TotalTransRollback"] > actSessionInfos[i]["TotalTransRollback"] )
      {
         throw new Error( "actual:" + actSessionInfos[i]["TotalTransRollback"] + "\nexpected:" + sessionInfos[i]["TotalTransRollback"] +
            "\nNodeName:" + sessionInfos[i]["NodeName"] + "\nthe expected result is less than the actual result" );
      }
   }

   commDropCS( db, csName );
}
