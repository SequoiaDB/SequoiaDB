/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: sync business configure ( mysql )
@modify list:
   2019-05-28 JiaWen He  Init

@parameter
   var BUS_JSON = {"ClusterName":"myCluster1","BusinessName":"MySQLInstance1","BusinessType":"sequoiasql-mysql","User":"sac","Passwd":"123", "Address":[{"HostName":"ubuntu-test-03","svcname":"3306"}],"omaddr":"ubuntu-test-01:11785"} ;
*/

import( catPath( getSelfPath(), '../lib/parseMySQL.js' ) ) ;

function _getErrorResult( msg )
{
   var result ;

   if ( getLastErrObj() )
   {
      result = getLastErrObj().toObj() ;
   }
   else
   {
      result = {} ;
      result[FIELD_ERRNO]  = getLastError() ;
      result[FIELD_DETAIL] = getLastErrMsg() ;
   }

   return new SdbError( result[FIELD_ERRNO],
                        sprintf( msg, result[FIELD_DETAIL] ) ) ;
}

function _getInstallPath( PD_LOGGER, oma )
{
   var defaultFilePath = '/etc/default/sequoiasql-mysql' ;
   var installPath = '' ;

   for( var i = 0; i <= 10; ++i )
   {
      try
      {
         var config = oma.getIniConfigs( i == 0 ? defaultFilePath : defaultFilePath + i ).toObj() ;
         installPath = config[FIELD_INSTALL_DIR] ;
         break ;
      }
      catch( e )
      {
      }
   }

   if ( installPath.length == 0 )
   {
      rc = SDB_INVALIDARG ;
      var error = new SdbError( rc, "MySQL Install path not found" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return installPath ;
}

function _getConfig( PD_LOGGER, remote, dbpath )
{
   var configPath = catPath( dbpath, 'auto.cnf' ) ;
   var config = {} ;
   var newConfig = {} ;

   try
   {
      var ini = remote.getIniFile( configPath,
                                   SDB_INIFILE_FLAGS_MYSQL );
      config = ini.toObj() ;
      config = config.toObj() ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to get mysql config, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   for ( var key in config )
   {
      if( key.indexOf( 'mysqld.' ) == 0 )
      {
         var newKey = key.split( '.' )[1] ;

         if ( newKey == 'datadir' )
         {
            newKey = 'dbpath' ;
         }

         newConfig[newKey] = config[key] + '' ;
      }
   }

   return newConfig ;
}

function _findInstanceConfig( PD_LOGGER, oma, businessName,
                              hostName, mysqlPort, installPath )
{
   var instanceConfigPath = catPath( installPath, 'conf/instance' ) ;
   var result = [] ;
   var isFind = false ;
   var fileList ;
   var remote = null ;

   try
   {
      var agentPort = Oma.getAOmaSvcName( hostName ) ;
      remote = new Remote( hostName, agentPort ) ;
      var file = remote.getFile() ;
      fileList = file.list( { 'pathname': instanceConfigPath } ).toArray()
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to instance file, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   var isUnknow = ( businessName.length == 0 ) ;

   for( var index in fileList )
   {
      var fileInfo = JSON.parse( fileList[index] ) ;
      var fileName = fileInfo[FIELD_NAME2] ;
      var instanceFilePath = catPath( instanceConfigPath, fileName ) ;
      var config = oma.getIniConfigs( instanceFilePath ).toObj() ;
      var instanceName = config[FIELD_INSTNAME] ;

      if( isUnknow )
      {
         var dataPath = config[FIELD_SQLDATA] ;
         var instanceConfig = _getConfig( PD_LOGGER, remote, dataPath ) ;

         if( instanceConfig[FIELD_PORT2] == mysqlPort )
         {
            isFind = true ;
            result[0] = instanceName ;
            result[1] = instanceConfig ;
            break ;
         }
      }
      else
      {
         if ( instanceName == businessName )
         {
            var dataPath = config[FIELD_SQLDATA] ;
            isFind = true ;
            result[0] = instanceName ;
            result[1] = _getConfig( PD_LOGGER, remote, dataPath ) ;
            break ;
         }
      }
   }

   if ( isFind == false )
   {
      rc = SDB_INVALIDARG ;
      var error = new SdbError( rc, "Instance not found" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}

function testConnect( PD_LOGGER, hostName, agentPort, installPath, port, user, passwd )
{
   var error = null ;
   var remote = new Remote( hostName, agentPort ) ;
   var cmd = remote.getCmd() ;
   var sql = 'SELECT * FROM db limit 1' ;

   try
   {
      ExecSsql( cmd, installPath, port, user, passwd, 'mysql', sql ) ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to connect database, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }
}

function _syncConfig( PD_LOGGER )
{
   var result       = {}
   var clusterName  = BUS_JSON[FIELD_CLUSTER_NAME] ;
   var businessName = BUS_JSON[FIELD_BUSINESS_NAME] ;
   var businessType = BUS_JSON[FIELD_BUSINESS_TYPE] ;
   var addressList  = BUS_JSON[FIELD_ADDRESS] ;
   var user         = BUS_JSON[FIELD_USER] ;
   var passwd       = BUS_JSON[FIELD_PASSWD] ;
   var installPath  ;

   if ( addressList.length == 0 )
   {
      rc = SDB_SYS ;
      var error = new SdbError( rc, "address is empty" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   var hostName    = addressList[0][FIELD_HOSTNAME] ;
   var port        = addressList[0][FIELD_SVCNAME] ;
   var installPath = addressList[0][FIELD_INSTALL_PATH] ;
   var agentPort   = Oma.getAOmaSvcName( hostName ) ;
   var oma ;

   try
   {
      oma = new Oma( hostName, agentPort ) ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to connect agent, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if ( typeof( installPath ) != 'string' || installPath.length == 0 )
   {
      installPath = _getInstallPath( PD_LOGGER, oma ) ;
   }

   var config = _findInstanceConfig( PD_LOGGER, oma, businessName, 
                                     hostName, port, installPath ) ;

   testConnect( PD_LOGGER, hostName, agentPort, installPath, port, user, passwd ) ;

   var hostInfo    = {} ;

   hostInfo[FIELD_ERRNO]         = SDB_OK ;
   hostInfo[FIELD_DETAIL]        = "" ;
   hostInfo[FIELD_HOSTNAME]      = hostName ;
   hostInfo[FIELD_CLUSTER_NAME]  = clusterName ;
   hostInfo[FIELD_BUSINESS_NAME] = config[0] ;
   hostInfo[FIELD_BUSINESS_TYPE] = businessType ;
   hostInfo[FIELD_DEPLOYMOD]     = "" ;
   hostInfo[FIELD_CONFIG]        = [ config[1] ] ;

   result[FIELD_HOST_INFO] = [ hostInfo ] ;

   return result ;
}

function run()
{
   var result = {} ;

   var PD_LOGGER = new Logger( "sequoiasql-mysql.js" ) ;

   return _syncConfig( PD_LOGGER ) ;
}