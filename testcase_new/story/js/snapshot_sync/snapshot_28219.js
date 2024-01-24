/******************************************************************************
 * @Description   : seqDB-28219:事务中查询快照
 * @Author        : liuli
 * @CreateTime    : 2022.10.12
 * @LastEditTime  : 2022.10.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28219";

main( test );
function test ( testPara )
{
   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28219";
   db.setSessionAttr( { Source: source } );

   var dbcl = testPara.testCL;
   var groupName = testPara.srcGroupName;
   var indexName = "index_28219";

   // 获取cl所在group主节点
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 创建唯一索引并插入一条数据
   dbcl.createIndex( indexName, { a: 1 }, true );
   dbcl.insert( { a: 1 } );

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 开启事务后插入一条冲突数据
   db.transBegin();
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

   // 开启事务后先插入数据成功，再插入数据失败
   db.transBegin();
   dbcl.insert( { a: 2 } );
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
