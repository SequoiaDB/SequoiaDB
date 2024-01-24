/******************************************************************************
 * @Description   : seqDB-28215:提交事务，事务中为事务操作，查询快照
 * @Author        : liuli
 * @CreateTime    : 2022.10.13
 * @LastEditTime  : 2022.10.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_28215";

main( test );
function test ( testPara )
{
   var groupName = testPara.srcGroupName;

   // coord节点会统计事务提交，重新连接一个coord确定使用coord NodeName
   var coordUrl = getCoordUrl( db );
   var sdb = new Sdb( coordUrl[0] );
   var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28215";
   sdb.setSessionAttr( { Source: source } );

   // 获取cl所在group主节点
   var masterNode = sdb.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   dbcl.insert( { a: 1 } );

   // 获取数据库快照非聚合结果
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = sdb.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 开启事务后读数据
   sdb.transBegin();
   dbcl.find().toArray();

   // 提交事务
   sdb.transCommit();

   // 校验数据库快照
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["NodeName"] == masterNodeName || dataBastInfos[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = sdb.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   for( var i in sessionInfos )
   {
      if( sessionInfos[i]["NodeName"] == masterNodeName || sessionInfos[i]["NodeName"] == coordUrl[0] )
      {
         sessionInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actSessionInfos, sessionInfos );

   // 开启事务后插入数据
   sdb.transBegin();
   dbcl.insert( { a: 2 } );

   // 提交事务
   sdb.transCommit();

   // 校验数据库快照
   var cursor = sdb.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   for( var i in dataBastInfos )
   {
      if( dataBastInfos[i]["NodeName"] == masterNodeName || dataBastInfos[i]["NodeName"] == coordUrl[0] )
      {
         dataBastInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = sdb.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   for( var i in sessionInfos )
   {
      if( sessionInfos[i]["NodeName"] == masterNodeName || sessionInfos[i]["NodeName"] == coordUrl[0] )
      {
         sessionInfos[i]["TotalTransCommit"] += 1;
      }
   }
   assert.equal( actSessionInfos, sessionInfos );

   sdb.close();
}
