import( "common/om.js" ) ;
import( "common/unit.js" ) ;

var CLUSTER_NAME ;
var BUSINESS_NAME ;
var BUSINESS_TYPE ;

var POSTGRES_NAME ;

var COORD_ADDR ;
var COORD_PORT ;

var BUZ_CONFIG ;
var TASK_ID ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test host" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.test( 'list business type', function(){
   var businessTypeList = OM_CTRL.list_business_type() ;
   UNIT_TEST.assert( true, businessTypeList.length > 1, "invalid list business type" ) ;
} ) ;

UNIT_TEST.test( 'list relationship', function(){
   OM_CTRL.list_relationship() ;
} ) ;

UNIT_TEST.test( 'query business authority', function(){
   OM_CTRL.query_business_authority() ;
} ) ;

UNIT_TEST.test( 'unbind sequoiadb distribution', function(){

   var hasSdb = false ;
   var businessList = OM_CTRL.query_business() ;

   for( var i in businessList )
   {
      var businessInfo = businessList[i] ;
      var clusterName  = businessInfo['ClusterName'] ;
      var businessName = businessInfo['BusinessName'] ;
      var businessType = businessInfo['BusinessType'] ;
      var deployMod    = businessInfo['DeployMod'] ;

      if( businessType == "sequoiadb" && deployMod == 'distribution' )
      {
         CLUSTER_NAME  = clusterName ;
         BUSINESS_NAME = businessName ;
         BUSINESS_TYPE = businessType ;
         hasSdb = true ;
         break ;
      }
   }

   UNIT_TEST.assert( true, hasSdb, "has no sequoiadb distribution business" ) ;

   var hasCoord = false ;
   var data = { 'cmd': 'list groups' } ;
   var groupList = OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
   for( var i in groupList )
   {
      var groupInfo = groupList[i] ;
      if( groupInfo['Role'] == 1 && groupInfo['GroupName'] == 'SYSCoord' )
      {
         COORD_ADDR = groupInfo['Group'][0]['HostName'] ;
         COORD_PORT = groupInfo['Group'][0]['Service'][0]['Name'] ;
         hasCoord = true ;
         break ;
      }
   }

   UNIT_TEST.assert( true, hasCoord, _sprintf( "has no coord, business=?", BUSINESS_NAME ) ) ;

   OM_CTRL.unbind_business( CLUSTER_NAME, BUSINESS_NAME ) ;
}, true ) ;

UNIT_TEST.test( 'discover sequoiadb distribution', function(){
   var config = {
      "ClusterName": CLUSTER_NAME,
      "BusinessType": BUSINESS_TYPE,
      "BusinessName": BUSINESS_NAME,
      "BusinessInfo": {
         "HostName": COORD_ADDR,
         "ServiceName": COORD_PORT,
         "User": "",
         "Passwd": "",
         "AgentService": "11790"
      }
   } ;
   OM_CTRL.discover_business( config ) ;
}, true ) ;

UNIT_TEST.test( 'sync sequoiadb distribution', function(){
   OM_CTRL.sync_business_configure( CLUSTER_NAME, BUSINESS_NAME ) ;
} ) ;

UNIT_TEST.test( 'get sequoiadb distribution extend config', function(){
   var templateInfo = {
      "ClusterName": CLUSTER_NAME,
      "BusinessName": BUSINESS_NAME,
      "DeployMod": "horizontal",
      "BusinessType": "sequoiadb",
      "Property": [
         {
            "Name": "replicanum",
            "Value": "3"
         },
         {
            "Name": "datagroupnum",
            "Value": "1"
         }
      ],
      "HostInfo": []
   } ;
   BUZ_CONFIG = OM_CTRL.get_business_config( templateInfo, 'extend' ) ;
   UNIT_TEST.assert( 1, BUZ_CONFIG.length, "get sequoiadb distribution extend config failed" ) ;

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

UNIT_TEST.test( 'extend sequoiadb distribution business', function(){

   var taskInfo = OM_CTRL.extend_business( BUZ_CONFIG ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "extend sequoiadb distribution business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check extend sequoiadb distribution business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "extend sequoiadb distribution business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "extend sequoiadb distribution business failed: taskid=" + TASK_ID ) ;
}, true ) ;

UNIT_TEST.test( 'shrink sequoiadb distribution business', function(){
   var config = {
      "ClusterName": BUZ_CONFIG['ClusterName'],
      "BusinessName": BUZ_CONFIG['BusinessName'],
      "Config": []
   } ;

   for( var i in BUZ_CONFIG['Config'] )
   {
      config['Config'].push( {
         "HostName": BUZ_CONFIG['Config'][i]['HostName'],
         "svcname": BUZ_CONFIG['Config'][i]['svcname']
      } ) ;
   }

   var taskInfo = OM_CTRL.shrink_business( config ) ;

   UNIT_TEST.assert( 1, taskInfo.length, "shrink sequoiadb distribution business failed: no task id" ) ;

   TASK_ID = taskInfo[0]['TaskID'] ;

}, true ) ;

UNIT_TEST.test( 'check shrink sequoiadb distribution business task', function(){
   TASK_INFO = OM_CTRL.query_task3( TASK_ID, IS_DEBUG ) ;
   UNIT_TEST.assert( 1, TASK_INFO.length, "shrink sequoiadb distribution business failed: no task info" ) ;
   UNIT_TEST.assert( 0, TASK_INFO[0]['errno'], "shrink sequoiadb distribution business failed: taskid=" + TASK_ID ) ;
}, true ) ;

UNIT_TEST.test( 'create relationship', function(){
   var hasPG = false ;
   var businessList = OM_CTRL.query_business() ;

   for( var i in businessList )
   {
      var businessInfo = businessList[i] ;
      var businessType = businessInfo['BusinessType'] ;
      var businessName = businessInfo['BusinessName'] ;

      if( businessType == "sequoiasql-postgresql" )
      {
         POSTGRES_NAME = businessName ;
         hasPG = true ;
         break ;
      }
   }

   UNIT_TEST.assert( true, hasPG, "has no postgresql distribution business" ) ;

   OM_CTRL.create_relationship( POSTGRES_NAME + '_' + BUSINESS_NAME + '_postgres', POSTGRES_NAME, BUSINESS_NAME, {
      "transaction": "off",
      "preferedinstance": "a",
      "DbName": "postgres"
   } ) ;
}, true ) ;

UNIT_TEST.test( 'remove relationship', function(){
   OM_CTRL.remove_relationship( POSTGRES_NAME + '_' + BUSINESS_NAME + '_postgres' ) ;
}, true ) ;

UNIT_TEST.start( IS_DEBUG ) ;



