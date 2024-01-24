/******************************************************************************
 * @Description   : seqDB-24310:取消执行成功的索引任务
 * @Author        : liuli
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24310";
testConf.clName = COMMCLNAME + "_maincl_24310";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( args )
{
   var indexName = "Index_24310";
   var csName = testConf.csName;
   var mainCLName = testConf.clName;
   var subCLName = "subcl_24310";
   var maincl = args.testCL;

   // 创建索引
   maincl.createIndex( indexName, { b: 1 } );

   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   insertBulkData( maincl, 1000 );
   maincl.copyIndex( csName + "." + subCLName );
   insertBulkData( maincl, 1000 );

   checkCopyTask( csName, mainCLName, indexName, csName + "." + subCLName, 0 );
   maincl.dropIndex( indexName );

   // 取消创建索引任务
   var taskId = getTaskID( csName + "." + mainCLName, "Create index" );
   assert.tryThrow( SDB_TASK_ALREADY_FINISHED, function()
   {
      db.cancelTask( taskId );
   } );

   // 取消删除索引任务
   var taskId = getTaskID( csName + "." + mainCLName, "Drop index" );
   assert.tryThrow( SDB_TASK_ALREADY_FINISHED, function()
   {
      db.cancelTask( taskId );
   } );

   // 取消复制索引任务
   var taskId = getTaskID( csName + "." + mainCLName, "Copy index" );
   assert.tryThrow( SDB_TASK_ALREADY_FINISHED, function()
   {
      db.cancelTask( taskId );
   } );
}

function getTaskID ( fullName, taskTypeDesc )
{
   var cursor = db.listTasks( { Name: fullName, TaskTypeDesc: taskTypeDesc } );
   var taskNum = 0;
   while( cursor.next() )
   {
      var task = cursor.current().toObj();
      var taskId = task.TaskID;
      taskNum++;
   }
   cursor.close();
   assert.equal( 1, taskNum, "the expected number of tasks is 1, the actual number of tasks is " + taskNum );
   return taskId;
}