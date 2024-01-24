/******************************************************************************
*@Description : common function for lob import/export/migration tool
*               the testcase about user/password cannot run in concurrent.
*@Modify list :
*               2016-06-20  XueWang Liang  Change
******************************************************************************/
var hostName = COORDHOSTNAME;	// 主机名：'localhost'
var coordPort = COORDSVCNAME;	// 端口号：11810
var user = null;                // 用户名：
var passwd = null;              // 密码：
var cmd = new Cmd();
var LocalPath = null;           // 当前目录
var InstallPath = null;         // 安装目录

/******************************************************************************
*@Description : when run these testcase in sequoiadb or trunk fold that not
*               installed, get home fold.   <?how to get sequoiadb home fold?>
******************************************************************************/
function toolGetToolPath ()
{
   try
   {
      var local = cmd.run( "pwd" ).split( "\n" );
      LocalPath = local[0];
   }
   catch( e )
   {
      throw new Error( e );
   }

   try
   {
      var folder = cmd.run( 'ls ' + LocalPath ).split( '\n' );
   }
   catch( e )
   {
      throw new Error( e );
   }

   var fcnt = 0;
   for( var i = 0; i < folder.length; ++i )
   {
      if( "bin" == folder[i] || "SequoiaDB" == folder[i] || "runtest.sh" == folder[i] )
      {
         fcnt++;
      }

      if( fcnt == 3 )
      {
         break;
      }
   }

   if( fcnt >= 3 )
   {
      InstallPath = LocalPath;
   }
   else
   {
      InstallPath = "";
   }
}

/******************************************************************************
*@Description : initalize the global variable in the begninning.
                初始化全局变量LocalPath、InstallPath
******************************************************************************/
function initPath ()
{
   toolGetToolPath();
   if( InstallPath !== "" )
   {
      return;
   }

   try
   {
      var install = cmd.run( "grep INSTALL_DIR  /etc/default/sequoiadb" ).split( "=" )[1].split( "\n" );
      InstallPath = install[0];   //获得默认安装目录 /opt/sequoiadb
   }
   catch( e )
   {
      throw new Error( "failed to excute : grep INSTALL_DIR  /etc/default/sequoiadb,rc = " + e );
   }
   println( "LocalPath: " + LocalPath );
   println( "InstallPath: " + InstallPath );
}

/******************************************************************************
*@Description : check sdblobtool exists or not
*               检查sdblobtool工具是否存在
******************************************************************************/
function IsLobtoolExist ()
{
   try
   {
      var command = InstallPath + "/bin/sdblobtool -v";
      cmd.run( command );
   }
   catch( e )
   {
      if( e == 127 )
         println( ">No sdblobtool in the computer!!!" );
      else
         println( ">fail to check sdblobtool,rc = " + e );
      return false;
   }
   return true;
}

/******************************************************************************
*@Description : command for sdblobtool
*               stdlobtool命令执行语句的生成
******************************************************************************/
function toolGetCmdstr ( Args )
{
   var command = InstallPath + "/bin/sdblobtool";
   for( var k in Args )
   {
      if( k == "prefer" )
      {
         command += " --" + k + " " + Args[k];
      }
      else if( Args[k] == true )
      {
         command += " --" + k;
      }
      else if( Args[k] != false )
      {
         command += " --" + k + " " + Args[k];
      }
   }
   return command;
}

function execCommand ( cmd, command )
{
   println( "> " + command );
   try
   {
      cmd.run( command );
   } catch( e )
   {
      throw new Error( e );
   }
}

