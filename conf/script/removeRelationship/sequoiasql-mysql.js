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
@description: remove sequoiasql-mysql relationship
@modify list:
   2018-07-11 JiaWen He  Init

@parameter
   var BUS_JSON = {"Name":"myModule1_myModule2_postgres","From":{"Info":{"AddtionType":0,"BusinessName":"myModule2","BusinessType":"sequoiasql-mysql","ClusterName":"myCluster1","DeployMod":"","Location":[{"HostName":"ubuntu-jw-01"}],"Time":{"$timestamp":"2017-10-16-15.01.43.000000"},"_id":{"$oid":"59e45957a018d9f17464d7ae"}},"Config":[{"HostName":"ubuntu-jw-01","port":"5432","InstallPath":"/opt/sequoiasql-mysql/"}]},"To":{"Info":{"AddtionType":0,"BusinessName":"myModule1","BusinessType":"sequoiadb","ClusterName":"myCluster1","DeployMod":"distribution","Location":[{"HostName":"ubuntu-jw-01"}],"Time":{"$timestamp":"2017-10-14-13.16.21.000000"},"_id":{"$oid":"59e19da5a018d9f17464d7a9"}},"Config":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord"},{"HostName":"ubuntu-jw-01","svcname":"11820","role":"catalog"},{"HostName":"ubuntu-jw-01","svcname":"11830","role":"data"}],"User":"a","Passwd":"1"},"Options":{"a":123}}

@return
   RET_JSON: the format is: { "errno": 0 }
*/

import( catPath( getSelfPath(), '../lib/parseMySQL.js' ) ) ;

function _getAgentPort( hostName )
{
   return Oma.getAOmaSvcName( hostName ) ;
}

function _execSql( port, user, passwd, cmd, installPath, sql, database )
{
   var result = null ;

   if( typeof( database ) != 'string' || database.length == 0 )
   {
      database = 'mysql' ;
   }

   if( typeof( user ) != 'string' || user.length == 0 )
   {
      user = 'root' ;
   }

   if( typeof( passwd ) != 'string' || passwd.length == 0 )
   {
      passwd = '' ;
   }

   result = ExecSsql( cmd, installPath, port, user, passwd, database, sql ) ;

   return result['value'] ;
}

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

function _resetConfigOnline( PD_LOGGER, hostName, agentPort, mysqlPort,
                             mysqlUser, mysqlPasswd, installPath )
{
   var sql = '' ;
   var error = null ;
   var cmd ;
   var result = {} ;
   var configs = {
      'sequoiadb_conn_addr': 'localhost:11810',
      'sequoiadb_user': '',
      'sequoiadb_password': ''
   } ;

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   PD_LOGGER.log( PDEVENT, "Rest sequoiadb variables" ) ;

   try
   {
      var remote = new Remote( hostName, agentPort ) ;
      cmd = remote.getCmd() ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to connect agent, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   for ( var key in configs )
   {
      if ( isString( configs[key] ) )
      {
         sql += sprintf( "set global ?='?';", key, configs[key] ) ;
      }
      else
      {
         sql += sprintf( "set global ?=?;", key, configs[key] ) ;
      }
   }

   try
   {
      _execSql( mysqlPort, mysqlUser, mysqlPasswd, cmd, installPath, sql ) ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to set mysql variables, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }
}

function _resetConfigOffline( PD_LOGGER, hostName, agentPort, path )
{
   var remote ;

   PD_LOGGER.log( PDEVENT, "Save mysql configs" ) ;

   try
   {
      remote = new Remote( hostName, agentPort ) ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to connect agent, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   try
   {
      var file = remote.getIniFile( path, SDB_INIFILE_FLAGS_MYSQL ) ;

      file.setValue( 'mysqld', 'sequoiadb_conn_addr', 'localhost:11810' ) ;
      file.setValue( 'mysqld', 'sequoiadb_user', '' ) ;
      file.setValue( 'mysqld', 'sequoiadb_password', '' ) ;

      var list = file.toObj().toObj() ;

      for ( var key in list )
      {
         if ( key.indexOf( 'mysqld.sequoiadb_' ) == 0 )
         {
            var item = key.split( '.', 2 ) ;

            try
            {
               file.disableItem( item[0], item[1] ) ;
            }
            catch( e )
            {
            }
         }
      }

      file.save() ;
   }
   catch( e )
   {
      var error = _getErrorResult( "Failed to modify config file, detail: ?" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }
}

function _removeWithSequoiaDB( PD_LOGGER )
{
   var result = {} ;

   var fromBuz       = BUS_JSON[FIELD_FROM] ;
   var fromBuzInfo   = fromBuz[FIELD_INFO] ;
   var fromBuzConfig = fromBuz[FIELD_CONFIG] ;
   var fromUser      = fromBuz[FIELD_USER] ;
   var fromPasswd    = fromBuz[FIELD_PASSWD] ;
   var fromBuzName   = fromBuzInfo[FIELD_BUSINESS_NAME] ;

   var hostName, agentPort, mysqlPort, installPath, dbpath, configFile ;

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   if ( fromBuzConfig.length !== 1 )
   {
      var error = new SdbError( SDB_SYS, "Invalid from business configure" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   fromBuzConfig = fromBuzConfig[0] ;

   hostName    = fromBuzConfig[FIELD_HOSTNAME] ;
   agentPort   = _getAgentPort( hostName ) ;
   dbpath      = fromBuzConfig[FIELD_DBPATH] ;
   mysqlPort   = fromBuzConfig[FIELD_PORT2] ;
   installPath = fromBuzConfig[FIELD_INSTALL_PATH] ;
   configFile  = dbpath + '/auto.cnf' ;

   //reset mysql system variables
   _resetConfigOnline( PD_LOGGER, hostName, agentPort, mysqlPort,
                       fromUser, fromPasswd, installPath ) ;

   //reset mysql config file
   _resetConfigOffline( PD_LOGGER, hostName, agentPort, configFile ) ;

   return result ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiasql-mysql.js" ) ;
   var result = null ;

   var toBuz        = BUS_JSON[FIELD_TO] ;
   var toBuzInfo    = toBuz[FIELD_INFO] ;
   var businessType = toBuzInfo[FIELD_BUSINESS_TYPE] ;

   if ( FIELD_SEQUOIADB == businessType )
   {
      result = _removeWithSequoiaDB( PD_LOGGER ) ;
   }
   else
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Invalid business type [?]",
                                         businessType ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}

