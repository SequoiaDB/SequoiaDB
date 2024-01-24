/******************************************************************************
 * @Description   : seqDB-23144:对hash表进行数据重分布，cl对应cs关联多个数据组
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

   rgNames.sort();
   var dmName = CHANGEDPREFIX + "_dm_23144";
   var csName = CHANGEDPREFIX + "_cs_23144";
   var clName = CHANGEDPREFIX + "_cl_23144";
   var fullCLName = csName + "." + clName;
   var insertRecsNum = 50000;
   var taskName = "rebalanceTask_23144";

   // 准备集合
   commDropDomain( db, dmName );
   commDropCS( db, csName );
   commDropCL( db, csName, clName );
   db.createDomain( dmName, rgNames );
   db.createCS( csName, { Domain: dmName } );
   var cl = db.getCS( csName )
      .createCL( clName, { "ShardingType": "hash", "ShardingKey": { "a": 1 }, "Group": rgNames[0] } );

   // 准备数据
   // hash按partition个数计算差值，默认差值为2个partition，上述是为了构造多个partition，其中group3没有此cl
   cl.split( rgNames[0], rgNames[1], { "Partition": 10 }, { "Partition": 20 } );
   cl.split( rgNames[0], rgNames[1], { "Partition": 30 }, { "Partition": 40 } );
   cl.split( rgNames[0], rgNames[1], { "Partition": 50 }, { "Partition": 60 } );
   cl.split( rgNames[0], rgNames[1], { "Partition": 70 }, { "Partition": 80 } );
   cl.split( rgNames[0], rgNames[1], { "Partition": 90 }, { "Partition": 100 } );
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
      checkResults( rgNames[0], csName, clName, 16694 );
      checkResults( rgNames[1], csName, clName, 16768 );
      checkResults( rgNames[2], csName, clName, 16538 );
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