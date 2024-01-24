/*******************************************************************************
 * @Description :  common function for test node configs
 * @author      :  Liang XueWang               
 *******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


function ConfDesp ( name, type, defaultVal, validVal, invalidVal )
{
   this.name = name;
   this.type = type;
   this.defaultVal = defaultVal;
   this.validVal = validVal;
   this.invalidVal = invalidVal;
}

ConfDesp.prototype.toString = function()
{
   return ( "conf: " + this.name + " type: " + this.type +
      " default value: " + this.defaultVal );
}

var Configs = ( function()
{
   var instance;
   var Configs = function()
   {
      if( instance !== undefined ) return instance;
      this.runConfigs = [];
      this.rebootConfigs = [];
      this.forbidConfigs = [];
      this.unknowConfigs = [];
      this.init();
      return instance = this;
   };

   Configs.prototype.init = function()
   {
      // register run configs
      this.runConfigs.push( new ConfDesp( "diagnum", "int", 20, 10, "abc" ) );
      this.runConfigs.push( new ConfDesp( "auditnum", "int", 20, 10, "abc" ) );
      this.runConfigs.push( new ConfDesp( "maxpool", "int", 50, 100, "123" ) );
      this.runConfigs.push( new ConfDesp( "diaglevel", "short", 3, 5, "abc" ) );
      this.runConfigs.push( new ConfDesp( "maxreplsync", "int", 10, 12, "abc" ) );
      this.runConfigs.push( new ConfDesp( "sortbuf", "int", 256, 128, null ) );
      this.runConfigs.push( new ConfDesp( "hjbuf", "int", 128, 256, "abc" ) );
      this.runConfigs.push( new ConfDesp( "sharingbreak", "int", 7000, 10000, "xxxx" ) );
      this.runConfigs.push( new ConfDesp( "indexscanstep", "int", 100, 200, "what" ) );
      this.runConfigs.push( new ConfDesp( "startshifttime", "int", 600, 1200, "some" ) );
      this.runConfigs.push( new ConfDesp( "preferedstrict", "bool", "FALSE", "TRUE", 123 ) );
      //this.runConfigs.push( new ConfDesp( "directioinlob", "bool", "FALSE", "TRUE", "1234" ) ) ;新建集合空间在线生效
      this.runConfigs.push( new ConfDesp( "sparsefile", "bool", "TRUE", "FALSE", null ) );
      this.runConfigs.push( new ConfDesp( "weight", "int", 10, 20, "asdafd" ) );
      //this.runConfigs.push( new ConfDesp( "usessl", "bool", "FALSE", "TRUE", "1234" ) ) ;新连接生效
      this.runConfigs.push( new ConfDesp( "auth", "bool", "TRUE", "FALSE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "planbuckets", "int", 500, 200, "125" ) );
      this.runConfigs.push( new ConfDesp( "optimeout", "int", 60000, 100000, "20" ) );
      this.runConfigs.push( new ConfDesp( "overflowratio", "int", 12, 22, "11" ) );
      this.runConfigs.push( new ConfDesp( "extendthreshold", "int", 32, 64, "22" ) );
      this.runConfigs.push( new ConfDesp( "signalinterval", "int", 0, 20, "20" ) );
      this.runConfigs.push( new ConfDesp( "maxcachesize", "int", 0, 128, "xxxx" ) );
      this.runConfigs.push( new ConfDesp( "maxcachejob", "int", 10, 20, "####" ) );
      this.runConfigs.push( new ConfDesp( "cachemergesz", "int", 0, 20, "sdfff" ) );
      this.runConfigs.push( new ConfDesp( "pagealloctimeout", "int", 0, 125, "fafrrr" ) );
      this.runConfigs.push( new ConfDesp( "maxsyncjob", "int", 10, 20, "gghttr" ) );
      this.runConfigs.push( new ConfDesp( "syncinterval", "int", 10000, 20000, "sadafe" ) );
      this.runConfigs.push( new ConfDesp( "syncrecordnum", "int", 0, 1000, "ssss" ) );
      this.runConfigs.push( new ConfDesp( "syncdeep", "bool", "FALSE", "TRUE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "archivecompresson", "bool", "TRUE", "FALSE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "archivetimeout", "int", 600, 300, "1213" ) );
      this.runConfigs.push( new ConfDesp( "archiveexpired", "int", 240, 120, "no" ) );
      this.runConfigs.push( new ConfDesp( "archivequota", "int", 10, 20, "why" ) );
      this.runConfigs.push( new ConfDesp( "dataerrorop", "int", 1, 2, "I" ) );
      this.runConfigs.push( new ConfDesp( "dmschkinterval", "int", 0, 120, "O" ) );
      //this.runConfigs.push( new ConfDesp( "perfstat", "bool", "FALSE", "TRUE", "1234" ) ) ;//数据库配置资料中已没有此参数
      this.runConfigs.push( new ConfDesp( "optcostthreshold", "int", 20, 10, "M" ) );
      this.runConfigs.push( new ConfDesp( "maxconn", "int", 0, 3000, "12345" ) );
      //this.runConfigs.push( new ConfDesp( "enablemixcmp", "bool", "FALSE", "TRUE", "1234" ) ) ;//数据库配置资料中已没有此参数
      this.runConfigs.push( new ConfDesp( "plancachelevel", "int", 3, 4, "88" ) );
      this.runConfigs.push( new ConfDesp( "memdebug", "bool", "FALSE", "TRUE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "memdebugsize", "int", 0, 256, "WEYEH" ) );
      this.runConfigs.push( new ConfDesp( "memdebugverify", "bool", "FALSE", "TRUE", "1234" ) );
      // this.runConfigs.push( new ConfDesp( "svcmaxconcurrency", "int", 100, 150, "1234" ) );当'svcscheduler'取值为0时，该参数不生效
      this.runConfigs.push( new ConfDesp( "transactiontimeout", "int", 60, 100, "23" ) );
      this.runConfigs.push( new ConfDesp( "transisolation", "int", 0, 1, "1234" ) );
      this.runConfigs.push( new ConfDesp( "translockwait", "bool", "FALSE", "TRUE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "transautocommit", "bool", "FALSE", "TRUE", "1234" ) );
      //this.runConfigs.push( new ConfDesp( "transautorollback", "bool", "TRUE", "FALSE", "1234" ) );当开启事务时才生效
      this.runConfigs.push( new ConfDesp( "transuserbs", "bool", "TRUE", "FALSE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "transrccount", "bool", "TRUE", "FALSE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "logtimeon", "bool", "FALSE", "TRUE", "1234" ) );
      this.runConfigs.push( new ConfDesp( "maxsocketpernode", "int", 5, 6, "1234" ) );
      this.runConfigs.push( new ConfDesp( "maxsocketperthread", "int", 1, 5, "1234" ) );
      this.runConfigs.push( new ConfDesp( "maxsocketthread", "int", 10, 12, "1234" ) );
      //this.runConfigs.push( new ConfDesp( "monslowquerythreshold", "int", 200, 300, "1234" ) );单号：
      //this.runConfigs.push( new ConfDesp( "mongroupmask", "string", "", "", 1234 ) );
      //SEQUOIADBMAINSTREAM-4809 修改为非法值时，取默认值，以下项无法测试非法值，故注释
      //this.runConfigs.push( new ConfDesp( "preferedinstance", "string", "M", "S", 123 ) );
      //this.runConfigs.push( new ConfDesp( "preferedinstancemode", "string", "random", "ordered", 123 ) );
      //this.runConfigs.push( new ConfDesp( "syncstrategy", "string", "KeepNormal", "KeepAll", 1111 ) );
      //this.runConfigs.push( new ConfDesp( "auditmask", "string", "SYSTEM|DDL|DCL", "SYSTEM", 122 ) );
      //this.runConfigs.push( new ConfDesp( "logwritemod", "string", "increment", "full", 1234 ) );

      // register reboot configs
      this.rebootConfigs.push( new ConfDesp( "transactionon", "bool", "TRUE", "FALSE", "1234" ) );
      this.rebootConfigs.push( new ConfDesp( "numpreload", "int", 0, 10, "and" ) );
      this.rebootConfigs.push( new ConfDesp( "maxprefpool", "int", 0, 10, "or" ) ); //TODO: affect maxsubquery
      this.rebootConfigs.push( new ConfDesp( "logbuffsize", "int", 1024, 2048, "Q" ) );
      this.rebootConfigs.push( new ConfDesp( "replbucketsize", "int", 32, 64, "test" ) );
      this.rebootConfigs.push( new ConfDesp( "dpslocal", "bool", "FALSE", "TRUE", "1234" ) );
      this.rebootConfigs.push( new ConfDesp( "traceon", "bool", "FALSE", "TRUE", 12345 ) );
      this.rebootConfigs.push( new ConfDesp( "tracebufsz", "int", 256, 512, "lxw" ) );
      this.rebootConfigs.push( new ConfDesp( "archiveon", "bool", "FALSE", "TRUE", 54321 ) );
      this.rebootConfigs.push( new ConfDesp( "instanceid", "int", 0, 1, "aassada" ) );

      // register forbid configs
      this.forbidConfigs.push( new ConfDesp( "dbpath", "path", "./", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "indexpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "confpath", "path", "./", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "logpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "bkuppath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "svcname", "string", "11810", "50000", null ) );
      this.forbidConfigs.push( new ConfDesp( "replname", "string", "", "51000", null ) );
      this.forbidConfigs.push( new ConfDesp( "shardname", "string", "", "52000", null ) );
      this.forbidConfigs.push( new ConfDesp( "catalogname", "string", "", "53000", null ) );
      this.forbidConfigs.push( new ConfDesp( "httpname", "string", "", "54000", null ) );
      this.forbidConfigs.push( new ConfDesp( "role", "string", "standalone", "data", "person" ) );
      this.forbidConfigs.push( new ConfDesp( "catalogaddr", "string", "", "hdgfdkj:57000", null ) );
      this.forbidConfigs.push( new ConfDesp( "logfilesz", "int", 64, 128, "10" ) );
      this.forbidConfigs.push( new ConfDesp( "logfilenum", "int", 20, 10, "128" ) );
      this.forbidConfigs.push( new ConfDesp( "lobpath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "lobmetapath", "path", "", "helloworld", null ) );
      this.forbidConfigs.push( new ConfDesp( "omaddr", "string", "", "helloworld:54345", null ) );
      this.forbidConfigs.push( new ConfDesp( "archivepath", "path", "", "helloworld", null ) );

      // register unknown configs
      this.unknowConfigs.push( new ConfDesp( "cataloglist", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "clustername", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "businessname", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "usertag", "string", "", "", null ) );
      this.unknowConfigs.push( new ConfDesp( "fap", "string", "fapmongo", "fapmongo", null ) );
      this.unknowConfigs.push( new ConfDesp( "arbiter", "bool", "FALSE", "TRUE", null ) );
   };
   return Configs;
} )();

function getRandomRunConfig ( value )
{
   var config = {};
   var configs = new Configs();
   var len = configs.runConfigs.length;
   var idx = Math.floor( Math.random() * len );
   confDesp = configs.runConfigs[idx];
   config[confDesp["name"]] = confDesp[value];
   return config;
}

function getRandomRebootConfig ( value )
{
   var config = {};
   var configs = new Configs();
   var len = configs.rebootConfigs.length;
   var idx = Math.floor( Math.random() * len );
   confDesp = configs.rebootConfigs[idx];
   config[confDesp["name"]] = confDesp[value];
   return config;
}

function getRandomForbidConfig ( value )
{
   var config = {};
   var configs = new Configs();
   var len = configs.forbidConfigs.length;
   var idx = Math.floor( Math.random() * len );
   confDesp = configs.forbidConfigs[idx];
   config[confDesp["name"]] = confDesp[value];
   return config;
}

function getConfigs ( value )
{
   var configs = new Configs();
   var runConfigs = configs.runConfigs;
   var rebootConfigs = configs.rebootConfigs;
   var forbidConfigs = configs.forbidConfigs;

   configs = {};
   configs["allConfigs"] = {};
   configs["runConfigs"] = {};
   configs["rebootConfigs"] = {};
   configs["forbidConfigs"] = {};
   for( i = 0; i < runConfigs.length; i++ )
   {
      var config = runConfigs[i];
      configs["allConfigs"][config["name"]] = config[value];
      configs["runConfigs"][config["name"]] = config[value];
   }
   for( i = 0; i < rebootConfigs.length; i++ )
   {
      config = rebootConfigs[i];
      configs["allConfigs"][config["name"]] = config[value];
      configs["rebootConfigs"][config["name"]] = config[value];
   }
   for( i = 0; i < forbidConfigs.length; i++ )
   {
      config = forbidConfigs[i];
      configs["allConfigs"][config["name"]] = config[value];
      configs["forbidConfigs"][config["name"]] = config[value];
   }
   return configs;
}

/************************************************************************
 * @Description : create group and start
 *                db: connection handle, can't be standalone
 *                rgName: group name
 *                nodesNum: node num, node svc like 26000 26010 ....
 *
 *                return logSourcePaths: log paths to be backed up
 * @author      : Liang XueWang
 ************************************************************************/