/******************************************************************************
*@Description : test lob export/import/migration with wrong parameter
*               大对象工具sdblobtool的导出导入迁移操作
******************************************************************************/
function execLobToolCommand ( Args )
{
   // 执行导出操作
   if( Args.operation === "export" )
   {
      execCommand( cmd, "rm -rf " + Args["file"] );
   }

   var command = toolGetCmdstr( Args );
   println( ">" + command );
   try
   {
      cmd.run( command );
      println( ">exec successful" );
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/******************************************************************************
*@Description : the function of make lobfile to be a lob  
                创建lobfile文件作为大对象
******************************************************************************/
function toolMakeLobfile ()
{
   var fileName = CHANGEDPREFIX + "_toolFile.txt";
   var lobfile = LocalPath + "/" + fileName;
   try
   {
      var file = new File( lobfile );
      var loopNum = 10;
      var content = null;
      for( var i = 0; i < loopNum; ++i )
      {
         content = content + i + "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
      }
      file.write( content );
      file.close();
   } catch( e )
   {
      throw new Error( e );
   }

   return lobfile;
}


/******************************************************************************
*@Description : the function of write lob to the collection with db 
                向集合cl中插入lobNum条lobfile大对象
******************************************************************************/
function toolPutLobs ( cl, lobfile, lobNum )
{
   try
   {
      var oids = [];
      for( var i = 0; i < lobNum; i++ )
      {
         oids[i] = cl.putLob( lobfile );     // putLob上传大对象成功后返回其OID
      }
      return oids;
   }
   catch( e )
   {
      println( "fail to put " + i + "th " + "lobs, rc = " + e );
      throw new Error( e );
   }
}

/******************************************************************************
*@Description : the function of check lob in the collection 
                检查集合中的lob数和oid值是否正确
******************************************************************************/
function toolCheckLob ( cl, lobnum, oids )
{
   try
   {
      var cursor = cl.listLobs();
      var i = 0;
      var failedPos = -1;
      var failedOid;
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         var oid = obj["Oid"]["$oid"]
         if( oid != oids[i] )
         {
            failedPos = i;
            failedOid = oid;
         }
         ++i;
      }
   } catch( e )
   {
      throw new Error( e );
   }

   if( failedPos !== -1 )
   {
      throw ( ">" + "expect lobnum: " + lobnum + "real lobnum:" + i + "fail to check lob oid,oid = " + failedOid + ",OID = " + OID[failedPos] );
   }

   if( i !== lobnum )
   {
      throw new Error( ">fail to check lob num,num = " + i + ",lobnum = " + lobnum );
   }

   println( ">success to check lobnum and oid" );
}

/******************************************************************************
*@Description : the function of get ErrNumber from e
                从返回的错误码中获得真实的错误参数，错误码转换
******************************************************************************/
function getErrMessage ( e )
{
   try
   {
      var errMessage = cmd.run( "grep 'shell rc: " + e + "' sdblobtool.log |tail -n 1|cut -d , -f1|cut -d ' ' -f4" ).split( "\n" );
      return errMessage[0];
   } catch( e )
   {
      throw new Error( e );
   }
}

function buildImportOrExprtArgs ( operator, clFullName, file, user, password, useSsl )
{

   if( useSsl === undefined )
   {
      var useSsl = false;
   }

   // sdblobtool 导出的选项参数
   var args = {};
   args["hostname"] = COORDHOSTNAME;   // 'localhost'
   args["svcname"] = COORDSVCNAME;     //  11810 独立模式下为50000
   args["usrname"] = user;
   args["passwd"] = password;
   args["operation"] = operator;
   args["collection"] = clFullName;
   args["file"] = file;
   args["prefer"] = "M";               //  优先选择的实例
   if( useSsl )
   {
      args["ssl"] = true;                 //  使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   }
   args["ignorefe"] = false;           //  大对象已经存在于集合中，忽略
   return args;
}

function buildMigrationArgs ( srcFullCL, dstFullCL, srcUsr, srcPwd, destUsr, destPwd, useSsl )
{
   if( useSsl === undefined )
   {
      var useSsl = false;
   }
   var args = {};
   args["hostname"] = COORDHOSTNAME;         // 'localhost'
   args["svcname"] = COORDSVCNAME;           // 11810
   args["usrname"] = srcUsr;
   args["passwd"] = srcPwd;
   args["operation"] = "migration";
   args["collection"] = srcFullCL;
   if( useSsl )
   {
      args["ssl"] = true;                      // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   }

   args["ignorefe"] = false;                 // 当前大对象已经存在于集合中，忽略这个错误
   args["dsthost"] = COORDHOSTNAME;
   args["dstservice"] = COORDSVCNAME;
   args["dstusrname"] = destUsr;
   args["dstpasswd"] = destPwd;
   args["dstcollection"] = dstFullCL;
   return args;
}

function getFileMd5 ( cmd, file )
{
   try
   {
      var output = cmd.run( "md5sum " + file ).split( " " );
   } catch( e )
   {
      throw new Error( e );
   }

   return output[0];
}

function getLob ( cl, oid, file )
{
   try
   {
      cl.getLob( oid, file );
   } catch( e )
   {
      throw new Error( e );
   }
}

function createUsr ( db, usr, pwd )
{
   try
   {
      db.createUsr( usr, pwd );
   } catch( e )
   {
      throw new Error( e );
   }
}

function dropUsr ( db, usr, pwd )
{
   try
   {
      db.dropUsr( usr, pwd );
   } catch( e )
   {
      throw new Error( e );
   }
}

function handleToolError ( args, errcode )
{
   try
   {
      execLobToolCommand( args );
      throw "expect:" + errcode + "real 0 ";
   }
   catch( e )
   {
      var errNumber = getErrMessage( e.message );
      if( errNumber == errcode )
      {
         println( ">success to test export with illegal usrname." );
      }
      else
      {
         println( ">fail to test export with illegal usrname. rc = " + errNumber );
         throw new Error( errNumber );
      }
   }
}

function LobToolTest ()
{

}

LobToolTest.prototype.checkEnv = function( paras )
{
   if( paras !== undefined && paras.user !== undefined && paras.passwd != undefined )
   {
      if( commIsStandalone( db ) )
      { // 独立模式下不能创建用户，直接跳过
         return false;
      }
      createUsr( db, paras.user, paras.passwd );
      db = new Sdb( hostName, coordPort, paras.user, paras.passwd );
   }

   initPath();

   // 检验是否存在sdblobtool工具
   return IsLobtoolExist();
}

LobToolTest.prototype.setUp = function( paras )
{

}

LobToolTest.prototype.test = function()
{

}

LobToolTest.prototype.checkResult = function()
{
   // 获得getLob文件的Md5值，检验导入是否成功
   this.tempfile = LocalPath + "/_temp.file";
   for( var i = 0; i < this.oids.length; ++i )
   {
      execCommand( cmd, "rm -rf " + this.tempfile );
      getLob( this.otherCl, this.oids[i], this.tempfile );
      var rMd5 = getFileMd5( cmd, this.tempfile );
      if( rMd5 != this.wMd5 )
      {
         throw ( ">putlob file have md5: " + this.wMd5 +
            " not equal to getlob file md5: " + rMd5 );
      }
      else
         println( ">success to import " + i + "th lob." );
   }

   // 检验导入大对象的条数是否正确
   //toolCheckLob(this.impCl, lobNum, oids);
   //println(">success to test eximport Normal Condition.\n\n");
}

LobToolTest.prototype.tearDown = function( paras )
{
   if( this.lobfile !== undefined )
   {
      execCommand( cmd, "rm -rf " + this.lobfile );
   }

   if( this.tempfile !== undefined )
   {
      execCommand( cmd, "rm -rf " + this.tempfile );
   }
   println( "####" + this.otherCLName );
   if( this.otherCLName !== undefined )
   {
      commDropCL( db, COMMCSNAME, this.otherCLName, true, true, "clean collection in the end." );
   }
   if( paras !== undefined && paras.user !== null && paras.passwd != null )
   {
      dropUsr( db, paras.user, paras.passwd );
   }
}

function lobToolMain ( main, paras )
{
   println( "call lobToolMain" );
   try
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "clean collection in the beginning" );

      main( db, paras );
      execCommand( cmd, "rm -rf sdblobtool.log" );
   }
   catch( e )
   {
      execCommand( cmd, "mkdir -p /tmp/lobtool" );
      execCommand( cmd, "mv sdblobtool.log /tmp/lobtool/migrationSSL.log" );

      if( e.constructor === Error )
      {
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "clean collection in the end, wrong" );

      db.close();
   }
}
