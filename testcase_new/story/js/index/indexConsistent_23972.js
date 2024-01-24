/******************************************************************************
 * @Description   : seqDB-23972:切分表取消删除索引任务
 * @Author        : liuli
 * @CreateTime    : 2021.08.04
 * @LastEditTime  : 2022.01.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23972";
testConf.clOpt = { ShardingKey: { d: 1 }, AutoSplit: true };

main( test );
function test ( args )
{
   var indexName = "index_23972";
   var clName = testConf.clName;
   var dbcl = args.testCL;

   // 创建索引
   dbcl.createIndex( indexName, { a: 1, b: 1, c: 1 } );

   // 插入大量数据
   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      docs.push( { a: i, b: i, c: i, d: i } );
   }
   dbcl.insert( docs );

   // 删除索引
   var taskId = dbcl.dropIndexAsync( indexName );

   // 循环获取任务，随机选择Ready状态或Running状态取消任务
   var num = parseInt( Math.random() * 10 );
   cancelTaskAsRequired( COMMCSNAME + "." + clName, "Drop index", taskId, num );

   // 等待任务结束后校验任务状态
   waitTaskFinish( COMMCSNAME, clName, "Drop index" );

   var resultCode = getTaskResultCode( COMMCSNAME + "." + clName, "Drop index" );

   if( resultCode == 0 )
   {
      commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, false );
   } else
   {
      commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, true );
   }
}

function getTaskResultCode ( fullName, taskTypeDesc )
{
   var cur = db.listTasks( { Name: fullName, TaskTypeDesc: taskTypeDesc } );
   var taskNum = 0;
   while( cur.next() )
   {
      var resultCode = cur.current().toObj().ResultCode;
      taskNum++;
   }
   cur.close();
   assert.equal( 1, taskNum, "the expected number of tasks is 1, the actual number of tasks is " + taskNum );
   return resultCode;
}

// status为偶数在Ready状态取消，为奇数在Running状态取消任务
function cancelTaskAsRequired ( fullName, taskTypeDesc, taskId, status )
{
   var timeOut = 5000;
   var doTime = 0;
   do
   {
      var cur = db.listTasks( { Name: fullName, TaskTypeDesc: taskTypeDesc } );
      while( cur.next() )
      {
         var statusDesc = cur.current().toObj().StatusDesc;
      }
      cur.close();
      if( status % 2 == 0 )
      {
         if( statusDesc == "Ready" && doTime > status )
         {
            try
            {
               db.cancelTask( taskId );
            }
            catch( e )
            {
               if( e != SDB_TASK_CANNOT_CANCEL && e != SDB_TASK_ALREADY_FINISHED )
               {
                  throw new Error( e );
               }
            }
            break;
         }
      } else
      {
         if( statusDesc == "Running" )
         {
            try
            {
               db.cancelTask( taskId );
               throw new Error( "expected cancel task failed, actual success !" );
            }
            catch( e )
            {
               if( e != SDB_TASK_CANNOT_CANCEL && e != SDB_TASK_ALREADY_FINISHED )
               {
                  throw new Error( e );
               }
            }
            break;
         }
      }
      sleep( 0.5 );
      doTime += 0.5;
   } while( doTime < timeOut )
}