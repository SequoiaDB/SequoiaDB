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
@description: modify sequoiasql-postgresql config
@modify list:
   2018-12-13 JiaWen He  Init

@parameter
   var BUS_JSON = {"Command":"update business config","ClusterName":"myCluster1","BusinessName":"myModule3","BusinessType":"sequoiasql-postgresql","User":"","Passwd":"","Info":{"HostName":"ubuntu-jw-02","dbpath":"/opt/sequoiasql/postgresql/database/5432","InstallPath":"/opt/sequoiasql/postgresql/"},"Config":{"property":{"a":1,"b":2}}}

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

function _runRemoteCmd( cmd, command, arg, timeout )
{
   var error = null ;
   var out = '' ;

   try
   {
      cmd.run( command, arg, timeout ) ;
      out = cmd.getLastOut() ;
   }
   catch( e )
   {
      var rc = cmd.getLastRet() ;
      out = cmd.getLastOut() ;

      if( rc )
      {
         error = new SdbError( rc, out ) ;
      }
      else
      {
         if( typeof( e ) == "number" )
         {
            error = new SdbError( e, "failed to exec cmd" ) ;
         }
         else
         {
            error = new SdbError( SDB_SYS, "failed to exec cmd." ) ;
         }
      }
   }

   return { 'error': error, 'out': out } ;
}

function _deleteConfig( remote, configFile, config )
{
   try
   {
      var ini = remote.getIniFile( configFile, SDB_INIFILE_FLAGS_POSTGRESQL ) ;

      for ( var key in config )
      {
         ini.disableItem( config[key] ) ;
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
      var ini = remote.getIniFile( configFile, SDB_INIFILE_FLAGS_POSTGRESQL ) ;

      for ( var key in config )
      {
         ini.setValue( key, config[key] ) ;
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

   var ctlFile       = installPath + '/bin/sdb_sql_ctl' ;
   var configFile    = dbpath + '/postgresql.conf' ;
   var exec          = ctlFile ;
   var args          = '' ;
   var timeout       = 600000 ;
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

   try
   {
      var remote = new Remote( hostName, agentPort ) ;
      var cmd    = remote.getCmd() ;

      var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;

      libraryCmd += installPath + '/lib ;' ;
      exec = libraryCmd + exec ;

      //reload
      args = ' reload ' + businessName + ' --print' ;
      var rc = _runRemoteCmd( cmd, exec, args, timeout ) ;
      if ( rc['error'] !== null )
      {
         if ( rc['error'].getErrCode() > 0 )
         {
            result[FIELD_ERRNO]  = SDB_RTN_CONF_NOT_TAKE_EFFECT ;
         }
         else
         {
            result[FIELD_ERRNO]  = rc['error'].getErrCode() ;
         }

         result[FIELD_DETAIL] = rc['error'].getErrMsg() ;
         PD_LOGGER.log( PDERROR, result[FIELD_DETAIL] ) ;

         oma.setIniConfigs( original, configFile, options ) ;
         return result ;
      }

      if ( rc['out'].length > 0 &&
           rc['out'].indexOf( 'invalid value for parameter' ) > 0 )
      {
         var lines = rc['out'].split( "\n" ) ;

         result[FIELD_ERRNO] = SDB_INVALIDARG ;
         result[FIELD_DETAIL] = rc['out'] ;

         for ( var i in lines )
         {
            if ( lines[i].indexOf( 'invalid value for parameter' ) > 0 )
            {
               result[FIELD_DETAIL] = lines[i].replace( 'LOG: ', '' ) ;
               break ;
            }
         }

         oma.setIniConfigs( original, configFile, options ) ;
         return result ;
      }
   }
   catch( e )
   {
      PD_LOGGER.log( PDERROR, e ) ;

      result[FIELD_ERRNO]  = SDB_RTN_CONF_NOT_TAKE_EFFECT ;
      result[FIELD_DETAIL] = getErr( SDB_RTN_CONF_NOT_TAKE_EFFECT ) ;

      oma.setIniConfigs( original, configFile, options ) ;

      return result ;
   }

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   return result ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiadb.js" ) ;
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