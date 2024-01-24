import( "om.js" ) ;
import( "unit.js" ) ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test undeolpy" ) ;

UNIT_TEST.setUp( function(){
   OM_CTRL.login( OM_USER, OM_PASSWD ) ;
} ) ;

function _clean_sequoiadb_data( clusterName, businessName ){
   var data = { 'cmd': 'list collectionspaces' } ;

   var csList = OM_CTRL.sequoiadb_exec( clusterName, businessName, data ) ;
   for( var i in csList )
   {
      var name = csList[i]['Name'] ;
      var data = {
         'cmd': 'drop collectionspace',
         'name': name
      } ;
      OM_CTRL.sequoiadb_exec( clusterName, businessName, data ) ;
   }
}

function _clean_postgresql_data( clusterName, businessName ){
   var data = {
      'Sql': 'SELECT datname FROM pg_database WHERE datname NOT LIKE \'template0\' AND datname NOT LIKE \'template1\'',
      'DbName': 'postgres'
   } ;

   var dbList = OM_CTRL.postgresql_exec( clusterName, businessName, data ) ;
   for( var i in dbList )
   {
      var dbName = dbList[i]['datname'] ;

      var data = {
         'Sql': 'select t.table_name,t.table_schema, t.table_type, u.usename from information_schema.tables t join pg_catalog.pg_class c on (t.table_name = c.relname) join pg_catalog.pg_user u on (c.relowner = u.usesysid) where t.table_schema=\'public\' order by t.table_type desc',
         'DbName': dbName
      } ;

      var tbList = OM_CTRL.postgresql_exec( clusterName, businessName, data ) ;
      for( var k in tbList )
      {
         var tbName = tbList[k]['table_name'] ;
         var data = {
            'Sql': _sprintf( 'drop table "?"', tbName ),
            'DbName': dbName
         } ;

         OM_CTRL.postgresql_exec( clusterName, businessName, data ) ;
      }

      if( dbName != 'postgres' )
      {
         var data = {
            'Sql': _sprintf( 'drop database "?"', dbName ),
            'DbName': 'postgres'
         } ;

         OM_CTRL.postgresql_exec( clusterName, businessName, data ) ;
      }
   }
}

UNIT_TEST.test( 'clean business', function(){
   var businessList = OM_CTRL.query_business() ;
   for( var i in businessList )
   {
      var businessInfo = businessList[i] ;
      var clusterName  = businessInfo['ClusterName'] ;
      var businessName = businessInfo['BusinessName'] ;
      var addtionType  = businessInfo['AddtionType'] ;
      var businessType = businessInfo['BusinessType'] ;

      if( addtionType == 0 )
      {
         if( businessType == "sequoiadb" )
         {
            _clean_sequoiadb_data( clusterName, businessName ) ;
         }
         else if( businessType == "sequoiasql-postgresql" )
         {
            _clean_postgresql_data( clusterName, businessName ) ;
         }

         var taskInfo = OM_CTRL.remove_business( businessName ) ;
         if( taskInfo.length > 0 )
         {
            OM_CTRL.query_task3( taskInfo[0]['TaskID'] ) ;
         }
      }
      else
      {
         OM_CTRL.undiscover_business( clusterName, businessName ) ;
      }
   }
} ) ;

UNIT_TEST.test( 'clean host', function(){

   var clusterList = OM_CTRL.query_cluster() ;

   for( var i in clusterList )
   {
      var clusterName = clusterList[i]['ClusterName'] ;
      var hostInfo = {
         "ClusterName": clusterName,
         "HostInfo": []
      } ;

      var hostList = OM_CTRL.query_host( { 'ClusterName': clusterName } ) ;
      if( hostList.length == 0 )
      {
         continue ;
      }

      for( var k in hostList )
      {
         hostInfo['HostInfo'].push( { 'HostName': hostList[k]['HostName'] } ) ;
      }

      var taskInfo = OM_CTRL.remove_host( hostInfo ) ;
      if( taskInfo.length > 0 )
      {
         OM_CTRL.query_task3( taskInfo[0]['TaskID'] ) ;
      }
   }

} ) ;

UNIT_TEST.test( 'clean cluster', function(){

   var clusterList = OM_CTRL.query_cluster() ;

   for( var i in clusterList )
   {
      OM_CTRL.remove_cluster( clusterList[i]['ClusterName'] ) ;
   }

} ) ;

UNIT_TEST.start( IS_DEBUG ) ;



