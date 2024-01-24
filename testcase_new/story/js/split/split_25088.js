/******************************************************************************
 * @Description   : seqDB-25088:取消切分任务，检查task信息
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.01.27
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_25088";
testConf.clOpt = { "ShardingKey": { "no": 1 }, "ShardingType": "hash" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var varCL = testPara.testCL;
   var isTaskFinish = false;
   var expResultCode = SDB_TASK_HAS_CANCELED;
   var expResultCodeDesc = "Task has been canceled";

   insertData( varCL, 5000 );
   var taskId = varCL.splitAsync( testPara.srcGroupName, testPara.dstGroupNames[0], 50 );
   var waitTime = parseInt( Math.random() * 5000 );
   sleep( waitTime );
   try
   {
      db.cancelTask( taskId );
   }
   catch( e )
   {
      if( e == SDB_TASK_ALREADY_FINISHED || e == SDB_TASK_CANNOT_CANCEL )
      {
         isTaskFinish = true;
      }
      else
      {
         throw new Error( "cancelTask error ! errorCode: " + e );
      }
   }

   try
   {
      // 等待任务结束
      db.waitTasks( taskId );
   }
   catch( e )
   {
      if( e != SDB_TASK_HAS_CANCELED )
      {
         throw new Error( "waitTasks error ! errorCode: " + e );
      }
   }

   var taskInfo = db.getTask( taskId ).toObj();
   var actResultCode = taskInfo.ResultCode;
   var actResultCodeDesc = taskInfo.ResultCodeDesc;
   if( isTaskFinish )
   {
      expResultCode = 0;
      expResultCodeDesc = "Succeed";
   }

   assert.equal( actResultCode, expResultCode, "taskInfo=" + JSON.stringify( taskInfo ) );
   assert.equal( actResultCodeDesc, expResultCodeDesc, "taskInfo=" + JSON.stringify( taskInfo ) );
}