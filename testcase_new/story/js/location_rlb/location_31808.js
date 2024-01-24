/******************************************************************************
 * @Description   : seqDB-31808:两个catalog节点异常，剩余节点启动Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.10.19
 * @LastEditTime  : 2023.10.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var clName = "cl_31808";

   // 获取catalog主节点
   var cataRG = db.getCataRG();
   var master = cataRG.getMaster();
   var masterNodeName = master.getHostName() + ":" + master.getServiceName();
   var cata = master.connect();

   try
   {
      // catalog节点设置auth为false
      db.updateConf( { auth: false }, { Role: "catalog" } );

      // 停止catalog复制组
      cataRG.stop();

      // 启动原主节点
      master.start();

      // catalog未强制选主，启动Critical模式
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
      {
         cataRG.startCriticalMode( options );
      } )

      // 剩余catalog执行强制选主
      cata = new Sdb( masterNodeName );
      var timeOut = 30;
      var doTime = 0;

      // 先sleep一秒，再强制选主
      while( doTime < timeOut )
      {
         sleep( 1000 );
         try
         {
            cata.forceStepUp( { Seconds: 300 } );
            break;
         } catch( e )
         {
            if( e == SDB_CLS_NODE_INFO_EXPIRED )
            {
               doTime++;
            } else
            {
               throw e;
            }
         }
      }

      if( doTime >= timeOut )
      {
         cata.forceStepUp( { Seconds: 300 } );
      }

      // catalog启动Critical模式
      cataRG.startCriticalMode( options );

      // 检查Critical模式
      checkStartCriticalMode( db, CATALOG_GROUPNAME, options );

      // 创建集合并插入数据
      commDropCL( db, COMMCSNAME, clName );
      var dbcs = testPara.testCS;
      var dbcl = dbcs.createCL( clName );
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
      commDropCL( db, COMMCSNAME, clName );
   } finally
   {
      cataRG.start();
      commCheckBusinessStatus( db );
      cataRG.stopCriticalMode();
      cata.close();
      db.deleteConf( { auth: 1 }, { Role: "catalog" } );
   }
}