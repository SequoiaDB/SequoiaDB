/******************************************************************************
 * @Description   : seqDB-28218:自动事务提交，查询快照统计事务提交次数
 * @Author        : liuli
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28218";

main( test );
function test ( testPara )
{
   var groupName = testPara.srcGroupName;

   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28218";
   db.setSessionAttr( { Source: source } );
   var dbcl = testPara.testCL;

   // 开启自动事务提交
   db.setSessionAttr( { TransAutoCommit: true } );

   // 获取cl所在group主节点
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 写数据
   dbcl.insert( { a: 1 } );

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["NodeName"] == masterNodeName )
      {
         dataBastInfos[i]["TotalTransCommit"] += 1;
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
         sessionInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actSessionInfos, sessionInfos );

   // 读数据
   dbcl.find().toArray();

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["NodeName"] == masterNodeName )
      {
         dataBastInfos[i]["TotalTransCommit"] += 1;
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
         sessionInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actSessionInfos, sessionInfos );
}
