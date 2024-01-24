/*******************************************************************************
*@Description : Backup and restore common functions
*@Modify list :
*               2014-3-16  Jianhui Xu  Init
*               2014-6-20  xiaojun Hu  Change
*               2018-1-10  wenjing Wang Change
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var csName = COMMCSNAME;
var clName = COMMCLNAME;
var backupandrestoreGroup = 'backup_restore';
var logSourcePaths = [];

function isPortUsed ( port )
{
   try
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
      cmd.run( "lsof -nP -iTCP:" + port + " -sTCP:LISTEN" );
      return true;
   } catch( e )
   {
      return false;
   }
}

function getLocalHostName ()
{
   var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   return cmd.run( 'hostname' ).split( '\n' )[0];
}

function createBackupRestoreGroup ( db, hosts )
{
   var rg = db.createRG( backupandrestoreGroup );
   var port = parseInt( RSRVPORTBEGIN );
   var hostname = getLocalHostName();
   for( var i = 0; i < 3; ++i )
   {
      while( isPortUsed( port ) )
      {
         port += 10;
      }
      var dataPath = RSRVNODEDIR + port;
      rg.createNode( hostname, port, dataPath, { diaglevel: 5 } );
      logSourcePaths.push( hostname + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log" );
      port += 10;
   }
   rg.start();

   var totalLen = 12000;
   var alreadyWait = 0;
   do
   {
      var group = db.getRG( backupandrestoreGroup ).getDetail().next().toObj();
      if( group.PrimaryNode !== undefined )
      {
         break;
      } else
      {
         sleep( 10 );
         alreadyWait += 1;
      }
      if( alreadyWait > totalLen )
      {
         throw new Error( "wait select primary timeout" );
      }
   } while( true );

}

function nodeInfo ( groupName, hostName, svcName, dbPath )
{
   this.hostName = hostName;
   this.svcName = svcName;
   this.groupName = groupName;
   this.dbPath = dbPath;
}

function backUpInfo ( bakName, bakPath, bakBeginIncId, bakEndIncId )
{
   this.bakName = bakName;
   this.bakPath = bakPath;
   if( bakBeginIncId === undefined )
   {
      this.bakBeginIncId = 0;
   }
   else
   {
      this.bakBeginIncId = bakBeginIncId;
   }
   if( bakEndIncId === undefined )
   {
      this.bakEndIncId = -1;
   }
   else
   {
      this.bakEndIncId = bakEndIncId;
   }
}

/* *****************************************************************************
@Description : insert data
@author: Jianhui Xu
@parameter:
   cl : collection object
@change :
          2014-6-20  xiaojun Hu
***************************************************************************** */
function bakInsertData ( cl, docs )
{
   if( typeof ( docs ) !== "object" )
   {
      var docs = [];

      docs.push( { no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } } );
      docs.push( { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } } );
      docs.push( { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } } );
      docs.push( { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } } );
      docs.push( { no: 1004, score: 69, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } } );
      docs.push( { no: 1005, score: 70, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } } );
      docs.push( { no: 1006, score: 84, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } } );
      docs.push( { no: 1007, score: 73, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } } );
      docs.push( { no: 1008, score: 72, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } } );
      docs.push( { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } } );
      docs.push( { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } } );
      docs.push( { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } } );
      docs.push( { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } } );
      docs.push( { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } } );
      docs.push( { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } } );
      docs.push( { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } } );
      docs.push( { no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } } );
   }

   cl.insert( docs );
   return docs;
}

