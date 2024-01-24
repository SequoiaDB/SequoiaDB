import( "om.js" ) ;
import( "unit.js" ) ;

var CLUSTER_NAME = 'Test_Deploy_Cluster' ;
var CLUSTER_INFO = {
    "ClusterName": CLUSTER_NAME,
    "Desc": "",
    "SdbUser": "sdbadmin",
    "SdbPasswd": "sdbadmin",
    "SdbUserGroup": "sdbadmin_group",
    "InstallPath": "/opt/sequoiadb/",
    "GrantConf": [
        {
            "Name": "HostFile",
            "Privilege": true
        },
        {
            "Name": "RootUser",
            "Privilege": true
        }
    ]
} ;
var HOST_INFO = {
   "ClusterName": CLUSTER_NAME,
   "HostInfo": [],
   "User": "-",
   "Passwd": "-",
   "SshPort": "-",
   "AgentService": "-"
} ;
var HOST_CONF = {} ;
var TASK_ID   = -1 ;
var TASK_INFO = {} ;
var BUZ_CONFIG = {} ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test deolpy" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.test( 'create cluster', function(){
   OM_CTRL.create_cluster( CLUSTER_INFO ) ;
}, true ) ;

/* ========== 添加主机 ========== */

UNIT_TEST.test( 'scan host', function(){

   UNIT_TEST.assert( true, HOST_LIST.length > 0, "testcase config no host", true ) ;

   for( var i in HOST_LIST )
   {
      HOST_INFO['HostInfo'].push( { "IP":  HOST_LIST[i]['IP'], "User": HOST_LIST[i]['User'], "Passwd": HOST_LIST[i]['Passwd'], "SshPort": "22", "AgentService": "11790" } ) ;
   }
   var result = OM_CTRL.scan_host( HOST_INFO ) ;
   for( var i in result )
   {
      if( result[i]['errno'] == 0 )
      {
         for( var k in HOST_INFO['HostInfo'] )
         {
            if( result[i]['IP'] == HOST_INFO['HostInfo'][k]['IP'] )
            {
               HOST_INFO['HostInfo'][k]['HostName'] = result[i]['HostName'] ;
               break ;
            }
         }
      }
      else
      {
          UNIT_TEST.assert( 0, result[i]['errno'], "scan host failed: detail:" + JSON.stringify( result[i] ), true ) ;
      }
   }
}, true ) ;

UNIT_TEST.test( 'check host', function(){
   HOST_CONF = OM_CTRL.check_host( HOST_INFO ) ;

   for( var i in HOST_CONF )
   {
      var diskList = HOST_CONF[i]['Disk'] ;
      HOST_CONF[i]['Disk'] = [] ;

      for( var k in diskList )
      {
         if( diskList[k]['CanUse'] == true && diskList[k]['IsLocal'] == true &&
             diskList[k]['Mount'] == '/' )
         {
            HOST_CONF[i]['Disk'].push( {
               'Name':     diskList[k]['Name'],
               'Mount':    diskList[k]['Mount'],
               'Size':     diskList[k]['Size'],
               'Free':     diskList[k]['Free'],
               'IsLocal':  diskList[k]['IsLocal']
            } ) ;
         }
      }
   }
}, true ) ;

