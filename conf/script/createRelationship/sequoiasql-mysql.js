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
@description: create sequoiasql-mysql relationship
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

function _mergeSequoiaDBAddress( PD_LOGGER, options, toBuzConfig )
{
   var address = [] ;
   var serialize = '' ;

   for( var index in toBuzConfig )
   {
      if( toBuzConfig[index][FIELD_ROLE] == FIELD_COORD ||
          toBuzConfig[index][FIELD_ROLE] == FIELD_STANDALONE )
      {
         var nodeInfo = {} ;

         nodeInfo[FIELD_HOSTNAME] = toBuzConfig[index][FIELD_HOSTNAME] ;
         nodeInfo[FIELD_SVCNAME]  = toBuzConfig[index][FIELD_SVCNAME] ;
         address.push( nodeInfo ) ;
      }
   }

   //serialize address
   if( address.length == 0 )
   {
      var error = new SdbError( SDB_SYS, "Invalid to business configure, " +
                                         "empty address" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if( typeof( options[FIELD_SEQUOIADB_CONN_ADDR] ) == "string" &&
       strTrim( options[FIELD_SEQUOIADB_CONN_ADDR] ).length > 0 )
   {
      //custom address
      var addressStr = '' ;
      options[FIELD_SEQUOIADB_CONN_ADDR] = strTrim( options[FIELD_SEQUOIADB_CONN_ADDR] ) ;

      var customList = options[FIELD_SEQUOIADB_CONN_ADDR].split( ',' ) ;

      for( var i in customList )
      {
         var customInfo = strTrim( customList[i] ).split( ':' ) ;
         var isBuzAddress = false ;

         if( customInfo.length != 2 )
         {
            var error = new SdbError( SDB_SYS,
                                      sprintf( "Invalid address [?]",
                                               options[FIELD_SEQUOIADB_CONN_ADDR] ) ) ;
            PD_LOGGER.log( PDERROR, error ) ;
            throw error ;
         }

         for( var index in address )
         {
            if( customInfo[0] == address[index][FIELD_HOSTNAME] &&
                customInfo[1] == address[index][FIELD_SVCNAME] )
            {
               isBuzAddress = true ;
               break ;
            }
         }

         if( isBuzAddress == false )
         {
            var error = new SdbError( SDB_SYS,
                                      sprintf( "Invalid address [?:?], " +
                                               "not a business address",
                                               customInfo[0],
                                               customInfo[1] ) ) ;
            PD_LOGGER.log( PDERROR, error ) ;
            throw error ;
         }

         if( i > 0 )
         {
            addressStr += ',' ;
         }
         addressStr += sprintf( '?:?', customInfo[0], customInfo[1] ) ;
      }

      PD_LOGGER.log( PDEVENT, 'server address: ' + addressStr ) ;

      serialize = addressStr ;
   }
   else
   {
      var addressStr = '' ;

      for( var index in address )
      {
         if( index > 0 )
         {
            addressStr += ',' ;
         }
         addressStr = sprintf( '?:?', address[index][FIELD_HOSTNAME],
                                      address[index][FIELD_SVCNAME] ) ;
      }

      serialize = addressStr ;
   }

   options[FIELD_SEQUOIADB_CONN_ADDR] = serialize ;

   return options ;
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

function _updateConfigOnline( PD_LOGGER, hostName, agentPort, mysqlPort,
                              mysqlUser, mysqlPasswd, installPath, configs )
{
   var sql = '' ;
   var cmd ;

   PD_LOGGER.log( PDEVENT, "Modify sequoiadb variables" ) ;

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

function _updateConfigOffline( PD_LOGGER, hostName, agentPort, path, config )
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

      for ( var key in config )
      {
         if( key.indexOf( 'sequoiadb_' ) == 0 )
         {
            file.setValue( 'mysqld', key, config[key] ) ;
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

function _relationWithSequoiaDB( PD_LOGGER )
{
   var result = {} ;

   var fromBuz       = BUS_JSON[FIELD_FROM] ;
   var fromBuzInfo   = fromBuz[FIELD_INFO] ;
   var fromBuzConfig = fromBuz[FIELD_CONFIG] ;
   var fromUser      = fromBuz[FIELD_USER] ;
   var fromPasswd    = fromBuz[FIELD_PASSWD] ;
   var fromBuzName   = fromBuzInfo[FIELD_BUSINESS_NAME] ;

   var toBuz         = BUS_JSON[FIELD_TO] ;
   var toBuzInfo     = toBuz[FIELD_INFO] ;
   var toBuzConfig   = toBuz[FIELD_CONFIG] ;
   var toUser        = toBuz[FIELD_USER] ;
   var toPasswd      = toBuz[FIELD_PASSWD] ;
   var toBuzName     = toBuzInfo[FIELD_BUSINESS_NAME] ;

   var options       = BUS_JSON[FIELD_OPTIONS] ;
   var hostName, agentPort, mysqlPort, installPath, error, dbpath, configFile ;

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   if ( fromBuzConfig.length !== 1 )
   {
      error = new SdbError( SDB_SYS, "Invalid from business configure" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if ( toBuzConfig.length <= 0 )
   {
      error = new SdbError( SDB_SYS, "Invalid to business configure" ) ;
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

   options = _mergeSequoiaDBAddress( PD_LOGGER, options, toBuzConfig ) ;

   if ( isString( toUser ) )
   {
      options[FIELD_SEQUOIADB_USER] = toUser ;
   }

   if ( isString( toPasswd ) )
   {
      options[FIELD_SEQUOIADB_PASSWORD] = toPasswd ;
   }

   //set mysql system variables
   _updateConfigOnline( PD_LOGGER, hostName, agentPort, mysqlPort,
                        fromUser, fromPasswd, installPath, options ) ;

   //set mysql config file
   _updateConfigOffline( PD_LOGGER, hostName, agentPort, configFile, options ) ;

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
      result = _relationWithSequoiaDB( PD_LOGGER ) ;
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