/* *****************************************************************************
@Description: remove backups data
@author: Jianhui Xu
@parameter:
   filter : backup name filter, default is ""
   ignoreNotExist : true/false, default is true
   path : backup path, default is ""
   isSubDir : true/false, default is false
***************************************************************************** */
function bakRemoveBackups ( db, filter, ignoreNotExist, path, isSubDir )
{
   if( filter == undefined ) { filter = ""; }
   if( ignoreNotExist == undefined ) { ignoreNotExist = true; }
   if( path == undefined ) { path = ""; }
   if( isSubDir == undefined ) { isSubDir = false; }

   var backups = commGetBackups( db, filter, path, isSubDir );
   for( var i = 0; i < backups.length; ++i )
   {
      try
      {
         if( path.length != 0 )
         {
            db.removeBackup( { Name: backups[i], Path: path } );
         }
         else
         {
            db.removeBackup( { Name: backups[i] } );
         }
      }
      catch( e )
      {
         // not exist
         if( !ignoreNotExist || e.message != SDB_BAR_BACKUP_NOTEXIST )
         {
            throw e;
         }
      }
   }
}

/* *****************************************************************************
@Description: backup offline
@author: Jianhui Xu
@parameter:
   backupObj : backup object
***************************************************************************** */
function bakBackup ( db, backupObj )
{
   if( backupObj == undefined ) { backupObj = {}; }

   if( typeof ( backupObj ) != "object" )
   {
      throw new Error( "bakBackup: backupObj is not object" );
   }
   db.backup( backupObj );
}

function getDateString ()
{
   var d = new Date();
   return d.getFullYear() + '-'
      + ( d.getMonth() + 1 ) + '-'
      + d.getDate() + '-'
      + ( d.getHours() + 1 ) + ':'
      + d.getMinutes() + ':'
      + d.getSeconds();
}

function checkBackupInfo ( db, errDesc, bakName, path, alreadStart, opt )
{
   var backups = commGetBackups( db, bakName, path, alreadStart, opt );
   println( "backup file = " + JSON.stringify( backups ) );
   if( !commIsStandalone( db ) && bakName === undefined )
   {
      if( 0 === backups.length )
      {
         println( JSON.stringify( backups, "", 3 ) );
         throw new Error( "check backup failed" );
      }
   } else if( 1 != backups.length )
   {

      println( JSON.stringify( backups, "", 3 ) );
      throw new Error( "check backup failed" );
   }
   return backups[0];

}

function getExecPath ( cmd )
{
   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   var path = "";
   try
   {
      cmd.run( "svn info" );
      path = cmd.run( "pwd" ).split( "\n" )[0];
   } catch( e )
   {
      var content = cmd.run( "cat /etc/default/sequoiadb|grep INSTALL_DIR" ).split( "\n" )[0]
      var pair = content.split( "=" );
      if( pair.length == 2 )
      {
         path = pair[1];
      }
   }

   if( path === undefined )
   {
      throw new Error( "execPath is invalid" );
   }

   var suffix = "/bin";
   var pos = path.lastIndexOf( suffix );
   if( pos === -1 || pos + suffix.length !== path.length )
   {
      path += "/bin/";
   }

   return path;
}

function genFile ( path )
{
   if( path === undefined )
   {
      var path = WORKDIR + "testdat" + Math.floor( Math.random() * 100 );
   }

   var content = "";
   for( var i = 0; i < 1024 + Math.floor( Math.random() * 4096 ); ++i )
   {
      var code = Math.floor( Math.random() * 127 );
      if( code < 20 )
      {
         code = code + 20;
      }
      content += String.fromCharCode( code );
   }

   var file = new File( path );
   file.write( content );
   file.close();
}

function calcMD5 ( cmd, path )
{
   if( path === undefined )
   {
      throw new Error( "path is invalid" );
   }

   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   var output = File.md5( path );
   output = output.split( "\n" )[0];
   var detail = output.split( " " );
   if( detail.length == 2 )
   {
      var MD5 = detail[0];
   }

   return MD5;
}

function sdbPutLob ( cl, filePath )
{
   var oid = cl.putLob( filePath );
   return oid;
}

function isTheSameMachine ( cmd, hostName )
{
   if( hostName === undefined )
   {
      throw new Error( "invalid parameters" );
   }

   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   if( hostName === "localhost" || hostName === "127.0.0.1" )
   {
      return true;
   }

   var curHostName = cmd.run( "hostname" ).split( "\n" )[0];
   if( curHostName === hostName )
   {
      return true;
   }
   else
   {
      return false;
   }
}

