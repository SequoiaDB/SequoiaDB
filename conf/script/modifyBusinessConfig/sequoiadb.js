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
@description: modify sequoiadb config
@modify list:
   2018-11-10 JiaWen He  Init

@parameter
   var BUS_JSON = 

@return
   RET_JSON: the format is: { "errno": 0 }
*/

function _connectSdb( PD_LOGGER, addressList, user, passwd )
{
   var rc = SDB_OK ;
   var db = null ;
   var hostName ;
   var svcname ;

   if( addressList.length == 0 )
   {
      rc = SDB_SYS ;
      var error = new SdbError( rc, "coord address is empty" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   for( var index in addressList )
   {
      try
      {
         rc = SDB_OK ;
         hostName = addressList[index][FIELD_HOSTNAME] ;
         svcname  = addressList[index][FIELD_SVCNAME] ;
         PD_LOGGER.log( PDEVENT, sprintf( "Connect sequoiadb[?:?]",
                                          hostName, svcname ) ) ;
         db = new Sdb( hostName, svcname, user, passwd ) ;
         break ;
      }
      catch( e )
      {
         rc = getLastError() ;
         if( rc == SDB_OK )
         {
            break ;
         }
      }
   }

   if( rc || db == null )
   {
      var error = new SdbError( rc, "Failed to connect coord" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return db ;
}

function _deleteConfig( PD_LOGGER )
{
   var result      = {} ;
   var addressList = BUS_JSON[FIELD_ADDRESS] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var config      = BUS_JSON[FIELD_CONFIG] ;
   var property    = config[FIELD_PROPERTY] ;
   var options     = config[FIELD_OPTIONS2] ;
   var db          = null ;

   db = _connectSdb( PD_LOGGER, addressList, user, passwd ) ;

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   try
   {
      db.deleteConf( property, options ) ;
   }
   catch( e )
   {
      var lastErrObj = getLastErrObj() ;

      result = lastErrObj.toObj() ;

      PD_LOGGER.log( PDERROR, lastErrObj ) ;
   }

   return result ;
}

function _updateConfig( PD_LOGGER )
{
   var result      = {} ;
   var addressList = BUS_JSON[FIELD_ADDRESS] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var config      = BUS_JSON[FIELD_CONFIG] ;
   var property    = config[FIELD_PROPERTY] ;
   var options     = config[FIELD_OPTIONS2] ;
   var db          = null ;

   db = _connectSdb( PD_LOGGER, addressList, user, passwd ) ;

   result[FIELD_ERRNO] = SDB_OK ;
   result[FIELD_DETAIL] = "" ;

   try
   {
      db.updateConf( property, options ) ;
   }
   catch( e )
   {
      var lastErrObj = getLastErrObj() ;

      result = lastErrObj.toObj() ;

      PD_LOGGER.log( PDERROR, lastErrObj ) ;
   }

   return result ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiadb.js" ) ;
   var result = null ;

   var command = BUS_JSON[FIELD_COMMAND] ;

   if ( FIELD_UPDATE_BUSINESS_CONFIG == command )
   {
      result = _updateConfig( PD_LOGGER ) ;
   }
   else if ( FIELD_DELETE_BUSINESS_CONFIG == command )
   {
      result = _deleteConfig( PD_LOGGER ) ;
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