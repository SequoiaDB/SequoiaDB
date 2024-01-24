import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

// 获取本地主机和远程主机
var localhost = toolGetLocalhost();
var remotehost = toolGetRemotehost();

var localSystem = new SystemTest( localhost, CMSVCNAME );
var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
var systems = [localSystem, remoteSystem];

/******************************************************************************
*@Description : check user name  in /etc/passwd
*@author      : Liang XueWang            
******************************************************************************/
function checkUser ( cmd, username )
{
   var names = cmd.run( "cat /etc/passwd | grep -w " + username + " | awk -F : '{print $1}'" ).split( "\n" );
   if( names.indexOf( username ) === -1 )
   {
      throw new Error( username + " is not exist in /etc/passwd" );
   }
}

/******************************************************************************
*@Description : check group and Group of user with id command
*@author      : Liang XueWang            
******************************************************************************/
function checkGroup ( cmd, groupname, username )
{
   var info = cmd.run( "id " + username ).split( "\n" )[0];
   if( info.indexOf( groupname ) === -1 )
   {
      throw new Error( groupname + " is not contained user: " + username );
   }
}

/******************************************************************************
*@Description : check user dir
*@author      : Liang XueWang            
******************************************************************************/
function checkDir ( cmd, dir, createDir )
{
   try
   {
      cmd.run( "ls -al " + dir );
      if( !createDir )
      {
         throw new Error( "command should be failed!" );
      }
   }
   catch( e )
   {
      if( createDir || ( !createDir && e.message != 2 ) )
      {
         throw e;
      }
   }
}

/******************************************************************************
*@Description : common function for js object System/Oma
*@auhor       : Liang XueWang
******************************************************************************/
function OmaTest ( hostName, cmSvcName, isLegalHost, isLegalSvc )
{
   if( arguments.length === 0 )
      this.oma = Oma;
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME;
   else
      this.hostname = hostName["hostname"];
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME;
   else
      this.svcname = cmSvcName;
   if( isLegalHost === undefined )
      this.islegalhost = true;
   else
      this.islegalhost = isLegalHost;
   if( isLegalSvc === undefined )
      this.islegalsvc = true;
   else
      this.islegalsvc = isLegalSvc;
   if( this.islegalhost )
   {
      var db = new Sdb( this.hostname, COORDSVCNAME );
      this.isStandalone = commIsStandalone( db );
      db.close();
   }
}

OmaTest.prototype.toString = function()
{
   if( this.oma === Oma )
      return ( "static Oma" );
   else
      return ( "OmaTest: hostname=" + this.hostname + " svcname=" + this.svcname );
}

OmaTest.prototype.testInit = function() 
{
   try
   {
      if( this.oma !== Oma )
         this.oma = new Oma( this.hostname, this.svcname );
   }
   catch( e )
   {
      if( ( !this.islegalhost || !this.islegalsvc ) && ( e.message == SDB_NETWORK || e.message == SDB_INVALIDARG ) )
         ;
      else
      {
         throw e;
      }
   }
}

function RemoteTest ( hostName, cmSvcName, isLegalHost, isLegalSvc )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME;
   else
      this.hostname = hostName["hostname"];
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME;
   else
      this.svcname = cmSvcName;
   if( isLegalHost === undefined )
      this.islegalHost = true;
   else
      this.islegalhost = isLegalHost;
   if( isLegalSvc === undefined )
      this.islegalsvc = true;
   else
      this.islegalsvc = isLegalSvc;
}

RemoteTest.prototype.toString = function()
{
   return ( "hostname=" + this.hostname + " svcname=" + this.svcname );
}

RemoteTest.prototype.testInit = function()
{
   try
   {
      this.remote = new Remote( this.hostname, this.svcname );
   }
   catch( e )
   {
      if( ( !this.islegalhost || !this.islegalsvc ) && e.message == SDB_NETWORK )
         ;
      else
      {
         throw e;
      }
   }
}

function SystemTest ( hostName, cmSvcName )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME;
   else
      this.hostname = hostName["hostname"];
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME;
   else
      this.svcname = cmSvcName;
   this.isLocal = hostName["isLocal"];
   var db = new Sdb( this.hostname, COORDSVCNAME );
   this.isStandalone = commIsStandalone( db );
   db.close();
}

SystemTest.prototype.toString = function()
{
   return ( "SystemTest: hostname=" + this.hostname + " svcname=" + this.svcname );
}

SystemTest.prototype.init = function()
{
   if( this.isLocal )
   {
      this.system = System;
      this.cmd = new Cmd();
   }
   else
   {
      this.remote = new Remote( this.hostname, this.svcname );
      this.system = this.remote.getSystem();
      this.cmd = this.remote.getCmd();
   }
}

