/******************************************************************************
 * @Description   : seqDB-28220:自动事务提交，失败自动回滚
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.20
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28220";

main( test );
function test ( testPara )
{
   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28220";
   db.setSessionAttr( { Source: source } );

   var dbcl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   var indexName = "index_28220";

   // 获取cl所在group主节点
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 创建唯一索引并插入一条数据
   dbcl.createIndex( indexName, { a: 1 }, true );
   dbcl.insert( { a: 1 } );

   // 设置TransAutoCommit为true
   db.setSessionAttr( { TransAutoCommit: true } );

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 插入一条数据指定唯一索引冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["NodeName"] == masterNodeName )
      {
         dataBastInfos[i]["TotalTransRollback"] += 1;
      }
   }
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   for( var i in sessionInfos )
   {
      if( sessionInfos[i]["NodeName"] == masterNodeName )
      {
         sessionInfos[i]["TotalTransRollback"] += 1;
      }
   }
   assert.equal( actSessionInfos, sessionInfos );
}