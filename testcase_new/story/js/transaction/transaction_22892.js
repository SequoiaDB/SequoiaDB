/******************************************************************************
*@Description  seqDB-22892:alter修改Shardingkey读老版本sharding索引
               1. 创建普通集合使用 enableSharding 开启分区，并插入记录
                 {_id:1,a:1,b:1}、{_id:2,a:2,b:2}
               2. 建立连接1，开启事务 remove {_id:2} 的记录

               3. 建立连接2，rc 级别下开启事务，分别走表扫描和 $shard 索引查询 {a:2}
                  表扫描和 $shard 索引都能查询到 {a:2} 记录

               4. 建立连接3，修改 shardingkey

               5. 在连接2再次走表扫描和 $shard 索引查询 {a:2}
                  表扫描和 $shard 索引都能查询到 {a:2} 记录
*@author      liyuanyue
*@createdate  2020.11.12
SEQUOIADBMAINSTREAM-6107
******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_22892";
main( test );

function test ( args )
{
   var CSName = COMMCSNAME;
   var CLName = COMMCLNAME + "_22892";
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var db3 = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   var cl = args.testCL;

   cl.enableSharding( { ShardingKey: { a: 1 } } );
   cl.insert( { _id: 1, a: 1, b: 1 } );
   cl.insert( { _id: 2, a: 2, b: 2 } );

   try
   {
      // 连接1开启事务 remove 记录
      db1.transBegin();
      var cl1 = db1.getCS( CSName ).getCL( CLName );
      cl1.remove( { _id: 2 } );

      // 连接2开启事务查询
      db2.setSessionAttr( { TransIsolation: 1, TransLockWait: false } );
      db2.transBegin();
      var cl2 = db2.getCS( CSName ).getCL( CLName );
      queryAncCheck( cl2 );

      // 连接3修改ShardingKey
      db3.getCS( CSName ).getCL( CLName ).enableSharding( { ShardingKey: { b: 1 } } );

      // 连接2再次查询
      var cl2 = db2.getCS( CSName ).getCL( CLName );
      queryAncCheck( cl2 );
   } finally
   {
      db1.transCommit();
      db2.transBegin();
   }
}

function queryAncCheck ( cl )
{
   var actResult = cl.find( { a: 2 } ).hint( { "": "$shard" } ).current().toObj();
   var expResult = { "_id": 2, "a": 2, "b": 2 };
   assert.equal( actResult, expResult );

   var actResult = cl.find( { a: 2 } ).hint( { "": null } ).current().toObj();
   var expResult = { "_id": 2, "a": 2, "b": 2 };
   assert.equal( actResult, expResult );
}