/*function createGroupAndNode( groupName, hostName, nodesNum )
{
   try
   {
      var rg = db.createRG( groupName ) ;
      var failedCount = 0;
      var nodes = [];
      for( var i = 0; i < nodesNum; i++ )
      {
         var svcName = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount ) ;
         var dbPath = RSRVNODEDIR + "data/" + svcName ;
         var checkSucc = false;
         var times = 0;
         var maxRetryTimes = 10;
         do
         {
            try
            {
               rg.createNode( hostName, svcName, dbPath, {diaglevel:5} ) ;
               println( "create node: " + hostName + ":" + svcName + " dbPath: " + dbPath ) ;
               checkSucc = true;
               nodes.push( { hostName: hostName, svcName: svcName.toString(), dbPath: dbPath } );
            }
            catch( e )
            {
               //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
               if( e == SDBCM_NODE_EXISTED || e == SDB_DIR_NOT_EMPTY )
               {
                  svcName = svcName + 10;
                  dbPath = RSRVNODEDIR + "data/" + svcName;
                  failedCount++;
               }
               else
               {
                  throw "create node failed!  port = " + svcName + " dbPath = " + dbPath + " errorCode: " + e;
               }
               times++;
            }
         }
         while(!checkSucc && times < maxRetryTimes);
      }
      println( "start group" ) ;
      rg.start() ;
      return nodes;
   }
   catch(e)
   {
      throw new Error(e);
   }
}*/

