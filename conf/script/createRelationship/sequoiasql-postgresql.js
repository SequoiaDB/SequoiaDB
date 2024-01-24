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
@description: create sequoiasql-postgresql relationship
@modify list:
   2017-10-16 JiaWen He  Init

@parameter
   var BUS_JSON = {"Name":"myModule1_myModule2_postgres","From":{"Info":{"AddtionType":0,"BusinessName":"myModule2","BusinessType":"sequoiasql-postgresql","ClusterName":"myCluster1","DeployMod":"","Location":[{"HostName":"ubuntu-jw-01"}],"Time":{"$timestamp":"2017-10-16-15.01.43.000000"},"_id":{"$oid":"59e45957a018d9f17464d7ae"}},"Config":[{"HostName":"ubuntu-jw-01","port":"5432","InstallPath":"/opt/sequoiasql-postgresql/"}]},"To":{"Info":{"AddtionType":0,"BusinessName":"myModule1","BusinessType":"sequoiadb","ClusterName":"myCluster1","DeployMod":"distribution","Location":[{"HostName":"ubuntu-jw-01"}],"Time":{"$timestamp":"2017-10-14-13.16.21.000000"},"_id":{"$oid":"59e19da5a018d9f17464d7a9"}},"Config":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord"},{"HostName":"ubuntu-jw-01","svcname":"11820","role":"catalog"},{"HostName":"ubuntu-jw-01","svcname":"11830","role":"data"}],"User":"a","Passwd":"1"},"Options":{"a":123}}

@return
   RET_JSON: the format is: { "errno": 0 }
*/

function _getAgentPort( hostName )
{
   return Oma.getAOmaSvcName( hostName ) ;
}

function _getErrorMsg( rc, e, message )
{
   var error = null ;

   if( rc == SDB_OK )
   {
      rc = SDB_SYS ;
      error = new SdbError( rc, e.message ) ;
   }
   else if( rc )
   {
      error = new SdbError( rc, message ) ;
   }

   return error ;
}

function _execSql( PD_LOGGER, port, cmd, installPath, sql, database )
{
   var result = null ;

   if( typeof( database ) != 'string' || database.length == 0 )
   {
      database = 'postgres' ;
   }

   try
   {
      result = ExecSsql( cmd, installPath, port, database, sql ) ;
   }
   catch( e )
   {
      PD_LOGGER.log( PDWARNING, e ) ;
      throw e ;
   }

   return result['rc'] ;
}

function _relationWithSequoiaDB( PD_LOGGER )
{
   var result = {} ;
   var fromBuz       = BUS_JSON[FIELD_FROM] ;
   var fromBuzInfo   = fromBuz[FIELD_INFO] ;
   var fromBuzConfig = fromBuz[FIELD_CONFIG] ;
   var fromBuzName   = fromBuzInfo[FIELD_BUSINESS_NAME] ;
   var toBuz         = BUS_JSON[FIELD_TO] ;
   var toBuzInfo     = toBuz[FIELD_INFO] ;
   var toBuzConfig   = toBuz[FIELD_CONFIG] ;
   var user          = toBuz[FIELD_USER] ;
   var passwd        = toBuz[FIELD_PASSWD] ;
   var toBuzName     = toBuzInfo[FIELD_BUSINESS_NAME] ;
   var options       = BUS_JSON[FIELD_OPTIONS] ;
   var dbName        = options[FIELD_DB_NAME] ;
   var serverName    = BUS_JSON[FIELD_NAME] ;
   var remote, cmd, hostName, agentPort, port, installPath, address ;
   var serialize = '' ;

   delete options[FIELD_DB_NAME] ;

   if ( fromBuzConfig.length !== 1 )
   {
      var error = new SdbError( SDB_SYS, "Invalid from business configure" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if ( toBuzConfig.length <= 0 )
   {
      var error = new SdbError( SDB_SYS, "Invalid to business configure" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   fromBuzConfig = fromBuzConfig[0] ;

   hostName    = fromBuzConfig[FIELD_HOSTNAME] ;
   agentPort   = _getAgentPort( hostName ) ;
   port        = fromBuzConfig[FIELD_PORT2] ;
   installPath = fromBuzConfig[FIELD_INSTALL_PATH] ;

   //get coord or standalone address
   address = [] ;
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
                                         "address empty" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if( typeof( options[FIELD_ADDRESS2] ) == "string" &&
       strTrim( options[FIELD_ADDRESS2] ).length > 0 )
   {
      //custom address
      var addressStr = '' ;
      options[FIELD_ADDRESS2] = strTrim( options[FIELD_ADDRESS2] ) ;

      var customList = options[FIELD_ADDRESS2].split( ',' ) ;

      for( var i in customList )
      {
         var customInfo = strTrim( customList[i] ).split( ':' ) ;
         var isBuzAddress = false ;

         if( customInfo.length != 2 )
         {
            var error = new SdbError( SDB_SYS,
                                      sprintf( "Invalid address [?]",
                                               options[FIELD_ADDRESS2] ) ) ;
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

      serialize = sprintf( 'address \'?\', service \'?\'',
                           addressStr, '0' ) ;
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

      PD_LOGGER.log( PDEVENT, 'server address: ' + addressStr ) ;

      serialize = sprintf( 'address \'?\', service \'?\'',
                           addressStr, '0' ) ;
   }

   //serialize options
   for( var key in options )
   {
      if( key == FIELD_ADDRESS2 )
      {
         continue ;
      }
      var optionStr = sprintf( '? \'?\'', key, options[key] ) ;

      serialize += ', ' + optionStr ;
   }

   if( typeof( user ) == 'string' )
   {
      serialize += sprintf( ', user \'?\', password \'?\'', user, passwd ) ;
   }

   //connect remote agent
   try
   {
      remote = new Remote( hostName, agentPort ) ;
      cmd = remote.getCmd() ;
   }
   catch( e )
   {
      var error = _getErrorMsg( getLastError(), e,
                                sprintf( "Failed to get remote obj: host [?:?]",
                                         hostName, agentPort ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   //create extension
   try
   {
      var command = 'create extension sdb_fdw;' ;
      _execSql( PD_LOGGER, port, cmd, installPath, command, dbName ) ;
   }
   catch( e )
   {
      var error = _getErrorMsg( getLastError(), e, "Failed to create extension" ) ;
      PD_LOGGER.log( PDWARNING, error ) ;
   }

   try
   {
      var command = sprintf( 'create server ? foreign data wrapper' +
                             ' sdb_fdw options(?);', serverName, serialize ) ;

      _execSql( PD_LOGGER, port, cmd, installPath, command, dbName ) ;
   }
   catch( e )
   {
      var error = _getErrorMsg( getLastError(), e, "Failed to create server" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   result[FIELD_ERRNO] = SDB_OK ;
   return result ;
}

function run()
{
   import( '../conf/script/lib/parsePostgres.js' ) ;
   var PD_LOGGER = new Logger( "sequoiasql-postgresql.js" ) ;
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