function getCmdByHostName ( cmd, hostName )
{
   if( !isTheSameMachine( cmd, hostName ) )
   {
      var remote = new Remote( hostName, CMSVCNAME );
      var cmd = remote.getCmd();
   }

   return cmd;
}

function sdbRestore ( db, cmd, bakInfo, node )
{
   if( node !== undefined && !( node instanceof nodeInfo ) )
   {
      throw new Error( " invalid parameters " );
   }

   if( !( bakInfo instanceof backUpInfo ) )
   {
      throw new Error( " invalid parameters " );
   }
   var isStandalone = false;
   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   if( commIsStandalone( db ) )
   {
      var isStandalone = true;
   }

   stopNode( db, isStandalone, cmd, node );
   var execProg = getExecPath( cmd ) + "sdbrestore";
   var output = cmd.run( execProg + " --bkname " + bakInfo.bakName + " --bkpath " + bakInfo.bakPath +
      " -b " + bakInfo.bakBeginIncId + " -i " + bakInfo.bakEndIncId );
   startNode( db, isStandalone, cmd, node );
}

function removeFile ( cmd, filePath )
{
   if( filePath === undefined )
   {
      throw new Error( "invalid parameters" );
   }

   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   var output = cmd.run( "rm -rf " + filePath );
}

function stopNode ( db, isStandalone, cmd, node )
{
   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   if( isStandalone )
   {
      var execProg = getExecPath( cmd ) + "sdbstop";
      cmd.run( execProg + " -p " + COORDSVCNAME );
   }
   else
   {
      if( !( node instanceof nodeInfo ) )
      {
         throw new Error( " invalid parameters " );
      }
      db.getRG( node.groupName ).getNode( node.hostName, node.svcName ).stop();
   }
}

function startNode ( db, isStandalone, cmd, node )
{
   if( cmd === undefined )
   {
      var cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   if( isStandalone )
   {
      var execProg = getExecPath( cmd ) + "sdbstart";
      cmd.run( execProg + " -p " + COORDSVCNAME );
   }
   else
   {
      if( !( node instanceof nodeInfo ) )
      {
         throw new Error( " invalid parameters " );
      }

      db.getRG( node.groupName ).getNode( node.hostName, node.svcName ).start();
   }
}

function IsBakPathEmpty ( cmd, bakPath )
{
   if( bakPath === undefined )
   {
      throw new Error( " parameter's invalid" );
   }

   if( cmd === undefined )
   {
      cmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
   }

   try
   {
      var output = cmd.run( " ls -A " + bakPath );
      if( output === "" )
      {
         return true;
      }
      else
      {
         throw new Error( "expect second item get a number" );
      }
   } catch( e )
   {
   }
}

function compareObj ( lobj, robj, ignoreId )
{
   if( typeof ( lobj ) === "object" &&
      typeof ( robj ) === "object" )
   {
      if( lobj == null && robj == null ) return true;
      if( lobj == null || robj == null || lobj.constructor !== robj.constructor ) return false;
      var _idNum = 1;
      var lkeys = Object.getOwnPropertyNames( lobj );
      var rkeys = Object.getOwnPropertyNames( robj );
      if( ignoreId &&
         ( lkeys.length !== rkeys.length + _idNum &&
            lkeys.length + _idNum !== rkeys.length ) ) 
      {
         return false;
      }

      for( key in lobj )
      {
         if( ignoreId && key === "_id" ) 
         {
            continue
         }

         if( !compareObj( lobj[key], robj[key] ) ) 
         {
            return false;
         }
      }
      return true;
   }
   else if( lobj === robj )
   {
      return true;
   }
   else
   {
      return false;
   }
}

function getBackups ( db, filter, grpNameArray )
{
   var backUp = {};
   if( typeof ( grpNameArray ) === "object" )
   {
      var cursor = db.listBackup( { GroupName: grpNameArray } );
   }
   else
   {
      var cursor = db.listBackup();
   }
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( filter !== undefined && 0 !== obj.Name.indexOf( filter ) )
      {
         continue;
      }

      var key = obj.GroupName + ":" + obj.ID;
      backUp[key] = obj;
   }

   return backUp;
}