UNIT_TEST.test( 'add host', function(){
   for( var i in HOST_CONF )
   {
      for( var k in HOST_INFO['HostInfo'] )
      {
         if( HOST_CONF[i]['IP'] == HOST_INFO['HostInfo'][k]['IP'] )
         {
            HOST_CONF[i]['HostName'] = HOST_INFO['HostInfo'][k]['HostName'] ;
            HOST_CONF[i]['User']     = HOST_INFO['HostInfo'][k]['User'] ;
            HOST_CONF[i]['Passwd']   = HOST_INFO['HostInfo'][k]['Passwd'] ;
            HOST_CONF[i]['SshPort']  = HOST_INFO['HostInfo'][k]['SshPort'] ;
            HOST_CONF[i]['AgentService'] = HOST_INFO['HostInfo'][k]['AgentService'] ;
            break ;
         }
      }
   }
   var hostConf = {
      'ClusterName': CLUSTER_NAME,
      'HostInfo': HOST_CONF,
      'User': '-',
      'Passwd': '-',
      'SshPort': '-',
      'AgentService': '-'
   } ;

   var taskInfo = OM_CTRL.add_host( hostConf ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add host failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add host task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add host failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add host failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add host failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

UNIT_TEST.test( 'unbind host', function(){
   var hostInfo = [] ;
   for( var i in HOST_LIST )
   {
      hostInfo.push( { "HostName":  HOST_LIST[i]['HostName'] } ) ;
   }

   OM_CTRL.unbind_host( CLUSTER_NAME, hostInfo ) ;
}, true ) ;

/* ========== 发现主机 ========== */

UNIT_TEST.test( 'scan host 2', function(){
   var result = OM_CTRL.scan_host( HOST_INFO ) ;
   for( var i in result )
   {
      UNIT_TEST.assert( 0, result[i]['errno'], "scan host failed: detail:" + JSON.stringify( result[i] ), true ) ;
   }
}, true ) ;

UNIT_TEST.test( 'check host 2', function(){
   HOST_CONF = OM_CTRL.check_host( HOST_INFO ) ;
   for( var i in HOST_CONF )
   {
      var diskList = HOST_CONF[i]['Disk'] ;
      HOST_CONF[i]['Disk'] = [] ;

      for( var k in diskList )
      {
         if( diskList[k]['CanUse'] == true && diskList[k]['IsLocal'] == true &&
             diskList[k]['Mount'] == '/' )
         {
            HOST_CONF[i]['Disk'].push( {
               'Name':     diskList[k]['Name'],
               'Mount':    diskList[k]['Mount'],
               'Size':     diskList[k]['Size'],
               'Free':     diskList[k]['Free'],
               'IsLocal':  diskList[k]['IsLocal']
            } ) ;
         }
      }
   }
}, true ) ;

UNIT_TEST.test( 'add host 2', function(){
   for( var i in HOST_CONF )
   {
      for( var k in HOST_INFO['HostInfo'] )
      {
         if( HOST_CONF[i]['IP'] == HOST_INFO['HostInfo'][k]['IP'] )
         {
            HOST_CONF[i]['HostName'] = HOST_INFO['HostInfo'][k]['HostName'] ;
            HOST_CONF[i]['User']     = HOST_INFO['HostInfo'][k]['User'] ;
            HOST_CONF[i]['Passwd']   = HOST_INFO['HostInfo'][k]['Passwd'] ;
            HOST_CONF[i]['SshPort']  = HOST_INFO['HostInfo'][k]['SshPort'] ;
            HOST_CONF[i]['AgentService'] = HOST_INFO['HostInfo'][k]['AgentService'] ;
            break ;
         }
      }
   }
   var hostConf = {
      'ClusterName': CLUSTER_NAME,
      'HostInfo': HOST_CONF,
      'User': '-',
      'Passwd': '-',
      'SshPort': '-',
      'AgentService': '-'
   } ;

   var taskInfo = OM_CTRL.add_host( hostConf ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add host failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add host task 2', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add host failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add host failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add host failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

/* ========== 部署包 ========== */

UNIT_TEST.test( 'deploy postgresql package', function(){
   var packageName = "sequoiasql-postgresql" ;
   var installPath = "/opt/sequoiasql/postgresql/" ;
   var hostList = [] ;
   for( var i in HOST_INFO['HostInfo'] )
   {
      hostList.push( {
         "HostName": HOST_INFO['HostInfo'][i]['HostName'],
         "User":     HOST_INFO['HostInfo'][i]['User'],
         "Passwd":   HOST_INFO['HostInfo'][i]['Passwd']
      } ) ;
   }

   var taskInfo = OM_CTRL.deploy_package( CLUSTER_NAME, packageName, installPath, hostList, false ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "deploy postgresql package failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;
}, true ) ;

UNIT_TEST.test( 'check deploy postgresql task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "deploy postgresql package failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "deploy postgresql package failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "deploy postgresql failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

UNIT_TEST.test( 'deploy mysql package', function(){
   var packageName = "sequoiasql-mysql" ;
   var installPath = "/opt/sequoiasql/mysql/" ;
   var hostList = [] ;
   for( var i in HOST_INFO['HostInfo'] )
   {
      hostList.push( {
         "HostName": HOST_INFO['HostInfo'][i]['HostName'],
         "User":     HOST_INFO['HostInfo'][i]['User'],
         "Passwd":   HOST_INFO['HostInfo'][i]['Passwd']
      } ) ;
   }

   var taskInfo = OM_CTRL.deploy_package( CLUSTER_NAME, packageName, installPath, hostList, false ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add host failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;
}, true ) ;

UNIT_TEST.test( 'check deploy mysql task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "deploy mysql package failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "deploy mysql package failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "deploy mysql failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

/* ========== 创建sequoiadb集群业务 ========== */

UNIT_TEST.test( 'get sequoiadb distribution config', function(){
   var templateInfo = {
      "ClusterName": CLUSTER_NAME,
      "BusinessName": "test_sequoiadb_distribution",
      "DeployMod": "distribution",
      "BusinessType": "sequoiadb",
      "Property": [
         {
            "Name": "replicanum",
            "Value": "3"
         },
         {
            "Name": "datagroupnum",
            "Value": "2"
         },
         {
            "Name": "catalognum",
            "Value": "3"
         },
         {
            "Name": "coordnum",
            "Value": "0"
         }
      ],
      "HostInfo": []
   } ;
   BUZ_CONFIG = OM_CTRL.get_business_config( templateInfo ) ;
   UNIT_TEST.assert( 1, BUZ_CONFIG.length, "get sequoiadb distribution config failed" ) ;

   BUZ_CONFIG = BUZ_CONFIG[0] ;

   delete BUZ_CONFIG['Property'] ;

   for( var index in BUZ_CONFIG['Config'] )
   {
      var newNodeConfig = {} ;
      var nodeConfig = BUZ_CONFIG['Config'][index] ;

      for ( var key in nodeConfig )
      {
         if ( key != 'datagroupname' && typeof( nodeConfig[key] ) == 'string' && nodeConfig[key].length == 0 )
         {
            continue ;
         }

         newNodeConfig[key] = nodeConfig[key] ;
      }

      BUZ_CONFIG['Config'][index] = newNodeConfig ;
   }

}, true ) ;

UNIT_TEST.test( 'add sequoiadb distribution business', function(){

   var taskInfo = OM_CTRL.add_business( BUZ_CONFIG ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add sequoiadb distribution business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add sequoiadb distribution business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add sequoiadb distribution business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add sequoiadb distribution business failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add sequoiadb failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

/* ========== 创建sequoiadb单机业务 ========== */

UNIT_TEST.test( 'get sequoiadb standlone config', function(){
   var templateInfo = {
      "ClusterName": CLUSTER_NAME,
      "BusinessName": "test_sequoiadb_standalone",
      "DeployMod": "standalone",
      "BusinessType": "sequoiadb",
      "Property": [
         {
            "Name": "replicanum",
            "Value": "1"
         },
         {
            "Name": "datagroupnum",
            "Value": "1"
         },
         {
            "Name": "catalognum",
            "Value": "1"
         },
         {
            "Name": "coordnum",
            "Value": "1"
         }
      ],
      "HostInfo": []
   } ;
   for( var i in HOST_INFO['HostInfo'] )
   {
      templateInfo['HostInfo'].push( {
         "HostName": HOST_INFO['HostInfo'][i]['HostName']
      } ) ;
   }
   BUZ_CONFIG = OM_CTRL.get_business_config( templateInfo ) ;
   UNIT_TEST.assert( 1, BUZ_CONFIG.length, "get sequoiadb standlone config failed" ) ;

   BUZ_CONFIG = BUZ_CONFIG[0] ;

   delete BUZ_CONFIG['Property'] ;

   for( var index in BUZ_CONFIG['Config'] )
   {
      var newNodeConfig = {} ;
      var nodeConfig = BUZ_CONFIG['Config'][index] ;

      for ( var key in nodeConfig )
      {
         if ( key != 'datagroupname' && typeof( nodeConfig[key] ) == 'string' && nodeConfig[key].length == 0 )
         {
            continue ;
         }

         newNodeConfig[key] = nodeConfig[key] ;
      }

      BUZ_CONFIG['Config'][index] = newNodeConfig ;
   }

}, true ) ;

UNIT_TEST.test( 'add sequoiadb standlone business', function(){

   var taskInfo = OM_CTRL.add_business( BUZ_CONFIG ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add sequoiadb standlone business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add sequoiadb standlone business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add sequoiadb standlone business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add sequoiadb standlone business failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add sequoiadb failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

/* ========== 创建sequoiasql-postgresql业务 ========== */

UNIT_TEST.test( 'get postgresql config', function(){
   var templateInfo = {
      "ClusterName": CLUSTER_NAME,
      "BusinessName": "test_postgresql",
      "DeployMod": "",
      "BusinessType": "sequoiasql-postgresql",
      "Property": [],
      "HostInfo": []
   } ;
   for( var i in HOST_INFO['HostInfo'] )
   {
      templateInfo['HostInfo'].push( {
         "HostName": HOST_INFO['HostInfo'][i]['HostName']
      } ) ;
   }
   BUZ_CONFIG = OM_CTRL.get_business_config( templateInfo ) ;
   UNIT_TEST.assert( 1, BUZ_CONFIG.length, "get postgresql config failed" ) ;

   BUZ_CONFIG = BUZ_CONFIG[0] ;

   delete BUZ_CONFIG['Property'] ;

   var configList = BUZ_CONFIG['Config'] ;
   BUZ_CONFIG['Config'] = [] ;
   for( var i in configList )
   {
      var newConfig = {} ;
      var configInfo = configList[i] ;
      for( var key in configInfo )
      {
         if( configInfo[key].length > 0 )
         {
            newConfig[key] = configInfo[key] ;
         }
      }
      BUZ_CONFIG['Config'].push( newConfig ) ;
   }
}, true ) ;

UNIT_TEST.test( 'add postgresql business', function(){

   var taskInfo = OM_CTRL.add_business( BUZ_CONFIG ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add postgresql business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add postgresql business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add postgresql business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add postgresql business failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add postgresql failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;

/* ========== 创建sequoiasql-mysql业务 ========== */
/* 等mysql的sdb_mysql_ctrl修复再开启测试
UNIT_TEST.test( 'get mysql config', function(){
   var templateInfo = {
      "ClusterName": CLUSTER_NAME,
      "BusinessName": "test_mysql",
      "DeployMod": "",
      "BusinessType": "sequoiasql-mysql",
      "Property": [],
      "HostInfo": []
   } ;
   for( var i in HOST_INFO['HostInfo'] )
   {
      templateInfo['HostInfo'].push( {
         "HostName": HOST_INFO['HostInfo'][i]['HostName']
      } ) ;
   }
   BUZ_CONFIG = OM_CTRL.get_business_config( templateInfo ) ;
   UNIT_TEST.assert( 1, BUZ_CONFIG.length, "get mysql config failed" ) ;

   BUZ_CONFIG = BUZ_CONFIG[0] ;

   delete BUZ_CONFIG['Property'] ;

   var configList = BUZ_CONFIG['Config'] ;
   BUZ_CONFIG['Config'] = [] ;
   for( var i in configList )
   {
      var newConfig = {} ;
      var configInfo = configList[i] ;
      for( var key in configInfo )
      {
         if( configInfo[key].length > 0 )
         {
            newConfig[key] = configInfo[key] ;
         }
      }
      BUZ_CONFIG['Config'].push( newConfig ) ;
   }
}, true ) ;

UNIT_TEST.test( 'add mysql business', function(){

   UNIT_TEST.assert( 1, 0, JSON.stringify( BUZ_CONFIG ) ) ;

   var taskInfo = OM_CTRL.add_business( BUZ_CONFIG ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "add mysql business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check add mysql business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "add mysql business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "add mysql business failed: taskid=" + TASK_ID ) ;
   for( var i in TASK_INFO[0]['ResultInfo'] )
   {
      UNIT_TEST.assert( 0, TASK_INFO[0]['ResultInfo'][i]['errno'], "add mysql failed: detail:" + JSON.stringify( TASK_INFO[0]['ResultInfo'][i] ), true ) ;
   }
}, true ) ;
*/
UNIT_TEST.start( IS_DEBUG ) ;


