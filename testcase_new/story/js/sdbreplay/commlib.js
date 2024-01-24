/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2019/6/27  XiaoNi Huang Init
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

isSdbReplayEnable();

var cmd = initCmd();
var localPath = null;
var installDir = getInstallDir();

var tmpFileDir = WORKDIR + '/sdbreplay/';
var testCaseDir = getTestCaseDir();
var rtCmd = null;


/* ****************************************************
@description: judge sdbreplay enable
**************************************************** */
function isSdbReplayEnable ()
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { "role": "data" } );
   var configs = cursor.current().toObj();
   if( configs.logwritemod !== "full" || configs.logtimeon !== "TRUE"
      || configs.archiveon !== "TRUE" || configs.archivetimeout > 10 )
   {
      cursor.close();
      throw new Error( "isSdbReplayEnable fail,[judge the sdbreplay is enable, data node conf as follows]" +
         "[logwritemod:full, logtimeon:TRUE, archiveon:TRUE, archivetimeout <= 10]" +
         "[logwritemod:" + configs.logwritemod + ", logtimeon:" + configs.logtimeon
         + ", archiveon:" + configs.archiveon + ", archivetimeout:" + configs.archivetimeout + "]" );
   }
}

/* ****************************************************
@description: new Cmd
@return: cmd
**************************************************** */
function initCmd ()
{
   var cmd = new Cmd();
   return cmd;
}

/* ****************************************************
@description: new Cmd
@return: cmd
**************************************************** */
function getRemoteCmd ( groupName )
{
   var hostName = getMasterHostName( groupName );
   var remote = new Remote( hostName, CMSVCNAME );
   println( "---remote host is " + hostName );
   rtCmd = remote.getCmd();
   return rtCmd;
}

/* ****************************************************
@description: exec sdbreplay
@return: results
@e.g:
  var rc = cmd.run('/opt/sequoiadb/bin/sdbreplay --type replica --path /home/sequoiadb/database/data/11850/replicalog/ --filter \'{CL:["cs.cl"],OP:["insert","update","delete"]}\' --outputconf /opt/test_sdbreplay/conf/sdbreplay.conf --status /opt/test_sdbreplay/1.status')
**************************************************** */
function execSdbReplay ( rtCmd, groupName, clNameArr, type, confPath, statusPath, daemon, watch, filter )
{
   var confName = clNameArr[0] + ".conf";
   var statusName = clNameArr[0] + ".status";
   var tmpCLNameArr = [];
   for( i = 0; i < clNameArr.length; i++ )
   {
      var tmpCLName = '\"' + clNameArr[i] + '\"';
      tmpCLNameArr.push( tmpCLName );
   }
   if( typeof ( confPath ) == "undefined" ) { confPath = tmpFileDir + confName; }
   if( typeof ( statusPath ) == "undefined" ) { statusPath = tmpFileDir + statusName; }
   if( typeof ( type ) == "undefined" ) { type = "archive"; }  //archive is the main user scenario.
   if( typeof ( daemon ) == "undefined" ) { daemon = false; }
   if( typeof ( watch ) == "undefined" ) { watch = false; }
   if( typeof ( filter ) == "undefined" ) 
   {
      filter = '\'{CL: [' + tmpCLNameArr + '], OP: ["insert", "update", "delete"] }\'';
   }

   // ready file
   var dbPath = getMasterDBPath( groupName );
   var logPath = "";
   if( type === "archive" ) 
   {
      logPath = dbPath + "archivelog";
   }
   else if( type === "replica" )
   {
      logPath = dbPath + "replicalog";
   }

   var command = installDir + 'bin/sdbreplay'
      + ' --type ' + type
      + ' --path ' + logPath
      + ' --filter ' + filter
      + ' --outputconf ' + confPath
      + ' --status ' + statusPath
      + ' --daemon ' + daemon
      + ' --watch ' + watch;

   var totalRetryTimes = 200;
   var currentRetryTimes = 0;
   var clName = clNameArr[0].split( "." )[1];
   var lsCommand = "ls " + tmpFileDir + " | grep " + clName + " | grep csv";

   //重放前对之前的操作进行刷盘
   db.sync();
   while( true ) 
   {
      var rcSdbreplay = rtCmd.run( "cd " + tmpFileDir + "; " + command );
      var rcLS;
      try 
      {
         rcLS = rtCmd.run( lsCommand ).split( "\n" )[0];
         break;
      } catch( e )
      {
         currentRetryTimes++;
         sleep( 100 );
         rtCmd.run( "cd " + tmpFileDir + "; rm *" + clName + "*.status" );
         if( currentRetryTimes >= totalRetryTimes ) 
         {
            throw new Error( "Failed to get csv file, after retry " + currentRetryTimes + "." );
         }
      }
   }

   return rcSdbreplay;
}