function getAllHosts ( groups )
{
   var hosts = [];
   var host2Count = {};
   for( var i = 0; i < groups.length; ++i )
   {
      for( var j = 1; j < groups[i].length; ++j )
      {
         if( host2Count[groups[i][j].HostName] === undefined )
         {
            hosts.push( groups[i][j].HostName );
            host2Count[groups[i][j].HostName] = 1;
         }
      }
   }
   return hosts;
}

function backupTestCase ( sdb )
{
   this.sdb = sdb;
   this.db = db;
   this.oids = [];
   this.localCmd = new Remote( COORDHOSTNAME, CMSVCNAME ).getCmd();
}

backupTestCase.prototype.csName = csName;
backupTestCase.prototype.clName = clName;
backupTestCase.prototype.init =
   function()
   {
      if( !commIsStandalone( db ) )
      {
         this.groups = commGetGroups( db );
         var hosts = getAllHosts( this.groups );
         createBackupRestoreGroup( db, hosts );
         this.group = db.getRG( backupandrestoreGroup ).getDetail().next().toObj();
         var primaryPos = this.group.PrimaryNode;

         for( var i = 0; i < this.group.Group.length; ++i )
         {
            if( primaryPos == this.group.Group[i].NodeID )
            {
               var hostName = this.group.Group[i].HostName;
               var svcName = this.group.Group[i].Service[0].Name;
               var dbPath = this.group.Group[i].dbpath;
            }
         }
         this.nodeinfo = new nodeInfo( this.group.GroupName, hostName, svcName, dbPath );

         this.db = new Sdb( hostName, svcName );
         var opt = { Group: this.group.GroupName, ReplSize: -1 }
         this.cl = commCreateCL( this.sdb, this.csName, this.clName, opt, true, false );
         this.cmd = getCmdByHostName( this.localCmd, hostName );
      }
      else
      {
         this.cl = commCreateCL( this.sdb, this.csName, this.clName, { ReplSize: -1 }, true, false );
         this.cmd = getCmdByHostName( this.localCmd, COORDHOSTNAME );
      }
      return true;
   }

backupTestCase.prototype.setUp =
   function()
   {
      commDropCL( db, this.csName, this.clName, true, true, "Drop CL in the beginning" );
      bakRemoveBackups( db, CHANGEDPREFIX, true );
      var flag = false;
      try
      {
         db.getRG( backupandrestoreGroup );
         flag = true;
         db.removeRG( backupandrestoreGroup );
      } catch( e )
      {
         if( flag ) throw e;
      }

      return this.init();
   }

backupTestCase.prototype.reInit = function() { }

backupTestCase.prototype.checkResult =
   function( times )
   {
      if( times === undefined )
      {
         var times = 1;
      }

      this.reInit();
      this.cl = this.sdb.getCS( this.csName ).getCL( this.clName );
      var cursor = this.cl.find().sort( { no: 1 } );
      var index = 0;
      var cnt = 0;
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         if( !compareObj( obj, this.docs[index], true ) )
         {
            throw new Error( "expect doc:" + JSON.stringify( this.docs[index] ) + "real doc:" + JSON.stringify( obj ) );
         }

         if( ++cnt === times )
         {
            ++index;
            cnt = 0;
         }
      }

      try
      {
         var path = WORKDIR + "getdat" + getDateString();
         for( var i = 0; i < this.oids.length; ++i )
         {
            this.cl.getLob( this.oids[i], path, true );
            var curMD5 = calcMD5( this.localCmd, path );
            if( this.originMD5 !== curMD5 )
            {
               throw new Error( "expect md5:" + this.originMD5 + "real md5:" + curMD5 );
            }
         }
      }
      finally
      {
         removeFile( this.localCmd, path );
      }
   }

