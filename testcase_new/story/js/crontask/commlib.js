/******************************************************************************
 * @Description   : cronTask公共方法
 *    只能实现rebalance任务自动化。
 *    shrink任务对主机缩容，CI无法适配（需要有一个主机支持将所有数据缩容到其他主机，对其他用例有影响）
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.03.10
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var cmd = getRemoteCmd();
var scriptPath = commGetInstallPath() + "/tools/crontask/";

/*****************************************************
@description : insert records
   数据重分布任务，均衡单位>16M，最低17M，约25万条数据
@return      : records
******************************************************/
function insertRecords ( cl, insertRecsNum )
{
   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var records = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         records.push( { "a": i, "b": i, "c": "test" + i, "d": "helloword" } );
      };
      cl.insert( records );
   }
}

/*****************************************************
@description : check results
   数据重分布任务，后端实际执行的split将部分数据切分到其他集合
   检查结果只需要检查数据有切分，且切分的数据量正确就可以了
******************************************************/
function checkResults ( rgName, csName, clName, expCount )
{
   var mstDB = null;
   try
   {
      mstDB = db.getRG( rgName ).getMaster().connect();

      // 检查集合是否存在，适用于切分后的目标组
      // 重试次数，每次暂停1秒钟
      var retryTimes = 60;
      var cl = null;
      while( retryTimes-- )
      {
         try
         {
            cl = mstDB.getCS( csName ).getCL( clName );
            break;
         }
         catch( e )
         {
            if( e === -34 || e === -23 )
            {
               sleep( 1000 );
            }
            else
            {
               throw e;
            }
         }
      }

      // 检查数据正确性
      // 重试次数，每次暂停1秒钟
      var retryTimes = 700;
      var actCount = 0;
      cl = mstDB.getCS( csName ).getCL( clName );
      while( retryTimes-- )
      {
         actCount = cl.count();
         if( Number( actCount ) === expCount )
         {
            return;
         }
         sleep( 1000 );
      }
      actCount = cl.count();
      assert.equal( actCount, expCount, "重试超时" );
   }
   finally
   {
      if( mstDB !== null )
         mstDB.close();
   }
}

/*****************************************************
@description : create rebalance task
   sdbtaskctl create <TASKNAME> <-t TASKTYPE> <-b BEGINTIME> <-f FREQUENCY> [-l CLNAME] 
   [-u UNIT] [--hosts HOSTLIST]
@parameter   : 
   taskName, 任务名称
   taskFrequency, 任务频率，取值：daily / once
   clName, 集合名称
   rebalanceUnit, 均衡单位，默认17M
******************************************************/
function createRebalanceTask ( taskName, taskFrequency, clName, rebalanceUnit )
{
   if( typeof ( rebalanceUnit ) == "undefined" ) { rebalanceUnit = "17"; }

   // 任务开始时间为当前时间，获取当前时间（如2021-03-10_09:35:39）
   var localDate = cmd.run( "date +%Y-%m-%d_%H:%M:%S" );
   var hmsTime = localDate.split( "_" )[1].split( ":" );//如09,35,39
   var hour = Number( hmsTime[0] );
   var minutes = Number( hmsTime[1] );
   var seconds = Number( hmsTime[2] );

   // 任务开始时间，至少从当前时间的下一分钟监听进程才能扫描到并执行任务
   minutes = minutes + 1;
   // 容错时间，如果时间为23:30:58，及时分钟加1，但是秒钟即满
   // 此时为了保证任务能按时执行，分钟再次加1，此时秒钟不影响任务执行时间（任务开始时间格式为hh:mm）
   var faultTolSeconds = 5;
   // 任务时间为hh:mm，判断当前minutes是否即将加1，如果获取当前时间后minutes加1，则任务可能过了时间不再执行
   // 如果任务即将加1大于60则minutes直接加1，如："23:30:58"，则修改hh:mm为"23:31"
   if( ( seconds + faultTolSeconds ) >= 60 )
   {
      minutes = minutes + 1;
   }
   // 判断minutes加1后时间如果大于60，则hour直接加1，minutes值为00
   // 如："22:59"加1分钟变为"22:60"，则纠正hh:mm为"23:00"
   if( minutes >= 60 )
   {
      hour = hour + 1;
      minutes = minutes - 60; // 如10:60->11:00, 10:61->11:01
      // 判断hour加1后是否大于24，如果大于24则修改为"00"，即hh:mm修改为"00:00"
      if( hour >= 24 )
      {
         hour = hour - 24;
      }
   }
   var taskTime = "" + hour + ":" + minutes;
   println( "taskName = " + taskName + ", taskTime = " + taskTime + ", localDate = " + localDate );

   // 创建任务
   var command = scriptPath + "sdbtaskctl create " + taskName
      + " -t rebalance"
      + " -f " + taskFrequency
      + " -b " + taskTime
      + " -l " + clName
      + " -u " + rebalanceUnit;
   try
   {
      cmd.run( command );
   }
   catch( e )
   {
      println( "command = " + command + "\n" );
      throw new Error( e );
   }
}

/*****************************************************
@description : list task
   sdbtaskctl list [-t TASKTYPE]
******************************************************/
function listTask ()
{
   var command = scriptPath + "sdbtaskctl list";
   var list = cmd.run( command );
   return list;
}

/*****************************************************
@description : remove task
   sdbtaskctl remove <TASKNAME>
@parameter   : 
   taskName, 任务名称
   ignoreError, 是否忽略错误，取值：true / false，默认false
******************************************************/
function removeTask ( taskName, ignoreError )
{
   if( typeof ( ignoreError ) == "undefined" ) { ignoreError = false; }
   var command = scriptPath + "sdbtaskctl remove " + taskName;
   try
   {
      cmd.run( command );
   }
   catch( e )
   {
      if( !ignoreError )
      {
         println( "\n" + command + "\n" );
         throw new Error( e );
      }
   }
}

/*****************************************************
@description : start task
   start: ./sdbtaskdaemon >> ./sdbtaskdaemon.log 2>&1
@parameter   : 
   ignoreError, 是否忽略错误，取值：true / false，默认true
******************************************************/
function startTask ( ignoreError )
{
   if( typeof ( ignoreError ) == "undefined" ) { ignoreError = true; }
   var command = scriptPath + "sdbtaskdaemon >> ./sdbtaskdaemon.log 2>&1 &";
   try
   {
      cmd.run( command );
   }
   catch( e )
   {
      if( !ignoreError )
      {
         println( "\n" + command + "\n" );
         throw new Error( e );
      }
   }
}

/*****************************************************
@description : stop task
   stop: ./sdbtaskdaemon --stop
******************************************************/
function stopTask ()
{
   var command = scriptPath + "sdbtaskdaemon --stop";
   cmd.run( command );
}

/*****************************************************
@description : taskStatus
   status: ./sdbtaskdaemon --status
@return      : exec results
******************************************************/
function taskStatus ()
{
   var command = scriptPath + "sdbtaskdaemon --status";
   var rc = cmd.run( command );
   return rc;
}

/*****************************************************
@description : new Cmd, 此任务只支持sdb所属用户执行sdbtaskctl，不需要要远程cmd才可以
@return      : cmd
******************************************************/
function getRemoteCmd ()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var cmd = remote.getCmd();
   return cmd;
}