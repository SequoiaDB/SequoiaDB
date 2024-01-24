/******************************************************************************
 * @Description   : seqDB-24809:锁升级节点与 session 配置的优先级验证
 * @Author        : liuli
 * @CreateTime    : 2021.12.15
 * @LastEditTime  : 2022.01.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24809";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   try
   {
      // 新建一个连接
      var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

      // 更新节点配置并校验
      var config = { transallowlockescalation: false, transmaxlocknum: 10, transmaxlogspaceratio: 10 };
      db.updateConf( config );
      var actConfig = { transallowlockescalation: "FALSE", transmaxlocknum: 10, transmaxlogspaceratio: 10 };
      checkSnapshot( db, actConfig );

      // 校验会话属性
      var option = { TransAllowLockEscalation: false, TransMaxLockNum: 10, TransMaxLogSpaceRatio: 10 };
      checkSessionAttr( db, option );

      // 使用新建的连接校验会话属性并测试按节点配置生效
      checkSessionAttr( db2, option );
      db.transBegin();
      // 插入10条数据，再次插入一条数据
      insertData( dbcl, 10 );
      dbcl.insert( { a: 11 } );
      assert.tryThrow( SDB_DPS_TRANS_LOCK_UP_TO_LIMIT, function()
      {
         dbcl.insert( { a: 12 } );
      } );
      db.transCommit();

      // 配置会话属性并校验
      var sessionAttr = { TransAllowLockEscalation: true, TransMaxLockNum: 20, TransMaxLogSpaceRatio: 20 };
      db2.setSessionAttr( sessionAttr );
      checkSessionAttr( db2, sessionAttr );

      // 会话属性优先级高于节点配置，测试按会话属性生效
      var dbcl2 = db2.getCS( COMMCSNAME ).getCL( COMMCLNAME + "_24809" );
      db2.transBegin();
      // 插入超过20条数据，插入成功
      insertData( dbcl2, 30 );
      db2.transCommit();
   }
   finally
   {
      db.deleteConf( { transallowlockescalation: "", transmaxlocknum: "", transmaxlogspaceratio: "" } );
      db2.close();
   }
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, { role: "data" } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var transallowlockescalation = obj.transallowlockescalation;
      var transmaxlocknum = obj.transmaxlocknum;
      var transmaxlogspaceratio = obj.transmaxlogspaceratio;
      assert.equal( transallowlockescalation, option.transallowlockescalation );
      assert.equal( transmaxlocknum, option.transmaxlocknum );
      assert.equal( transmaxlogspaceratio, option.transmaxlogspaceratio );
   }
   cursor.close();
}

function checkSessionAttr ( sdb, option )
{
   var obj = sdb.getSessionAttr().toObj();
   assert.equal( obj.TransAllowLockEscalation, option.TransAllowLockEscalation );
   assert.equal( obj.TransMaxLockNum, option.TransMaxLockNum );
   assert.equal( obj.TransMaxLogSpaceRatio, option.TransMaxLogSpaceRatio );
}