/************************************************************************
 * @Description : update config
 *                db: connection handle
 *                config: update config object
 *                option: update option object
 *                errno: errno expected
 * @author      : Liang XueWang
 ************************************************************************/
function updateConf ( db, configs, options, errno )
{

   try
   {
      db.updateConf( configs, options );
      if( errno !== undefined ) 
      {
         throw new Error( "Update config should be failed!" );
      }
   }
   catch( e )
   {
      if( errno === undefined || e.message != errno )
      {
         throw e;
      }
   }
}

/************************************************************************
 * @Description : delete config
 *                db: connection handle
 *                config: delete config object
 *                option: delete option object
 *                errno: errno expected
 * @author      : Liang XueWang
 ************************************************************************/
function deleteConf ( db, configs, options, errno )
{
   try
   {
      db.deleteConf( configs, options );
      if( errno !== undefined )
      {
         throw new Error( "Delete config should be failed!" );
      }
   }
   catch( e )
   {
      if( errno === undefined || e.message != errno )
      {
         throw e;
      }
   }
}

/************************************************************************
 * @Description : snapshot config on node
 *                host: node hostname
 *                svc: node svcname
 *                return conf obj, like { dbpath: "xxx", .... }
 * @author      : Liang XueWang
 ************************************************************************/
function getConfFromSnapshot ( db, hostName, svcName )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: hostName + ":" + svcName } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
   }
   cursor.close();
   return obj;
}

