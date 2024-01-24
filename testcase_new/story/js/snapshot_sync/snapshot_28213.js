/******************************************************************************
 * @Description   : seqDB-28213 事务中查询快照 
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.17
 * @LastEditTime  : 2022.10.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28213";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   // 插入数据
   dbcl.insert( { a: 1 } );
   // 会话属性设置Source，用于后续查询会话快照时匹配
   var source = "source_28213";
   db.setSessionAttr( { Source: source } );

   // 开启事务后读数据
   db.transBegin();
   dbcl.find().toArray();

   // 获取数据库快照非聚合结果
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var dataBastInfos = getSnapshotResults( cursor );

   // 获取会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var sessionInfos = getSnapshotResults( cursor );

   // 写入数据
   dbcl.insert( { a: 2 } );

   // 校验数据库快照
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true }, {}, { NodeName: 1 } );
   var actDataBastInfos = getSnapshotResults( cursor );
   assert.equal( actDataBastInfos, dataBastInfos );

   // 校验会话快照
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { RawData: true, Source: source }, {}, { NodeName: 1 } );
   var actSessionInfos = getSnapshotResults( cursor );
   assert.equal( actSessionInfos, sessionInfos );

   db.transCommit();
}