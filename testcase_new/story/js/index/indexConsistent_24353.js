/******************************************************************************
 * @Description   : seqDB-24353:子表取消创建索引任务 
 * @Author        : liuli
 * @CreateTime    : 2021.08.04
 * @LastEditTime  : 2022.01.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var indexName = "index_24353";
   var csName = "cs_24353";
   var mainCLName = "maincl_24353";
   var subCLName1 = "subcl_24353_1";
   var subCLName2 = "subcl_24353_2";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 16000 } } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 16000 }, UpBound: { a: 20000 } } );

   // 插入大量数据
   insertBulkData( maincl, 20000 );

   // 创建索引
   maincl.createIndexAsync( indexName, { c: 1 } );

   // 随机选择一个子表取消任务
   var cancel = parseInt( Math.random() * 2 );
   if( cancel == 0 )
   {
      var cur = db.listTasks( { Name: csName + "." + subCLName1, TaskTypeDesc: "Create index" } );
   }
   else
   {
      var cur = db.listTasks( { Name: csName + "." + subCLName2, TaskTypeDesc: "Create index" } );
   }
   while( cur.next() )
   {
      var obj = cur.current().toObj();
      var taskId = obj.TaskID;
   }
   cur.close();

   // 随机等待1000ms后取消任务
   var time = parseInt( Math.random() * 1000 );
   sleep( time );
   try
   {
      db.cancelTask( taskId );
   }
   catch( e )
   {
      if( e != SDB_TASK_ALREADY_FINISHED )
      {
         throw new Error( e );
      }
   }

   waitTaskFinish( csName, mainCLName, "Create index" );
   waitTaskFinish( csName, subCLName1, "Create index" );
   waitTaskFinish( csName, subCLName2, "Create index" );

   var resultCode1 = getTaskResultCode( csName + "." + subCLName1, "Create index" );
   var resultCode2 = getTaskResultCode( csName + "." + subCLName2, "Create index" );
   var mainCode = getTaskResultCode( csName + "." + mainCLName, "Create index" );

   if( resultCode1 == 0 )
   {
      commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   }
   else
   {
      commCheckIndexConsistent( db, csName, subCLName1, indexName, false );
   }

   if( resultCode2 == 0 )
   {
      commCheckIndexConsistent( db, csName, subCLName2, indexName, true );
   }
   else
   {
      commCheckIndexConsistent( db, csName, subCLName2, indexName, false );
   }

   if( mainCode == 0 )
   {
      checkIndexExist( db, csName, mainCLName, indexName, true );
   }
   else
   {
      checkIndexExist( db, csName, mainCLName, indexName, false );
   }

   commDropCS( db, csName );
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