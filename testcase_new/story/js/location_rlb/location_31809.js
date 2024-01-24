/******************************************************************************
 * @Description   : seqDB-31809:catalog节点异常，未超过MinKeepTime，手动停止Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.10.19
 * @LastEditTime  : 2023.10.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.clName = COMMCLNAME + "_31809";

main( test );
function test ( args )
{
   var dbcl = args.testCL;

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
      var options = { NodeName: masterNodeName, MinKeepTime: 10, MaxKeepTime: 20 };
      cataRG.startCriticalMode( options );

      // 检查Critical模式
      checkStartCriticalMode( db, CATALOG_GROUPNAME, options );

      // 节点恢复正常
      cataRG.start();
      commCheckBusinessStatus( db );

      // 未超过MinKeepTime，手动停止Critical模式
      cataRG.stopCriticalMode();

      checkGroupStopMode( db, CATALOG_GROUPNAME );

      // 集合插入数据并校验
      var docs = insertBulkData( dbcl, 1000 );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   } finally
   {
      cataRG.start();
      commCheckBusinessStatus( db );
      cataRG.stopCriticalMode();
      cata.close();
      db.deleteConf( { auth: 1 }, { Role: "catalog" } );
   }
}