SystemTest.prototype.release = function()
{
   if( this.remote !== undefined )
      this.remote.close();
}

function FileTest ( hostName, cmSvcName, fileName )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME;
   else
      this.hostname = hostName["hostname"];   // 主机名    
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME;
   else
      this.svcname = cmSvcName;   // 端口号  
   this.filename = fileName;      // 文件名
   this.isLocal = hostName["isLocal"];
}

FileTest.prototype.init = function()
{
   if( this.isLocal )
   {
      this.cmd = new Cmd();       // 本地cmd对象
      this.system = System;
      if( this.filename === undefined )
         this.file = File;                           // 本地File类类型
      else
      {
         try
         {
            this.file = new File( this.filename );      // 本地file对象
         }
         catch( e )
         {
            var dirmode = toolGetDirMode( File, this.filename );
            throw new Error( e.message + dirmode );
         }
      }
   }
   else
   {
      this.remote = new Remote( this.hostname, this.svcname );
      this.cmd = this.remote.getCmd();   // 远程cmd对象
      this.system = this.remote.getSystem();
      if( this.filename === undefined )
         this.file = this.remote.getFile();          // 远程File类类型
      else
      {
         try
         {
            this.file = this.remote.getFile( this.filename );  // 远程file对象
         }
         catch( e )
         {
            var dirmode = toolGetDirMode( this.remote.getFile(), this.filename );
            throw new Error( e.message + dirmode );
         }
      }
   }
}

FileTest.prototype.release = function()
{
   if( this.filename !== undefined )
      this.cmd.run( "rm -rf " + this.filename );    // 删除文件
   if( this.remote !== undefined )
   {
      this.remote.close();    // 断开连接
   }
}

FileTest.prototype.toString = function()
{
   return ( "FileTest: hostname=" + this.hostname + " svcname=" + this.svcname + " filename=" + this.filename );
}

function CmdTest ( hostName, cmSvcName )
{
   if( hostName === undefined )
      this.hostname = COORDHOSTNAME;
   else
      this.hostname = hostName["hostname"];
   if( cmSvcName === undefined )
      this.svcname = CMSVCNAME;
   else
      this.svcname = cmSvcName;
   this.isLocal = hostName["isLocal"];
}

CmdTest.prototype.toString = function()
{
   return ( "CmdTest: hostname=" + this.hostname + " svcname=" + this.svcname );
}

CmdTest.prototype.init = function()
{
   if( this.isLocal )
   {
      this.cmd = new Cmd();
   }
   else
   {
      this.remote = new Remote( this.hostname, this.svcname );
      this.cmd = this.remote.getCmd();
   }
}

CmdTest.prototype.release = function()
{
   if( this.remote !== undefined )
      this.remote.close();
}

/******************************************************************************
*@Description : check two number is approximately equal to each other or not
*@author      : Liang XueWang
******************************************************************************/
function isApproEqual ( n1, n2 )  // n1 n2 > 0
{
   var max = n1 > n2 ? n1 : n2;
   var min = ( max === n1 ) ? n2 : n1;
   return min / max >= 0.85 || max - min <= 2;
}

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang            
******************************************************************************/
function toolGetHosts ()
{
   var hosts = [];
   var k = 0;

   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( commIsStandalone( db ) )
   {
      db.close();
      return hosts;
   }

   var tmpInfo = db.listReplicaGroups().toArray();
   for( var i = 0; i < tmpInfo.length; i++ )
   {
      var tmpObj = JSON.parse( tmpInfo[i] );
      var tmpArr = tmpObj.Group;
      for( var j = 0; j < tmpArr.length; j++ )
      {
         if( hosts.indexOf( tmpArr[j].HostName ) == -1 )
            hosts[k++] = tmpArr[j].HostName;
      }
   }
   db.close();
   return hosts;
}


/******************************************************************************
*@Description : get local hostname( COORDHOSTNAME ), return obj
*               localhost means cluster local host, host of COORDHOSTNAME
*@author      : Liang XueWang            
******************************************************************************/
function toolGetLocalhost ()
{
   // get local host of cluster, with COORDHOSTNAME
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var cmd = remote.getCmd();
   var localhost = cmd.run( "hostname" ).split( "\n" )[0];
   remote.close();

   var obj = {};
   obj["hostname"] = localhost;
   obj["isLocal"] = isLocal( localhost );
   return obj;
}

