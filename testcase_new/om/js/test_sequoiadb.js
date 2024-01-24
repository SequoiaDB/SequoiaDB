import( "common/om.js" ) ;
import( "common/unit.js" ) ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test sequoiadb" ) ;

var CS_NAME = "Test_SequoiaDB_CS_FOO" ;
var CL_NAME = "Test_SequoiaDB_CL_BAR" ;

var FULL_NAME = CS_NAME + '.' + CL_NAME ;

var CLUSTER_NAME ;
var BUSINESS_NAME ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
   var hasSdb = false ;
   var businessList = OM_CTRL.query_business() ;

   for( var i in businessList )
   {
      var businessInfo = businessList[i] ;
      var clusterName  = businessInfo['ClusterName'] ;
      var businessName = businessInfo['BusinessName'] ;
      var businessType = businessInfo['BusinessType'] ;

      if( businessType == "sequoiadb" )
      {
         CLUSTER_NAME  = clusterName ;
         BUSINESS_NAME = businessName ;
         hasSdb = true ;
         break ;
      }
   }

   UNIT_TEST.assert( true, hasSdb, "has no sequoiadb business" ) ;
} ) ;

UNIT_TEST.test( 'create cs', function(){

   var data = {
      'cmd': 'create collectionspace',
      'name': CS_NAME,
      'options': {
         'PageSize': 65536,
         'LobPageSize': 262144
      }
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'create cl', function(){

   var data = {
      'cmd': 'create collection',
      'name': FULL_NAME,
      'options': {
         'Compressed': false,
         'ReplSize': 1
      }
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'insert', function(){

   for( var i = 1; i <= 20; ++i )
   {
      var data = {
         'cmd': 'insert',
         'name': FULL_NAME,
         'insertor': { 'name': 'abc_' + i, 'time': new Date(), 'number': i }
      } ;

      OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
   }

}, true ) ;

UNIT_TEST.test( 'delete', function(){

   var data = {
      'cmd': 'delete',
      'name': FULL_NAME,
      'deletor': { 'number': { '$gt': 15 } }
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'update', function(){

   var data = {
      'cmd': 'update',
      'name': FULL_NAME,
      'filter': { 'number': { '$gt': 10 } },
      'updator': { '$set': { 'number': 50 } }
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'query', function(){

   var data = {
      'cmd': 'query',
      'name': FULL_NAME,
      'filter': { 'number': { '$lte': 10 } },
      'returnnum': 30,
      'skip': 0
   } ;

   var result = OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;

   UNIT_TEST.assert( 10, result.length, "record count invalid" ) ;
}, true ) ;

UNIT_TEST.test( 'drop cl', function(){
   var data = {
      'cmd': 'drop collection',
      'name': FULL_NAME
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'drop cs', function(){
   var data = {
      'cmd': 'drop collectionspace',
      'name': CS_NAME
   } ;

   OM_CTRL.sequoiadb_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.start( IS_DEBUG ) ;



