/******************************************************************************
*@Description : test js object System function: listProcess isProcExist
*                                               killProcess
*               TestLink : 10645 System对象枚举进程如sdbcm
*                          10646 System对象判断进程是否存在
*                          10647 System对象判断进程是否存在，type取值name/pid，与value不匹配
*                          10648 System对象杀死进程
*@author      : Liang XueWang
******************************************************************************/

// 测试枚举进程
SystemTest.prototype.testListProcess = function()
{
   this.init();

   // 测试detail为true时枚举sdbcm进程
   var cmProcs;
   cmProcs = this.system.listProcess( { detail: true },
      { cmd: "sdbcm(" + CMSVCNAME + ")" } ).toArray();
   if( cmProcs.length !== 1 )
   {
      throw new Error( "testListProcess", null,
         "list sdbcm process " + this, 1, cmProcs.length );
   }
   var command = "ps aux 2>/dev/null | grep 'sdbcm(" + CMSVCNAME + ")' | grep -v grep | " +
      "awk '{print $1,$2,$8,$11}'";
   var result = this.cmd.run( command ).split( "\n" )[0].split( " " );
   var user = result[0];     // 进程用户
   var pid = result[1];      // 进程id
   var stat = result[2];     // 进程状态
   var order = result[3];    // 进程命令
   var obj = JSON.parse( cmProcs[0] );
   if( user !== obj.user || pid !== obj.pid ||
      stat !== obj.status || order !== obj.cmd )
   {
      throw new Error( "testListProcess fail,list sdbcm process detail true " + this + result + cmProcs[0] );
   }

   // 测试detail为false时枚举sdbcm进程
   cmProcs = this.system.listProcess( { detail: false },
      { cmd: "sdbcm(" + CMSVCNAME + ")" } ).toArray();
   obj = JSON.parse( cmProcs[0] );
   if( undefined !== obj.user || pid !== obj.pid ||
      undefined !== obj.stat || order !== obj.cmd )
   {
      throw new Error( "testListProcess fail,list sdbcm process detail false " + this + result + cmProcs[0] );
   }

   this.release();
}

// 测试判断进程是否存在
SystemTest.prototype.testIsProcExist = function()
{
   this.init();

   // 测试判断存在的sdbcm进程
   var result;
   result = this.system.isProcExist( { value: "sdbcm(" + CMSVCNAME + ")", type: "name" } );
   if( result !== true )
   {
      throw new Error( "testIsProcExist fail, test sdbcm exist " + this + true + result );
   }
   // 测试判断sdbcm进程，type与value不匹配
   result = this.system.isProcExist( { value: "sdbcm(" + CMSVCNAME + ")", type: "pid" } );
   if( result !== false )
   {
      throw new Error( "testIsProcExist fail,test sdbcm mismatch " + this + false + result );
   }
   // 测试判断不存在的进程
   result = this.system.isProcExist( { value: "sdbcm", type: "name" } );
   if( result !== false )
   {
      throw new Error( "testIsProcExist fail,test sdbcm notexist " + this + false + result );
   }

   this.release();
}

// 测试杀死进程，强杀
SystemTest.prototype.testKillProcessWithSigKill = function()
{
   this.init();

   // 创建进程并获取进程id
   this.cmd.run( "rm -rf /tmp/term.txt" );
   var command = "trap \"echo 'term' > /tmp/term.txt && exit 15\" 15; while true; "
      + "do sleep 1s; done";
   var pid = this.cmd.start( command );

   // 测试强杀进程后变成僵尸进程
   var option = {};
   option["pid"] = pid;
   option["sig"] = "kill";
   this.system.killProcess( option );
   process = this.system.listProcess( {}, { "pid": "" + pid } ).toArray();
   assert.equal( process.length, 1 );
   assert.notEqual( JSON.parse( process[0] ).cmd.indexOf( "defunct" ), -1 );
   try
   {
      this.cmd.run( "ls /tmp/term.txt" );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != 2 )
      {
         throw e;
      }
   }

   this.release();
}

// 测试杀死进程，普通杀
SystemTest.prototype.testKillProcessWithSigTerm = function()
{
   this.init();

   // 创建进程并获取进程id
   this.cmd.run( "rm -rf /tmp/term.txt" );
   var command = "trap \"echo 'term' > /tmp/term.txt && exit 15\" 15; while true; "
      + "do sleep 1s; done";
   var pid = this.cmd.start( command );

   // 测试普通杀进程
   var option = {};
   option["pid"] = pid;
   option["sig"] = "term";
   this.system.killProcess( option );
   process = this.system.listProcess( {}, { "pid": "" + pid } ).toArray();
   var start = new Date().getTime();
   while( process.length !== 0 )
   {
      sleep( 1000 );
      process = this.system.listProcess( {}, { "pid": "" + pid } ).toArray();
      var end = new Date().getTime();
      if( end - start > 10000 )
         break;
   }
   assert.equal( process.length, 0 );

   this.cmd.run( "ls /tmp/term.txt" );
   this.cmd.run( "rm -rf /tmp/term.txt" );

   this.release();
}


main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试枚举进程
      systems[i].testListProcess();

      // 测试判断进程是否存在
      systems[i].testIsProcExist();

      // 测试强杀进程
      // systems[i].testKillProcessWithSigKill() ;

      // 测试普通杀进程
      // systems[i].testKillProcessWithSigTerm() ;
   }
}

