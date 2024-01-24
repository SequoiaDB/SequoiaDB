/******************************************************************************
 * @Description   : seqDB-28216:回滚事务，事务中为非事务操作，查询快照
 * @Author        : liuli
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   // 会话属性设置Source，用于后续查询会话快照时匹配
   var csName = "cs_28216";
   var clName = "cl_28216";
   var source = "source_28216";
   db.setSessionAttr( { Source: source } );
   commDropCS( db, csName );

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 开启事务后回滚事务
   db.transBegin();
   db.transRollback();

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   assert.equal( actSessionInfos, sessionInfos );

   // 开启事务后执行非事务操作，回滚事务
   db.transBegin();
   db.snapshot( SDB_SNAP_SESSIONS ).toArray();
   var dbcs = db.createCS( csName );
   dbcs.createCL( clName );
   db.transRollback();

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   assert.equal( actSessionInfos, sessionInfos );

   commDropCS( db, csName );
}
