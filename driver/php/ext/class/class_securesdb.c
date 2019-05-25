/*******************************************************************************
   Copyright (C) 2012-2014 SequoiaDB Ltd.

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

#include "class_securesdb.h"

extern INT32 connectionDesc ;

PHP_METHOD( SecuresDB, __construct )
{
   INT32 rc = SDB_OK ;
   INT32 argsNum     = ZEND_NUM_ARGS() ;
   PHP_LONG userNameLen = 0 ;
   PHP_LONG passwordLen = 0 ;
   zval *pAddress    = NULL ;
   CHAR *pUserName   = NULL ;
   CHAR *pPassword   = NULL ;
   zval *pThisObj    = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;

   zend_update_property_long( Z_OBJCE_P( pThisObj ),
                              pThisObj,
                              ZEND_STRL( "_return_model" ),
                              1 TSRMLS_CC ) ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if( PHP_GET_PARAMETERS( "|zss",
                           &pAddress,
                           &pUserName,
                           &userNameLen,
                           &pPassword,
                           &passwordLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( argsNum > 0 )
   {
      rc = driver_batch_connect( pAddress,
                                 pUserName,
                                 pPassword,
                                 TRUE,
                                 &connection TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      PHP_SAVE_HANDLE( pThisObj, connection, connectionDesc ) ;
   }

done:
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SecuresDB, connect )
{
   INT32 rc = SDB_OK ;
   PHP_LONG userNameLen = 0 ;
   PHP_LONG passwordLen = 0 ;
   zval *pAddress    = NULL ;
   CHAR *pUserName   = NULL ;
   CHAR *pPassword   = NULL ;
   zval *pThisObj    = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zss",
                            &pAddress,
                            &pUserName,
                            &userNameLen,
                            &pPassword,
                            &passwordLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = driver_batch_connect( pAddress,
                              pUserName,
                              pPassword,
                              TRUE,
                              &connection TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_SAVE_HANDLE( pThisObj, connection, connectionDesc ) ;
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}