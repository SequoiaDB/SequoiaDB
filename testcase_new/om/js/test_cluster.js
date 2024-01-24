import( "common/om.js" ) ;
import( "common/unit.js" ) ;

var CLUSTER_NAME = 'Test_Cluster' ;
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
            "Privilege": false
        },
        {
            "Name": "RootUser",
            "Privilege": true
        }
    ]
} ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test cluster" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

UNIT_TEST.finish( function( isError ){
   if( isError == false )
   {
      OM_CTRL.remove_cluster( CLUSTER_NAME ) ;
   }
} ) ;

UNIT_TEST.test( 'create cluster', function(){
   OM_CTRL.create_cluster( CLUSTER_INFO ) ;
}, true ) ;

UNIT_TEST.test( 'query cluster', function(){

   var result = OM_CTRL.query_cluster( { 'ClusterName': CLUSTER_NAME } ) ;

   UNIT_TEST.assert( 1, result.length, "Cluster is not exist" ) ;

   delete result[0]['_id'] ;
   CLUSTER_INFO = result[0] ;

   var hasHostFile = false ;
   for( var i in CLUSTER_INFO['GrantConf'] )
   {
      if( result[0]['GrantConf'][i]['Name'] == 'HostFile' )
      {
         hasHostFile = true ;
      }
   }

   UNIT_TEST.assert( true, hasHostFile, _sprintf( "cluster sysconf has no HostFile, cluster=?", CLUSTER_NAME ) ) ;

} ) ;

UNIT_TEST.test( 'grant sysconf', function(){
   OM_CTRL.grant_sysconf( CLUSTER_NAME, 'HostFile', true ) ;
}, true ) ;

UNIT_TEST.test( 'check sysconf', function(){

   var result = OM_CTRL.query_cluster( { 'ClusterName': CLUSTER_NAME } ) ;

   UNIT_TEST.assert( 1, result.length, "Cluster is not exist" ) ;

   var hasHostFile = false ;
   for( var i in result[0]['GrantConf'] )
   {
      if( result[0]['GrantConf'][i]['Name'] == 'HostFile' )
      {
         hasHostFile = true ;
         UNIT_TEST.assert( true, result[0]['GrantConf'][i]['Privilege'], "failed to grant sysconf, name=HostFile" ) ;
      }
   }

   UNIT_TEST.assert( true, hasHostFile, "cluster has no HostFile" ) ;

}, true ) ;

UNIT_TEST.start( IS_DEBUG ) ;



