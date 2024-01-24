/******************************************************************************
 * @Description   : seqDB-23974:主表取消复制索引任务
 * @Author        : liuli
 * @CreateTime    : 2021.08.04
 * @LastEditTime  : 2022.01.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_maincl_23974";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( args )
{
   var indexName = "index_23974";
   var clName = testConf.clName;
   var subCLName1 = "subcl_23974_1";
   var subCLName2 = "subcl_23974_2";
   var maincl = args.testCL;

   // 创建索引
   maincl.createIndex( indexName, { b: 1, c: 1 } );

   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5000 } } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 5000 }, UpBound: { a: 10000 } } );

   // 插入大量数据
   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      docs.push( { a: i, b: i, c: i, d: i } );
   }
   maincl.insert( docs );

   // 复制索引
   var taskId = maincl.copyIndexAsync();

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

   // 等待任务结束后进行校验
   waitTaskFinish( COMMCSNAME, clName, "Copy index" );
   waitTaskFinish( COMMCSNAME, subCLName1, "Create index" );
   waitTaskFinish( COMMCSNAME, subCLName2, "Create index" );
   maincl.getIndex( indexName );

   var resultCode1 = getTaskResultCode( COMMCSNAME + "." + subCLName1, "Create index" );
   var resultCode2 = getTaskResultCode( COMMCSNAME + "." + subCLName2, "Create index" );

   if( resultCode1 == 0 )
   {
      commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, true );
   }
   else
   {
      commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, false );
   }

   if( resultCode2 == 0 )
   {
      commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, true );
   }
   else
   {
      commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, false );
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