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
@description: get sequoiadb dialog
@modify list:
   2020-10-30 JiaWen He  Init

@parameter
   var BUS_JSON = { "HostName": "xxx", "svcname": "xxxx" ] } ;

@return
   RET_JSON: the format is: { "Log": "xxxxxxxxxxxx" }
*/

function _getNodeLogPath( PD_LOGGER, hostname, agentPort, svcname )
{
   try
   {
      var oma = new Oma( hostname, agentPort ) ;
      var config = oma.getNodeConfigs( svcname ) ;

      if ( config )
      {
         config = config.toObj() ;
         if ( config[FIELD_DIAGPATH] )
         {
            return catPath( config[FIELD_DIAGPATH], SDB_DIAGLOG_NAME ) ;
         }
         else if( config[FIELD_DBPATH] )
         {
            var path = catPath( config[FIELD_DBPATH], FIELD_DIAGLOG ) ;
            return catPath( path, SDB_DIAGLOG_NAME ) ;
         }
      }
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_OK )
      {
         rc = SDB_SYS ;
         error = new SdbError( rc, e.message ) ;
      }
      else
      {
         error = new SdbError( rc, sprintf( "Failed to get node log path [?:?]",
                                            hostname, svcname ) ) ;
      }
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   var error = new SdbError( "SDB_SYS",
                             sprintf( "Failed to get node log path [?:?]",
                                      hostname, svcname ) ) ;
   PD_LOGGER.log( PDERROR, error ) ;
   throw error ;
}

function _getNodeLog( PD_LOGGER )
{
   var result   = {} ;
   var hostname = BUS_JSON[FIELD_HOSTNAME] ;
   var svcname  = BUS_JSON[FIELD_PORT2] ;
   var maxSize  = 1024 * 1024 ;

   try
   {
      var agentPort = Oma.getAOmaSvcName( hostname ) ;
      var diagpath  = _getNodeLogPath( PD_LOGGER, hostname,
                                       agentPort, svcname ) ;
      var remote    = new Remote( hostname, agentPort ) ;
      var file      = remote.getFile( diagpath ) ;
      var size      = file.getSize( diagpath ) ;
      var readSize  = 0 ;

      if ( size > maxSize )
      {
         file.seek( size - maxSize ) ;
         readSize = maxSize ;
      }
      else
      {
         readSize = size ;
      }

      result[FIELD_LOG] = file.read( readSize ) ;
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_OK )
      {
         rc = SDB_SYS ;
         error = new SdbError( rc, e.message ) ;
      }
      else
      {
         error = new SdbError( rc, sprintf( "Failed to get node log [?:?]",
                                            hostname, svcname ) ) ;
      }
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}

function run()
{
   var result = {} ;

   var PD_LOGGER = new Logger( "sequoiadb.js" ) ;

   return _getNodeLog( PD_LOGGER ) ;
}
