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
@description: modify sequoiasql-mysql config
@modify list:
   2019-01-23 JiaWen He  Init

@parameter
   var BUS_JSON = {"Command":"update business config","ClusterName":"myCluster1","BusinessName":"myModule3","BusinessType":"sequoiasql-mysql","User":"","Passwd":"","Info":{"HostName":"ubuntu-jw-02","dbpath":"/opt/sequoiasql/mysql/database/3306","InstallPath":"/opt/sequoiasql/mysql/"},"Config":{"property":{"a":1,"b":2}}}

@return
   RET_JSON: the format is: { "errno": 0 }
*/

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

function _getAgentPort( hostName )
{
   return Oma.getAOmaSvcName( hostName ) ;
}

function _deleteConfig( remote, configFile, config )
{
   try
   {
      var ini = remote.getIniFile( configFile, SDB_INIFILE_FLAGS_MYSQL ) ;

      for ( var key in config )
      {
         ini.disableItem( 'mysqld', config[key] ) ;
      }

      ini.save() ;
   }
   catch( e )
   {
      var lastErrObj = getLastErrObj() ;
      if ( lastErrObj )
      {
         return lastErrObj ;
      }
   }
   return null ;
}

function _updateConfig( remote, configFile, config )
{
   try
   {
      var ini = remote.getIniFile( configFile, SDB_INIFILE_FLAGS_MYSQL ) ;

      for ( var key in config )
      {
         ini.setValue( 'mysqld', key, config[key] ) ;
      }

      ini.save() ;
   }
   catch( e )
   {
      var lastErrObj = getLastErrObj() ;
      if ( lastErrObj )
      {
         return lastErrObj ;
      }
   }
   return null ;
}

function _modifyConfig( PD_LOGGER, command )
{
   var result        = {} ;
   var info          = BUS_JSON[FIELD_INFO] ;
   var config        = BUS_JSON[FIELD_CONFIG] ;
   var businessName  = BUS_JSON[FIELD_BUSINESS_NAME] ;

   var hostName      = info[FIELD_HOSTNAME] ;
   var dbpath        = info[FIELD_DBPATH] ;
   var installPath   = info[FIELD_INSTALL_PATH] ;
   var agentPort     = _getAgentPort( hostName ) ;
   var property      = config[FIELD_PROPERTY] ;

   var configFile    = dbpath + '/auto.cnf' ;
   var remote        = null ;

   try
   {
      remote = new Remote( hostName, agentPort ) ;
   }
   catch( e )
   {
      var lastErrObj = getLastErrObj() ;
      result = lastErrObj.toObj() ;
      PD_LOGGER.log( PDERROR, lastErrObj ) ;
      return result ;
   }

   if ( FIELD_UPDATE_BUSINESS_CONFIG == command )
   {
      result = _updateConfig( remote, configFile, property ) ;
   }
   else if ( FIELD_DELETE_BUSINESS_CONFIG == command )
   {
      result = _deleteConfig( remote, configFile, property ) ;
   }

   if ( result !== null )
   {
      PD_LOGGER.log( PDERROR, result ) ;
      return result.toObj() ;
   }

   result = {} ;
   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   return result ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiasql-mysql.js" ) ;
   var result = null ;

   var command = BUS_JSON[FIELD_COMMAND] ;

   if ( FIELD_UPDATE_BUSINESS_CONFIG == command ||
        FIELD_DELETE_BUSINESS_CONFIG == command )
   {
      result = _modifyConfig( PD_LOGGER, command ) ;
   }
   else
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Invalid command [?]", command ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}