/******************************************************************************
*@Description : get a remote hostname in cluster
*               if cluster has no remote host,return localhost
*               return obj
*@author      : Liang XueWang
******************************************************************************/
function toolGetRemotehost ()
{
   var hosts = toolGetHosts();
   var localhost = toolGetLocalhost();
   var remotehost = localhost["hostname"];
   for( var i = 0; i < hosts.length; i++ )
   {
      if( hosts[i] !== localhost["hostname"] )
      {
         remotehost = hosts[i];
         break;
      }
   }
   var obj = {};
   obj["hostname"] = remotehost;
   obj["isLocal"] = false;
   return obj;
}

/******************************************************************************
*@Description : check sdbom exist or not
*@author      : Liang XueWang            
******************************************************************************/
function isOmExist ( hostName, cmSvcName )
{
   var oma = new Oma( hostName, cmSvcName );
   var rc;

   var arr = oma.listNodes( { type: "om" } ).toArray();
   if( arr.length !== 0 )
      rc = true;
   else
      rc = false;

   oma.close();
   return rc;
}

/******************************************************************************
*@Description : check getOmaConfigs/getNodeConfigs result
*@author      : Liang XueWang              
******************************************************************************/
function checkResult ( info, content, func )
{
   for( var i in info )
   {
      var found = false;
      for( var j = 0; j < content.length; j++ )
      {
         content[j] = content[j].replace( / /g, "" );
         if( content[j][0] === "#" )
            continue;
         var ind = content[j].indexOf( i );
         if( ind === -1 )
            continue;
         if( content[j][ind + i.length] !== "=" )
            continue;
         found = true;
         var value1 = content[j].slice( ind + i.length + 1 ).toLowerCase();
         var value2 = info[i].toString().toLowerCase();
         assert.equal( value1, value2 );
      }
      assert.notEqual( found, false );
   }
}

/******************************************************************************
*@Description : check machine is ppc or not
*@author      : Liang XueWang            
******************************************************************************/
function isPPC ( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName );
   var cmd = remote.getCmd();
   var info;

   info = cmd.run( "uname -m" ).split( "\n" )[0];

   remote.close();
   return ( info.indexOf( "ppc" ) !== -1 );
}

/******************************************************************************
*@Description : get a idle svcname
*@author      : Liang XueWang            
******************************************************************************/
function toolGetIdleSvcName ( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName );
   var cmd = remote.getCmd();

   var svcname;
   for( svcname = RSRVPORTBEGIN; svcname <= RSRVPORTEND; svcname = svcname * 1 + 5 )
   {
      try
      {
         cmd.run( "netstat -anp | grep " + svcname );
      }
      catch( e )
      {
         if( e.message == 1 )
         {
            remote.close();
            return svcname;
         }
         throw e;
      }
   }
   remote.close();
   return svcname;
}

/******************************************************************************
*@Description : get sdbcm user
*@author      : Liang XueWang            
******************************************************************************/
function toolGetSdbcmUser ( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName );
   var cmd = remote.getCmd();
   var command = "ps aux | grep sdbcm | grep -E -v 'grep|sdbcmd' | awk '{print $1}'";
   var user = cmd.run( command ).split( "\n" )[0];
   return user;
}

/******************************************************************************
*@Description : get current user whoami
*@author      : Liang XueWang            
******************************************************************************/
function toolGetCurrentUser ( hostName, cmSvcName )
{
   var remote = new Remote( hostName, cmSvcName );
   var cmd = remote.getCmd();
   var tmp = cmd.run( "whoami" ).split( "\n" );
   var user = tmp[tmp.length - 2];
   return user;
}

/******************************************************************************
*@Description : get user and group  sdbcm.conf
*@author      : Liang XueWang            
******************************************************************************/
function toolGetCmUserGroup ( hostname, svcname )
{
   var sdbDir = toolGetSequoiadbDir( hostname, svcname );
   var file = sdbDir[0] + "/conf/sdbcm.conf";

   var remote = new Remote( hostname, svcname );
   var cmd = remote.getCmd();
   var command = "ls -l " + file + " | awk '{print $3,$4}'";
   var tmpInfo = cmd.run( command ).split( "\n" );
   var tmp = tmpInfo[tmpInfo.length - 2].split( " " );
   var result = {};
   result["user"] = tmp[0];
   result["group"] = tmp[1];
   return result;
}

/******************************************************************************
*@Description : check object is empty or not
*@author      : Liang XueWang            
******************************************************************************/
function isEmptyObject ( obj )
{
   for( var k in obj )
      return false;
   return true;
}

