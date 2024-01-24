/******************************************************************************
 * @Description   : seqDB-23142:对range表进行数据重分布，cl对应cs关联多个数据组
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.02.26
 * @LastEditTime  : 2021.03.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var rgNames = commGetDataGroupNames( db );
   if( rgNames.length < 3 )
   {
      println( "少于3个数据组，跳过用例" );
      return;
   }

   var rgNames = commGetDataGroupNames( db );
   rgNames.sort();
   var dmName = CHANGEDPREFIX + "_dm_23142";
   var csName = CHANGEDPREFIX + "_cs_23142";
   var clName = CHANGEDPREFIX + "_cl_23142";
   var fullCLName = csName + "." + clName;
   var insertRecsNum = 300000;
   var taskName = "rebalanceTask_23144";

   // 准备集合
   commDropDomain( db, dmName );
   commDropCS( db, csName );
   commDropCL( db, csName, clName );
   db.createDomain( dmName, rgNames );
   db.createCS( csName, { Domain: dmName } );
   var cl = db.getCS( csName )
      .createCL( clName, { "ShardingType": "range", "ShardingKey": { "a": 1 }, "Group": rgNames[0] } );

   // 准备数据
   // 多个组都有数据，但不均匀
   cl.insert( [{ "a": 0 }, { "a": insertRecsNum }, { "a": insertRecsNum + 1 }] );
   cl.split( rgNames[0], rgNames[1], { "a": insertRecsNum }, { "a": insertRecsNum + 1 } );
   cl.split( rgNames[0], rgNames[2], { "a": insertRecsNum + 1 }, { "a": { "$maxKey": 1 } } );
   // 向其中一个组插入大批量数据
   insertRecords( cl, insertRecsNum );
   // 启动监听进程，创建任务
   try
   {
      // 清理可能被残留的task
      removeTask( taskName, true );

      // 启动监听进程
      startTask();

      // 创建任务
      createRebalanceTask( taskName, "once", fullCLName );

      // 检查任务执行结果
      checkResults( rgNames[0], csName, clName, 100002 );
      checkResults( rgNames[1], csName, clName, 100002 );
      checkResults( rgNames[2], csName, clName, 99999 );
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