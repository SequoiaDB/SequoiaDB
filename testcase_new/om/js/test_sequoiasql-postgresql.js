import( "common/om.js" ) ;
import( "common/unit.js" ) ;
import( "common/loadConf.js" ) ;

var PG_DATABASE_NAME = "TEST_PG_DB_FOO" ;
var PG_TABLE_NAME = "TEST_PG_DB_BAR" ;

var CLUSTER_NAME ;
var BUSINESS_NAME ;

var OM_CTRL = new SdbOMCtrl( OMHOSTNAME, OMWEBNAME ) ;
var UNIT_TEST = new SdbUnit( "Test postgresql" ) ;

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

      if( businessType == "sequoiasql-postgresql" )
      {
         CLUSTER_NAME  = clusterName ;
         BUSINESS_NAME = businessName ;
         hasSdb = true ;
         break ;
      }
   }

   UNIT_TEST.assert( true, hasSdb, "has no postgresql business" ) ;
} ) ;

UNIT_TEST.test( 'create database', function(){
   var data = {
      'Sql': _sprintf( 'create database "?"', PG_DATABASE_NAME ),
      'DbName': 'postgres'
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'create table', function(){
   var data = {
      'Sql': _sprintf( 'CREATE TABLE "?" ( "a" integer NULL , "b" text NULL , "c" boolean NULL )', PG_TABLE_NAME ),
      'DbName': PG_DATABASE_NAME
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'insert', function(){
   for( var i = 1; i <= 20; ++i )
   {
      var data = {
         'Sql': _sprintf( 'INSERT INTO "?" ("a","b","c") VALUES (\'?\',\'?\',\'?\')', PG_TABLE_NAME, i, 'abcdefg_' + i, 't' ),
         'DbName': PG_DATABASE_NAME
      } ;

      OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
   }
}, true ) ;

UNIT_TEST.test( 'delete', function(){
   var data = {
      'Sql': _sprintf( 'DELETE FROM "?" WHERE "a" > \'?\'', PG_TABLE_NAME, 15 ),
      'DbName': PG_DATABASE_NAME
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'update', function(){
   var data = {
      'Sql': _sprintf( 'UPDATE "?" SET "c" = \'f\' WHERE "a" > \'?\'', PG_TABLE_NAME, 10 ),
      'DbName': PG_DATABASE_NAME
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'query', function(){
   var data = {
      'Sql': _sprintf( 'SELECT * FROM "?"', PG_TABLE_NAME ),
      'DbName': PG_DATABASE_NAME
   } ;

   var result = OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
   UNIT_TEST.assert( 15, result.length, "record count invalid" ) ;
}, true ) ;

UNIT_TEST.test( 'drop table', function(){
   var data = {
      'Sql': _sprintf( 'drop table "?"', PG_TABLE_NAME ),
      'DbName': PG_DATABASE_NAME
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.test( 'drop database', function(){
   var data = {
      'Sql': _sprintf( 'drop database "?"', PG_DATABASE_NAME ),
      'DbName': 'postgres'
   } ;

   OM_CTRL.postgresql_exec( CLUSTER_NAME, BUSINESS_NAME, data ) ;
}, true ) ;

UNIT_TEST.start( IS_DEBUG ) ;