/******************************************************************************
*@Description : get sequoiadb dir eg: /opt/sequoiadb /opt/sequoiadb/bin/..
*@author      : Liang XueWang              
******************************************************************************/
function toolGetSequoiadbDir ( hostname, svcname )
{
   var dir = [];
   var remote = new Remote( hostname, svcname );
   var system = remote.getSystem();
   var tmp = system.getEWD();
   var ind = tmp.indexOf( "/bin" );
   dir[0] = tmp + "/..";
   dir[1] = tmp.slice( 0, ind );
   remote.close();

   if( isLocal( hostname ) )
   {
      system = System;
      tmp = system.getEWD();
      ind = tmp.indexOf( "/bin" );
      dir[2] = tmp + "/..";
      dir[3] = tmp.slice( 0, ind );
   }

   return dir;
}

/******************************************************************************
*@Description : delete user 
*@author      : Liang XueWang              
******************************************************************************/
function deleteUser ( hostname, svcname, username, system, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   var remote = new Remote( hostname, svcname );
   var cmd = remote.getCmd();
   try
   {
      cmd.run( "grep '^" + username + ":' /etc/passwd" );
      var option = { "name": username, "isRemoveDir": true };
      system.delUser( option );
   }
   catch( e )
   {
      if( !ignoreNotExist || e.message != 1 )
      {
         throw e;
      }
   }
   remote.close();
}

/******************************************************************************
*@Description : delete group
*@author      : Liang XueWang              
******************************************************************************/
function deleteGroup ( hostname, svcname, groupname, system, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   var remote = new Remote( hostname, svcname );
   var cmd = remote.getCmd();
   try
   {
      cmd.run( "grep '^" + groupname + ":' /etc/group" );
      system.delGroup( groupname );
   }
   catch( e )
   {
      if( !ignoreNotExist || e.message != 1 )
      {
         throw e;
      }
   }
   remote.close();
}

/******************************************************************************
*@Description : check group exist or not
*@author      : Liang XueWang              
******************************************************************************/
function toolGetDirMode ( f, filename )
{
   var ind = filename.lastIndexOf( "/" );
   var dir = filename.slice( 0, ind );
   var mode = f.stat( dir ).toObj().mode;
   return mode;
}

/******************************************************************************
*@Description : check host is local or not
*@author      : Liang XueWang              
******************************************************************************/
function isLocal ( hostname )
{
   var cmd = new Cmd();
   var localhostname = cmd.run( "hostname" ).split( "\n" )[0];
   if( hostname === "localhost" || hostname === localhostname )
      return true;
   else
      return false;
}

/*******************************************************************
* @Description : check path has / in the end or not
*                add / if not
* @author      : Liang XueWang
*
********************************************************************/
function adaptPath ( path )
{
   if( path.lastIndexOf( '/' ) !== path.length - 1 )
      path += '/';
   return path;
}

/***********************************************************
*@Description : common function for import importOnce
*@auhor       : Liang XueWang
***********************************************************/
commMakeDir( COORDHOSTNAME, WORKDIR );
// js file without return
var withoutRetFile = WORKDIR + "/withoutRet_11902.js";
// js file with return
var withRetFile = WORKDIR + "/withRet_11903.js";

function createWithoutRetFile ()
{
   var file = new File( withoutRetFile );
   file.write( "function add( a, b ) { return a + b ; }" );
   file.close();
}
function createWithRetFile ()
{
   file = new File( withRetFile );
   file.write( "function mul( a, b ) { return a * b ; } var tmp = 100 ; mul( 1, 2 ) ;"
      + " mul( 2, 3 ) ;" );
   file.close();
}

function removeFile ( filename )
{
   File.remove( filename );
}
function currUser ()
{
   var cmd = new Cmd();
   var tmp = cmd.run( "whoami" ).split( "\n" );
   var user = tmp[tmp.length - 2];
   return user;
}

function getCoordUser ()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var system = remote.getSystem();
   var cursor = system.listProcess( { detail: true },
      { cmd: "sequoiadb(" + COORDSVCNAME + ") S" } );
   var user = cursor.next().toObj()["user"];
   remote.close();
   return user;
}

function initWorkDir ( cmd, remote )
{
   // localhost
   try
   {
      cmd.run( "ls " + WORKDIR );
   }
   catch( e )
   {
      if( 2 == e.message )   // 2: No such file or directory
      {
         cmd.run( "mkdir -p " + WORKDIR );
      }
      else
      {
         throw e;
      }
   }

   // remote host
   var file = remote.getFile();
   var dirExist = file.exist( WORKDIR );
   if( false === dirExist )
   {
      commMakeDir( COORDHOSTNAME, WORKDIR );
   }
}
/******************************************************************************
*@Description : used to get sdbcm.conf user and group
*@author      : Liang XueWang              
******************************************************************************/
function getFileUsrGrp ( file )
{
   var tmpObj = File.stat( file ).toObj();
   var obj = {};
   obj["user"] = tmpObj.user;
   obj["group"] = tmpObj.group;
   return obj;
}