/* ****************************************************
@description: config the output csv file
    fieldType:   
        ORIGINAL_TIME��
        AUTO_OP��
        CONST_STRING��
        MAPPING_STRING��
        MAPPING_INT��
        MAPPING_LONG��
        MAPPING_DECIMAL��
        MAPPING_TIMESTAMP
**************************************************** */
function readyOutputConfFile ( rtCmd, groupName, csName, clName, fieldType, delimiter )
{
   getOutputConfFile( groupName, csName, clName );
   configOutputConfFile( rtCmd, groupName, csName, clName, fieldType, delimiter );
}

/* ****************************************************
@description: get outputconf file
**************************************************** */
function getOutputConfFile ( groupName, csName, clName, confName )
{
   if( typeof ( confName ) == "undefined" ) { confName = "sdbreplay.conf"; }

   var fullCLName = csName + "." + clName;
   var mstHostName = getMasterHostName( groupName );
   var sourceFilePath = testCaseDir + "conf/" + confName;
   var targetConfPath = tmpFileDir + fullCLName + ".conf";
   File.scp( sourceFilePath, mstHostName + ":" + CMSVCNAME + "@" + targetConfPath );
   //http://jira.web:8080/browse/CI-2526 CI上调整WORKDIR传参后，替换写死公共目录的用例适配外部传参目录
   rtCmd.run( "sed -i 's#/tmp/jstest/sdbreplay#" + tmpFileDir + "#g' " + targetConfPath );
}

/* ****************************************************
@description: config the output csv file
    fieldType:   
        ORIGINAL_TIME��
        AUTO_OP��
        CONST_STRING��
        MAPPING_STRING��
        MAPPING_INT��
        MAPPING_LONG��
        MAPPING_DECIMAL��
        MAPPING_TIMESTAMP
**************************************************** */
function configOutputConfFile ( rtCmd, groupName, csName, clName, fieldType, delimiter )
{
   if( typeof ( fieldType ) == "undefined" ) { fieldType = "MAPPING_STRING"; }
   if( typeof ( delimiter ) == "undefined" ) { delimiter = ","; }

   var fullCLName = csName + "." + clName;
   var targetConfPath = tmpFileDir + fullCLName + ".conf";

   rtCmd.run( "sed -i 's/filePrefix_ori/test_" + groupName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/delimiter_ori/" + delimiter + "/g' " + targetConfPath );

   rtCmd.run( "sed -i 's/source_fullCLName_ori/" + fullCLName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullCLName + "_new/g' " + targetConfPath );

   rtCmd.run( "sed -i 's/fieldType_ori/" + fieldType + "/g' " + targetConfPath );
}

/* ****************************************************
@description: check the output csv file
**************************************************** */
function checkCsvFile ( rtCmd, clName, expDataArr )
{

   var csvFileName = rtCmd.run( "ls " + tmpFileDir + " | grep " + clName + " | grep csv" ).split( "\n" )[0];
   var csvFilePath = tmpFileDir + csvFileName;

   var actDataArr = rtCmd.run( "cat " + csvFilePath ).split( "\n" );
   for( i = 0; i < actDataArr.length; i++ )
   {
      assert.equal( actDataArr[i], expDataArr[i] );
   }
}

/* ****************************************************
@description: check the output status file
**************************************************** */
function checkStatusFile ( rtCmd, statusFilePath, expSubString )
{
   var actString = rtCmd.run( "cat " + statusFilePath ).split( "\n" )[0];
   assert.notEqual( actString.indexOf( expSubString ), -1 );
}

/* ****************************************************
@description: get testcase director
@return: testcase director
**************************************************** */
function getTestCaseDir ()
{
   if( typeof ( TESTCASEDIR ) == "undefined" ) 
   {
      var testCaseDir = './testcase_new/story/js/sdbreplay/';
   }
   else
   {
      // TESTCASEDIR default: ....../testcases/hlt/js_testcases/js/sdbreplay/
      var testCaseDir = TESTCASEDIR + '/';
   }
   return testCaseDir;
}

/* ****************************************************
@description: get install_dir of sequoiadb
   sdbreplay因为要指定 - path replicalog目录，且用例本身是通过new remote().getCmd()在sequoiadb节点所在远程机器执行的
   所以获取远程机器sequoiadb安装目录下的bin/sdbreplay即可。
@return: install_dir
**************************************************** */
function getInstallDir ()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var rtCmd = remote.getCmd();
   //get sequoiadb, if not exists to throw
   var tmpDir = rtCmd.run( 'find /etc/default/sequoiadb' );
   //get sequoiadb install_dir configurature item, if not exists to throw
   var tmpDir = rtCmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR"' );
   //get sequoiadb director, if not exists to throw
   var tmpDir = rtCmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );
   var installPath = tmpDir.split( "\n" )[0] + "/";

   return installPath;
}