/************************************************************************
 * @Description : get node conf from conf file
 *                host: node hostname
 *                svc: node svcname
 *                return conf obj, like { dbpath: "xxx", .... }
 * @author      : Liang XueWang
 ************************************************************************/
function getConfFromFile ( hostName, svcName )
{
   var confFile = commGetRemoteInstallPath( hostName, CMSVCNAME ) + "/conf/local/" + svcName + "/sdb.conf";
   var remote = new Remote( hostName, CMSVCNAME );
   var cmd = remote.getCmd();
   var obj = {};
   var arr = cmd.run( "cat " + confFile ).split( "\n" );
   for( var i = 0; i < arr.length - 1; i++ )
   {
      var info = arr[i].split( "=" );
      var key = info[0];
      var val = info[1];
      obj[key] = val;
   }
   remote.close();
   return obj;
}

/************************************************************************
 * @Description : check snapshot conf 
 * allowedEmpty: 当删除配置参数，获取配置文件中的参数中只有固定的几个参数，没有期望的参数时为undefined
 * @author      : Liang XueWang
 ************************************************************************/
function checkResult ( expResult, actResult, allowedEmpty )
{
   if( allowedEmpty === undefined ) { allowedEmpty = false; }

   for( var key in expResult )
   {
      if( allowedEmpty )
      {
         if( actResult[key] !== undefined )
         {
            throw new Error( "config " + key + " should be undefined!" );
         }
         continue;
      }
      else if( expResult[key] != actResult[key] )
      {
         if( typeof expResult[key] !== "string" || ( actResult[key] + "/" != expResult[key] && expResult[key] + "/" != actResult[key] ) )
         {
            throw new Error( "Expected " + key + ": " + expResult[key] + ",  actual " + key + ": " + actResult[key] +
               "\n***expResult: " + JSON.stringify( expResult ) + "\n***actResult: " + JSON.stringify( actResult ) );
         }
      }
   }
} 
