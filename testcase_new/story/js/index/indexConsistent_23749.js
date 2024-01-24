/******************************************************************************
 * @Description   : seqDB-23749:切分表创建$shard索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.06.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_23749";
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "$shard";
   var srcGroup = testPara.srcGroupName;
   var dstGroup = testPara.dstGroupNames;
   var cl = testPara.testCL;

   // 开启集合分区属性
   cl.enableSharding( { ShardingKey: { "a": 1 }, ShardingType: "hash" } );

   // 检查不存在创建shard索引任务
   checkNoTask( COMMCSNAME, testConf.clName, "Create index" );

   // 删除shard索引
   assert.tryThrow( SDB_IXM_DROP_SHARD, function()
   {
      cl.dropIndex( indexName );
   } );

   // 检查不存在删除shard索引任务
   checkNoTask( COMMCSNAME, testConf.clName, "Drop index" );

   // 插入数据并进行切分
   insertBulkData( cl, 300 );
   cl.split( srcGroup, dstGroup[0], { Partition: 10 }, { Partition: 20 } );

   // 校验索引一致性
   checkExistIndex( db, COMMCSNAME, testConf.clName, indexName, true );
}

function checkExistIndex ( db, csName, clName, idxName )
{
   var doTime = 0;
   var timeOut = 300000;
   var nodes = commGetCLNodes( db, csName + "." + clName );
   do
   {
      var sucNodes = 0;
      for( var i = 0; i < nodes.length; i++ )
      {
         var seqdb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
         try
         {
            var dbcl = seqdb.getCS( csName ).getCL( clName );
         }
         catch( e )
         {
            if( e != SDB_DMS_NOTEXIST && e != SDB_DMS_CS_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         try
         {
            dbcl.getIndex( idxName );
            sucNodes++;
         }
         catch( e )
         {
            if( e != SDB_IXM_NOTEXIST )
            {
               throw new Error( e );
            }
            break;
         }
         seqdb.close();
      }
      sleep( 200 );
      doTime += 200;
   } while( doTime < timeOut && sucNodes < nodes.length )
   if( doTime >= timeOut )
   {
      throw new Error( "check timeout index not synchronized !" );
   }
}