/* ****************************************************
@description: ready tmp director
**************************************************** */
function initTmpDir ( rtCmd )
{
   rtCmd.run( "rm -rf " + tmpFileDir + "*.*" );
   rtCmd.run( "mkdir -p " + tmpFileDir );
}

/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj, message )
{
   if( message == undefined ) { message = ""; }
   if( optionObj == undefined ) { optionObj = { ReplSize: 0 }; }

   commDropCL( db, csName, clName, true, true );

   var cl = commCreateCL( db, csName, clName, optionObj, true, true );

   return cl;
}

/* ****************************************************
@description: clean cl
**************************************************** */
function cleanCL ( csName, clName )
{

   commDropCL( db, csName, clName, false, false );
}

/* ****************************************************
@description: clean file
**************************************************** */
function cleanFile ( rtCmd )
{
   rtCmd.run( "rm -rf " + tmpFileDir + "*.*" );
}

/* ****************************************************
@description: backup file, when the testcase fail
**************************************************** */
function backupFile ( rtCmd, clName )
{
   var targetPath = tmpFileDir + clName + "/";
   rtCmd.run( "mkdir -p " + targetPath );

   // list all files in the current path
   var lsCommand = "ls " + tmpFileDir + " | grep " + clName + " | grep csv";
   var fileNames = rtCmd.run( lsCommand ).split( "\n" );

   // copy all files to the target path
   for( i = 0; i < fileNames.length - 1; i++ )
   {
      var sourcePath = tmpFileDir + fileNames[i];
      var cpCommand = "cp " + sourcePath + " " + targetPath;
      rtCmd.run( cpCommand );
   }
}

/* ****************************************************
@description: get dataRG Info
@parameter:
   [nameStr] "GroupName","HostName","svcname"
@return: groupArray
**************************************************** */
function getDataGroupNames ()
{
   var tmpArray = commGetGroups( db );
   var groupNameArray = new Array;
   for( i = 0; i < tmpArray.length; i++ )
   {
      groupNameArray.push( tmpArray[i][0].GroupName );
   }
   return groupNameArray;
}

/* ****************************************************
@description: get hostName of master node
@return: masterHostName
**************************************************** */
function getMasterHostName ( groupName )
{
   var masterHostName = db.getRG( groupName ).getMaster().getHostName();
   return masterHostName;
}

/* ****************************************************
@description: get the data directory of the sequoiadb node
@return: dir
**************************************************** */
function getMasterDBPath ( groupName ) 
{
   var info = db.list( SDB_SNAP_SYSTEM, { "GroupName": groupName } ).current().toObj();
   var primaryNode = info.PrimaryNode;
   var groups = info.Group;
   for( i = 0; i < groups.length; i++ )
   {
      var group = groups[i];
      var nodeID = group.NodeID;
      if( nodeID === primaryNode ) 
      {
         var dbpath = group.dbpath;
         break;
      }
   }
   return dbpath;
}

/* ****************************************************
@description: get random string
@return: string
**************************************************** */
function getRandomString ( strLen ) 
{
   var str = "";
   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 );
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

/* ****************************************************
@description: get random int
@return: int
**************************************************** */
function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

/* ****************************************************
@description: turn to local time
@parameter:
   time: Timestamp with time zone to millisecond,eg:'1901-12-31T15:54:03.000Z'
   format: eg:%Y-%m-%d-%H.%M.%S.000000
@return: 
   localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   var msecond = new Date( time ).getTime();
   var second = parseInt( msecond / 1000 );  //millisecond to second
   var localtime = cmd.run( 'date -d@"' + second + '" "+' + format + '"' ).split( "\n" )[0];

   return localtime;
}

/* ****************************************************
@description: new File
@return: file
**************************************************** */
function initFile ( fileName )
{
   var file = new File( fileName );
   return file;
}

/* ****************************************************
@description: getLSN
@return: lsn
**************************************************** */
function getMinLSN ( groupNames )
{
   var svcName = db.getRG( groupNames[0] ).getMaster().getServiceName();
   cursor = db.snapshot( 6, { ServiceName: svcName, RawData: true, IsPrimary: true } );
   var minLSN = cursor.current().toObj().CompleteLSN;
   return minLSN;
}

