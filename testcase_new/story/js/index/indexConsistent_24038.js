/******************************************************************************
 * @Description   : seqDB-24038:创建集合开启、关闭切分功能
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2022.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24038";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var clName = COMMCLNAME + "_24038";
   var indexName = "$shard";
   var srcGroup = testPara.srcGroupName;
   var dstGroup = testPara.dstGroupNames;

   checkExistIndex( db, COMMCSNAME, clName, indexName );

   // 关闭切分功能
   cl.disableSharding();

   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      cl.getIndex( indexName );
   } );

   // 使用enableSharding指定分区键和方式进行分区
   cl.enableSharding( { "ShardingKey": { "a": 1 }, "ShardingType": "range" } );
   cl.split( srcGroup, dstGroup[0], { a: 1000 } );
   checkExistIndex( db, COMMCSNAME, clName, indexName );

   var docs = insertBulkData( cl, 2000 );

   // 直连数据节点校验数据，校验原组数据
   var data1 = db.getRG( srcGroup ).getMaster().connect();
   var dataCL1 = data1.getCS( COMMCSNAME ).getCL( clName );
   var actResult = dataCL1.find().sort( { a: 1 } );
   commCompareResults( actResult, docs.slice( 0, 1000 ) );
   data1.close();

   // 校验目标组数据
   var data2 = db.getRG( dstGroup[0] ).getMaster().connect();
   var dataCL2 = data2.getCS( COMMCSNAME ).getCL( clName );
   var actResult = dataCL2.find().sort( { a: 1 } );
   commCompareResults( actResult, docs.slice( 1000 ) );
   data2.close();
}

// 校验主备节点存在$shard索引
function checkExistIndex ( db, csName, clName, indexName )
{
   var doTime = 0;
   var timeOut = 600000;
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
            dbcl.getIndex( indexName );
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