backupTestCase.prototype.checkBackupRes =
   function( bakInfo, times, grpNameArray )
   {
      if( times === undefined )
      {
         var times = 1;
      }
      var groupNum = grpNameArray === undefined ? 1 : grpNameArray.length;
      var backUp = getBackups( this.db, bakInfo.bakName, grpNameArray );
      var props = Object.getOwnPropertyNames( backUp );
      if( props.length !== times * groupNum )
      {
         var cond = grpNameArray === undefined ? "" : "{Group:[" + JSON.stringify( grpNameArray ) + "]}";
         throw new Error( "expect listBack(" + cond + ") return " + ( times * groupNum ) + " ,real return " +
            props.length + " item " );
      }

      if( times <= 1 )
      {
         return;
      }

      for( var k = 0; k < groupNum; ++k )
      {
         for( var i = 1; i < times; ++i )
         {
            if( grpNameArray !== undefined )
            {
               var key = grpNameArray[k];
               key += ":";
               key += i;
            }
            else
            {
               var key = i;
            }
            var obj = backUp[key];
            if( obj.EnsureInc !== true )
            {
               throw new Error( " expect EnsureInc=true, real EnsureInc=" + obj.EnsureInc );
            }
         }
      }
   }

backupTestCase.prototype.execTest = function( backupName, path ) { }

backupTestCase.prototype.test =
   function()
   {
      try
      {
         var backupName = CHANGEDPREFIX + getDateString();
         var path = WORKDIR + "testdat" + getDateString();
         genFile( path );
         this.originMD5 = calcMD5( this.localCmd, path );
         this.execTest( backupName, path );
      }
      finally
      {
         if( this.group !== undefined )
         {
            this.addNodeExceptPrimary();
         }
         removeFile( this.localCmd, path );
      }
   }

backupTestCase.prototype.removeNodeExceptPrimary =
   function()
   {
      this.group = this.sdb.getRG( backupandrestoreGroup ).getDetail().next().toObj();
      for( var i = 0; i < this.group.Group.length; ++i )
      {
         var hostName = this.group.Group[i].HostName;
         var svcName = this.group.Group[i].Service[0].Name;
         if( this.group.PrimaryNode !== this.group.Group[i].NodeID )
         {
            try
            {
               this.sdb.getRG( this.group.GroupName ).removeNode( hostName, svcName );
            } catch( e )
            {
               println( "removeNodeExceptPrimary" + hostName + ":" + svcName );
            }
         }
      }
   }

backupTestCase.prototype.addNodeExceptPrimary =
   function()
   {
      for( var i = 0; i < this.group.Group.length; ++i )
      {
         var hostName = this.group.Group[i].HostName;
         var svcName = this.group.Group[i].Service[0].Name;
         var dbPath = this.group.Group[i].dbpath;
         if( this.group.PrimaryNode != this.group.Group[i].NodeID )
         {
            try
            {
               this.sdb.getRG( this.group.GroupName ).createNode( hostName, parseInt( svcName ), dbPath );
               this.sdb.getRG( this.group.GroupName ).start();
            } catch( e )
            {
               if( e.message != SDBCM_NODE_EXISTED )
               {
                  println( "createNode(" + hostName + "," + svcName + "," + dbPath + " ),err" + e );
               }
            }
         }
      }
      db.getRG( this.group.GroupName ).start();
      var totalTimeLen = 60;
      var alreadySleepTime = 0;
      while( true )
      {
         var errGroups = commCheckBusiness( this.groups, true );
         if( errGroups.length == 0 )
         {
            break;
         }

         // 检查所有组是否都是空组
         var i = 0;
         for( ; i < errGroups.length; ++i )
         {
            if( errGroups[i].length != 1 )
            {
               sleep( 1000 );
               alreadySleepTime += 1;
               break;
            }
         }

         if( i == errGroups.length || alreadySleepTime >= totalTimeLen )
         {
            break;
         }
      }
   }

backupTestCase.prototype.tearDown =
   function tearDown ()
   {
      bakRemoveBackups( this.db, CHANGEDPREFIX, true );
      commDropCL( this.sdb, this.csName, this.clName, true, false, "Drop CL in the end" );
   }
