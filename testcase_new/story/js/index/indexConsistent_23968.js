/******************************************************************************
 * @Description   : seqDB-23968:取消异步创建索引任务   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2022.01.18
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range" };
testConf.clName = COMMCLNAME + "_index23968";

main( test );
function test ( testPara )
{
   var indexName = "Index_23968";
   var recordNum = 30000;
   var expRecs = insertBulkData( testPara.testCL, recordNum );
   var taskID = testPara.testCL.createIndexAsync( indexName, { a: 1, b: 1 } );
   //随机等待200ms再取消任务，覆盖任务ready和running阶段
   var waitTime = Math.floor( Math.random() * 200 );
   sleep( waitTime );

   try
   {
      db.cancelTask( taskID );
      checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName], -243 );
      checkIndexExist( db, COMMCSNAME, testConf.clName, indexName, false );
   }
   catch( e )
   {
      if( e == SDB_TASK_ALREADY_FINISHED )
      {  //如果任务已执行完成，则取消失败，校验执行完成结果
         checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName], 0 );
         checkIndexExist( db, COMMCSNAME, testConf.clName, indexName, true );
      }
      else
      {
         throw new Error( e );
      }
   }

   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
}
