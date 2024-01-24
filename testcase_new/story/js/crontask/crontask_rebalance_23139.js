/******************************************************************************
 * @Description   : seqDB-23139:创建/列取/删除周期性数据重分布任务，集合数据分布不均匀
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.02.26
 * @LastEditTime  : 2021.03.03
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ( testPara )
{
   var rgNames = commGetDataGroupNames( db );
   var dmName = CHANGEDPREFIX + "_dm_23139";
   var csName = CHANGEDPREFIX + "_cs_23139";
   var clName = CHANGEDPREFIX + "_cl_23139";
   var fullCLName = csName + "." + clName;
   var insertRecsNum = 250000;
   var taskName = "rebalanceTask_23139";

   // 准备集合
   commDropDomain( db, dmName );
   commDropCS( db, csName );
   commDropCL( db, csName, clName );
   db.createDomain( dmName, rgNames.slice( 0, 2 ) );
   db.createCS( csName, { Domain: dmName } );
   var cl = db.getCS( csName )
      .createCL( clName, { "ShardingType": "range", "ShardingKey": { "a": 1 }, "Group": rgNames[0] } );

   // 准备数据
   insertRecords( cl, insertRecsNum );
   // 启动监听进程，创建任务
   try
   {
      // 清理可能被残留的task
      removeTask( taskName, true );

      // 启动监听进程
      startTask();

      // 创建任务
      createRebalanceTask( taskName, "daily", fullCLName );

      // 列取任务
      var taskList = listTask();
      if( taskList.indexOf( taskName ) === -1 )
      {
         throw new Error( "列取任务失败，不包含" + taskName + "任务, taskList = " + taskList );
      }

      // 检查任务执行结果
      checkResults( rgNames[0], csName, clName, insertRecsNum / 2 );
      checkResults( rgNames[1], csName, clName, insertRecsNum / 2 );

      // 删除任务
      removeTask( taskName );
      // 列取任务
      var taskList = listTask();
      assert.equal( taskList.indexOf( taskName ), -1, "taskList = " + taskList );
   }
   finally 
   {
      // 停止监听进程
      stopTask();
   }
   // 清理环境
   commDropCS( db, csName );
   commDropDomain( db, dmName );
}