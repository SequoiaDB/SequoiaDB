/******************************************************************************
 * @Description   : seqDB-28214:提交事务，事务中为非事务操作，查询快照
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.20
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_28214";
   var clName = "cl_28214";

   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28214";
   db.setSessionAttr( { Source: source } );

   commDropCS( db, csName );

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 开启事务
   db.transBegin();

   // 提交事务
   db.transCommit();

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   assert.equal( actSessionInfos, sessionInfos );

   // 开启事务
   db.transBegin();

   // 执行非事务操作，创建CL
   var cs = db.createCS( csName );
   cs.createCL( clName );
   db.snapshot( SDB_SNAP_DATABASE ).toArray();

   //提交事务
   db.transCommit();

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