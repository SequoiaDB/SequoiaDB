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

#include "class_sequoiadb.h"

extern zend_class_entry *pSequoiadbCs ;
extern zend_class_entry *pSequoiadbCl ;
extern zend_class_entry *pSequoiadbCursor ;
extern zend_class_entry *pSequoiadbDomain ;
extern zend_class_entry *pSequoiadbGroup ;
extern zend_class_entry *pSequoiadbNode ;
extern zend_class_entry *pSequoiadbInt64 ;

extern INT32 connectionDesc ;
extern INT32 csDesc ;
extern INT32 clDesc ;
extern INT32 cursorDesc ;
extern INT32 domainDesc ;
extern INT32 groupDesc ;
extern INT32 nodeDesc ;

PHP_METHOD( SequoiaDB, __construct )
{
   INT32 rc = SDB_OK ;
   INT32 argsNum     = ZEND_NUM_ARGS() ;
   PHP_LONG userNameLen = 0 ;
   PHP_LONG passwordLen = 0 ;
   BOOLEAN useSSL    = FALSE ;
   zval *pAddress    = NULL ;
   CHAR *pUserName   = NULL ;
   CHAR *pPassword   = NULL ;
   zval *pThisObj    = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "|zssb",
                           &pAddress,
                           &pUserName,
                           &userNameLen,
                           &pPassword,
                           &passwordLen,
                           &useSSL ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( argsNum > 0 )
   {
      rc = driver_batch_connect( pAddress,
                                 pUserName,
                                 pPassword,
                                 useSSL,
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

PHP_METHOD( SequoiaDB, install )
{
   INT32 rc = SDB_OK ;
   zval *pOptions    = NULL ;
   zval *pValue      = NULL ;
   zval *pThisObj    = getThis() ;
   BOOLEAN install   = FALSE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( pOptions )
   {
      if( Z_TYPE_P( pOptions ) == IS_ARRAY &&
          php_getArrayType( pOptions TSRMLS_CC ) == PHP_ASSOCIATIVE_ARRAY )
      {
         if( php_assocArrayFind( pOptions,
                                 "install",
                                 &pValue TSRMLS_CC ) == SUCCESS )
         {
            if( PHP_IS_BOOLEAN( Z_TYPE_P( pValue ) ) == FALSE )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            install = Z_BVAL_P( pValue ) ;
            zend_update_property_long( Z_OBJCE_P( pThisObj ),
                                       pThisObj,
                                       ZEND_STRL( "_return_model" ),
                                       install TSRMLS_CC ) ;
         }
      }
      else if( Z_TYPE_P( pOptions ) == IS_STRING )
      {
         MAKE_STD_ZVAL( pValue ) ;

         rc = php_jsonFind( Z_STRVAL_P( pOptions ),
                            "install",
                            &pValue TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
         if( Z_TYPE_P( pValue ) != IS_NULL )
         {
            if( PHP_IS_BOOLEAN( Z_TYPE_P( pValue ) ) == FALSE )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            install = Z_BVAL_P( pValue ) ;
            zend_update_property_long( Z_OBJCE_P( pThisObj ),
                                       pThisObj,
                                       ZEND_STRL( "_return_model" ),
                                       install TSRMLS_CC ) ;
         }
      }
      else if( Z_TYPE_P( pOptions ) == IS_NULL )
      {
         goto done ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
done:
   PHP_RETURN_OPTIONS( pThisObj ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, getLastErrorMsg )
{
   INT32 rc = SDB_OK ;
   INT32 errNum = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson record ;
   bson_iterator bsonItr ;

   bson_init( &record ) ;

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   if ( SDB_INVALID_HANDLE != connection )
   {
      rc = sdbGetLastErrorObj( connection, &record ) ;
      if ( SDB_OK == rc )
      {
         if ( bson_is_empty( &record ) )
         {
            rc = SDB_DMS_EOC ;
         }
         else
         {
            bson_type bType = bson_find( &bsonItr, &record, "errno" ) ;

            if ( BSON_INT == bType )
            {
               errNum = bson_iterator_int( &bsonItr ) ;
            }
         }
      }
   }

   if ( SDB_OK == errNum )
   {
      zval *pError = NULL ;

      PHP_READ_VAR( pThisObj, "_error", pError ) ;
      errNum = Z_LVAL_P( pError ) ;

      PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, errNum ) ;
      goto done ;
   }

   PHP_RETURN_AUTO_RECORD( TRUE, pThisObj, ( rc == SDB_OK ? FALSE : TRUE ),
                           record ) ;

done:
   bson_destroy( &record ) ;
}

PHP_METHOD( SequoiaDB, cleanLastErrorMsg )
{
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   sdbCleanLastErrorObj( connection ) ;
}

//db
PHP_METHOD( SequoiaDB, connect )
{
   INT32 rc = SDB_OK ;
   PHP_LONG userNameLen = 0 ;
   PHP_LONG passwordLen = 0 ;
   BOOLEAN useSSL    = FALSE ;
   zval *pAddress    = NULL ;
   CHAR *pUserName   = NULL ;
   CHAR *pPassword   = NULL ;
   zval *pThisObj    = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zssb",
                            &pAddress,
                            &pUserName,
                            &userNameLen,
                            &pPassword,
                            &passwordLen,
                            &useSSL ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = driver_batch_connect( pAddress,
                              pUserName,
                              pPassword,
                              useSSL,
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

PHP_METHOD( SequoiaDB, close )
{
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   sdbDisconnect( connection ) ;
}

PHP_METHOD( SequoiaDB, isValid )
{
   BOOLEAN result = FALSE ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   result = sdbIsValid( connection ) ;

done:
   RETVAL_BOOL( result ) ;
   return ;
error:
   goto done ;
}

PHP_METHOD( SequoiaDB, syncDB )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbSyncDB( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }

done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, invalidateCache )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson condition ;
   bson_init( &condition ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "|z", &pCondition ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbInvalidateCache( connection, &condition ) ;
   if( rc )
   {
      goto error ;
   }

done:
   bson_destroy( &condition ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, interrupt )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbInterrupt ( connection ) ;
   if( rc )
   {
      goto error ;
   }

done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, interruptOperation )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbInterruptOperation ( connection ) ;
   if( rc )
   {
      goto error ;
   }

done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//snapshot & list
//e.g. Rename getSnapshot
PHP_METHOD( SequoiaDB, snapshot )
{
   INT32 rc = SDB_OK ;
   INT32 type       = 0 ;
   zval *pType      = NULL ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pNumToSkip = NULL ;
   zval *pNumToReturn = NULL ;
   INT64 numToSkip  = 0 ;
   INT64 numToReturn = -1 ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzzzzz",
                            &pType,
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_zval2Int( pType, &type TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Long( pNumToSkip, &numToSkip TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Long( pNumToReturn, &numToReturn TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbGetSnapshot1( connection,
                         type,
                         &condition,
                         &selector,
                         &orderBy,
                         &hint,
                         numToSkip,
                         numToReturn,
                         &cursor ) ;

   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, resetSnapshot )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbResetSnapshot( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//e.g. Rename getList
PHP_METHOD( SequoiaDB, list )
{
   INT32 rc = SDB_OK ;
   INT32 type        = 0 ;
   INT64 numToSkip   = 0 ;
   INT64 numToReturn = -1 ;
   zval *pType      = NULL ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pNumToSkip   = NULL ;
   zval *pNumToReturn = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;

   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "z|zzzzzz",
                            &pType,
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_zval2Int( pType, &type TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_zval2Long( pNumToSkip, &numToSkip TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_zval2Long( pNumToReturn, &numToReturn TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbGetList1( connection,
                     type,
                     &condition,
                     &selector,
                     &orderBy,
                     &hint,
                     numToSkip,
                     numToReturn,
                     &cursor ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//cs
//e.g. Rename listCSs
PHP_METHOD( SequoiaDB, listCS )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListCollectionSpaces( connection, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, selectCS )
{
   INT32 rc = SDB_OK ;
   PHP_LONG csNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   zval *pOptions  = NULL ;
   CHAR *pCsName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s|z",
                            &pCsName,
                            &csNameLen,
                            &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetCollectionSpace( connection, pCsName, &cs ) ;
   if( rc == SDB_DMS_CS_NOTEXIST )
   {
      if( pOptions && Z_TYPE_P( pOptions ) == IS_LONG )
      {
         rc = sdbCreateCollectionSpace( connection, pCsName, Z_LVAL_P( pOptions ), &cs ) ;
         if( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
         rc = sdbCreateCollectionSpaceV2( connection, pCsName, &options, &cs ) ;
         if( rc )
         {
            goto error ;
         }
      }
   }
   else if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCs,
                    cs,
                    csDesc ) ;
done:
   bson_destroy( &options ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, createCS )
{
   INT32 rc = SDB_OK ;
   PHP_LONG csNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   zval *pOptions  = NULL ;
   CHAR *pCsName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s|z",
                            &pCsName,
                            &csNameLen,
                            &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbCreateCollectionSpaceV2( connection, pCsName, &options, &cs ) ;
   if( rc )
   {
      goto error ;
   }
   sdbReleaseCS( cs ) ;
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, getCS )
{
   INT32 rc = SDB_OK ;
   PHP_LONG csNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   CHAR *pCsName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pCsName,
                            &csNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetCollectionSpace( connection, pCsName, &cs ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCs,
                    cs,
                    csDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//e.g. Rename dropCollectionSpace
PHP_METHOD( SequoiaDB, dropCS )
{
   INT32 rc = SDB_OK ;
   PHP_LONG csNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   CHAR *pCsName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pCsName,
                            &csNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbDropCollectionSpace( connection, pCsName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, renameCS )
{
   INT32 rc = SDB_OK ;
   PHP_LONG csOldNameLen = 0 ;
   PHP_LONG csNewNameLen = 0 ;
   zval *pThisObj = getThis() ;
   CHAR *pOldName = NULL ;
   CHAR *pNewName = NULL ;
   zval *pOptions = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;

   bson_init( &options ) ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "ss|z", &pOldName, &csOldNameLen,
                                    &pNewName, &csNewNameLen,
                                    &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbRenameCollectionSpace( connection, pOldName, pNewName, &options ) ;
   if( rc )
   {
      goto error ;
   }

done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//cl
//e.g. Rename listCollections
PHP_METHOD( SequoiaDB, listCL )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListCollections( connection, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, getCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG fullNameLen = 0 ;
   zval *pThisObj    = getThis() ;
   CHAR *pFullName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pFullName,
                            &fullNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetCollection( connection, pFullName, &cl ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCl,
                    cl,
                    clDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, truncate )
{
   INT32 rc = SDB_OK ;
   PHP_LONG fullNameLen = 0 ;
   zval *pThisObj    = getThis() ;
   zval *pCL         = NULL ;
   CHAR *pFullName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pFullName,
                            &fullNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbTruncateCollection( connection, pFullName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//domain
//e.g. Rename listDomains
PHP_METHOD( SequoiaDB, listDomain )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   /* 预留参数
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   */
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListDomains( connection,
                       &condition,
                       &selector,
                       &orderBy,
                       &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, createDomain )
{
   INT32 rc = SDB_OK ;
   PHP_LONG domainNameLen = 0 ;
   zval *pThisObj      = getThis() ;
   zval *pOptions      = NULL ;
   CHAR *pDomainName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s|z",
                            &pDomainName,
                            &domainNameLen,
                            &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbCreateDomain( connection, pDomainName, &options, &domain ) ;
   if( rc )
   {
      goto error ;
   }
   sdbReleaseDomain( domain ) ;
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, getDomain )
{
   INT32 rc = SDB_OK ;
   PHP_LONG domainNameLen = 0 ;
   zval *pThisObj      = getThis() ;
   CHAR *pDomainName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pDomainName,
                            &domainNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetDomain( connection, pDomainName, &domain ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbDomain,
                    domain,
                    domainDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, dropDomain )
{
   INT32 rc = SDB_OK ;
   PHP_LONG domainNameLen = 0 ;
   zval *pThisObj      = getThis() ;
   CHAR *pDomainName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pDomainName,
                            &domainNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbDropDomain( connection, pDomainName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//group
PHP_METHOD( SequoiaDB, listGroup )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   /* 预留参数
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   */
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListReplicaGroups( connection, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//e.g. Rename selectGroup
PHP_METHOD( SequoiaDB, getGroup )
{
   INT32 rc = SDB_OK ;
   PHP_LONG groupNameLen = 0 ;
   zval *pThisObj     = getThis() ;
   CHAR *pGroupName   = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbReplicaGroupHandle group    = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pGroupName,
                            &groupNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetReplicaGroup( connection, pGroupName, &group ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbGroup,
                    group,
                    groupDesc ) ;
   PHP_SAVE_VAR_STRING( return_value, "_name", pGroupName ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}


PHP_METHOD( SequoiaDB, createGroup )
{
   INT32 rc = SDB_OK ;
   PHP_LONG groupNameLen  = 0 ;
   zval *pThisObj      = getThis() ;
   CHAR *pGroupName    = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pGroupName,
                            &groupNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbCreateReplicaGroup( connection, pGroupName, &domain ) ;
   if( rc )
   {
      goto error ;
   }
   sdbReleaseReplicaGroup( domain ) ;
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, removeGroup )
{
   INT32 rc = SDB_OK ;
   PHP_LONG groupNameLen  = 0 ;
   zval *pThisObj      = getThis() ;
   CHAR *pGroupName    = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pGroupName,
                            &groupNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbRemoveReplicaGroup( connection, pGroupName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, createCataGroup )
{
   INT32 rc = SDB_OK ;
   PHP_LONG hostNameLen     = 0 ;
   PHP_LONG serviceNameLen  = 0 ;
   PHP_LONG databasePathLen = 0 ;
   CHAR *pHostName       = NULL ;
   CHAR *pServiceName    = NULL ;
   CHAR *pDatabasePath   = NULL ;
   zval *pConfigure      = NULL ;
   zval *pThisObj        = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson configure ;
   bson_init( &configure ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "sss|z",
                           &pHostName,
                           &hostNameLen,
                           &pServiceName,
                           &serviceNameLen,
                           &pDatabasePath,
                           &databasePathLen,
                           &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pConfigure, &configure TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbCreateReplicaCataGroup( connection,
                                   pHostName,
                                   pServiceName,
                                   pDatabasePath,
                                   &configure ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configure ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//sql
PHP_METHOD( SequoiaDB, execSQL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG sqlLen = 0 ;
   CHAR *pSQL     = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pSQL, &sqlLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbExec( connection, pSQL, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, execUpdateSQL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG sqlLen = 0 ;
   CHAR *pSQL     = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pSQL, &sqlLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbExecUpdate( connection, pSQL ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//user
PHP_METHOD( SequoiaDB, createUser )
{
   INT32 rc = SDB_OK ;
   PHP_LONG userLen = 0 ;
   PHP_LONG pwdLen  = 0 ;
   CHAR *pUser    = NULL ;
   CHAR *pPwd     = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "ss",
                            &pUser,
                            &userLen,
                            &pPwd,
                            &pwdLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbCreateUsr( connection, pUser, pPwd ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, removeUser )
{
   INT32 rc = SDB_OK ;
   PHP_LONG userLen = 0 ;
   PHP_LONG pwdLen  = 0 ;
   CHAR *pUser    = NULL ;
   CHAR *pPwd     = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "ss",
                            &pUser,
                            &userLen,
                            &pPwd,
                            &pwdLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbRemoveUsr( connection, pUser, pPwd ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//config
PHP_METHOD( SequoiaDB, flushConfigure )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbFlushConfigure( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//update config
PHP_METHOD( SequoiaDB, updateConfig )
{
   INT32 rc = SDB_OK ;
   zval *pConfigs = NULL ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson configs ;
   bson options ;
   bson_init( &configs ) ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|z", &pConfigs, &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pConfigs, &configs TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbUpdateConfig( connection, &configs, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configs ) ;
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//delete config
PHP_METHOD( SequoiaDB, deleteConfig )
{
   INT32 rc = SDB_OK ;
   zval *pConfigs = NULL ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson configs ;
   bson options ;
   bson_init( &configs ) ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|z", &pConfigs, &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pConfigs, &configs TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDeleteConfig( connection, &configs, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configs ) ;
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//procedure
PHP_METHOD( SequoiaDB, listProcedure )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   /* 预留参数
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   */
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListProcedures( connection, &condition, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, createJsProcedure )
{
   INT32 rc = SDB_OK ;
   PHP_LONG codeLen = 0 ;
   CHAR *pCode    = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pCode, &codeLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbCrtJSProcedure( connection, pCode ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, removeProcedure )
{
   INT32 rc = SDB_OK ;
   PHP_LONG nameLen = 0 ;
   CHAR *pName    = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pName, &nameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbRmProcedure( connection, pName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, evalJs )
{
/*
   INT32 rc = SDB_OK ;
   PHP_LONG codeLen = 0 ;
   SDB_SPD_RES_TYPE type = SDB_SPD_RES_TYPE_VOID ;
   CHAR *pCode    = NULL ;
   zval *pThisObj = getThis() ;
   const CHAR *pErrMsg = "" ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson errMsg ;
   bson next ;
   bson_init( &errMsg ) ;
   bson_init( &next ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pCode, &codeLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbEvalJS( connection, pCode, &type, &cursor, &errMsg ) ;
   if( rc )
   {
      bson_iterator it ;
      bson_type iteType = bson_find( &it, &errMsg, FMP_ERR_MSG ) ;
      if( iteType == BSON_STRING )
      {
         pErrMsg = bson_iterator_string( &it ) ;
      }
      goto error ;
   }
   switch( type )
   {
   case FMP_RES_TYPE_VOID:
      break ;
   case FMP_RES_TYPE_STR:
   case FMP_RES_TYPE_NUMBER:
   case FMP_RES_TYPE_OBJ:
   case FMP_RES_TYPE_BOOL:
      {
         bson_iterator it ;
         bson_type iteType = BSON_EOO ;
         rc = sdbNext( cursor, &next ) ;
         if( rc )
         {
            goto error ;
         }
         iteType = bson_find( &it, &next, FMP_RES_VALUE ) ;
         if( iteType == BSON_STRING )
         {
            const CHAR *pString = bson_iterator_string( &it ) ;
            PHP_RETVAL_STRING( pString, 1 ) ;
         }
         else if( iteType == BSON_INT )
         {
            RETVAL_LONG( bson_iterator_int( &it ) ) ;
         }
         else if( iteType == BSON_LONG )
         {
            PHP_RETURN_INT_OR_LONG( TRUE, pThisObj, pSequoiadbInt64, bson_iterator_long( &it ) ) ;
         }
         else if( iteType == BSON_DOUBLE )
         {
            RETVAL_DOUBLE( bson_iterator_double( &it ) ) ;
         }
         else if( iteType == BSON_OBJECT )
         {
            bson_iterator_subobject( &it, &next ) ;
            PHP_RETURN_AUTO_RECORD( TRUE,
                                    pThisObj,
                                    FALSE,
                                    next ) ;
         }
         else if( iteType == BSON_BOOL )
         {
            RETVAL_BOOL( bson_iterator_bool( &it ) ) ;
         }
         else
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      break ;
   case FMP_RES_TYPE_RECORDSET:
      PHP_BUILD_CLASS( TRUE,
                       pThisObj,
                       pSequoiadbCursor,
                       cursor,
                       cursorDesc ) ;
      break ;
   case FMP_RES_TYPE_CS:
   case FMP_RES_TYPE_CL:
   case FMP_RES_TYPE_RG:
   case FMP_RES_TYPE_RN:
      {
         bson_iterator it ;
         bson_type iteType = BSON_EOO ;
         const CHAR *pName = NULL ;
         rc = sdbNext( cursor, &next ) ;
         if( rc )
         {
            goto error ;
         }
         iteType = bson_find( &it, &next, FMP_RES_VALUE ) ;
         if( iteType != BSON_STRING )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         pName = bson_iterator_string( &it ) ;
         if( type == FMP_RES_TYPE_CS )
         {
            sdbCSHandle cs = SDB_INVALID_HANDLE ;
            rc = sdbGetCollectionSpace( connection, pName, &cs ) ;
            if( rc )
            {
               goto error ;
            }
            PHP_BUILD_CLASS( TRUE,
                             pThisObj,
                             pSequoiadbCs,
                             cs,
                             csDesc ) ;
         }
         else if( type == FMP_RES_TYPE_CL )
         {
            sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
            rc = sdbGetCollection( connection, pName, &cl ) ;
            if( rc )
            {
               goto error ;
            }
            PHP_BUILD_CLASS( TRUE,
                             pThisObj,
                             pSequoiadbCl,
                             cl,
                             clDesc ) ;
         }
         else if( type == FMP_RES_TYPE_RG )
         {
            sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
            rc = sdbGetReplicaGroup( connection, pName, &group ) ;
            if( rc )
            {
               goto error ;
            }
            PHP_BUILD_CLASS( TRUE,
                             pThisObj,
                             pSequoiadbGroup,
                             group,
                             groupDesc ) ;
         }
         else if( type == FMP_RES_TYPE_RN )
         {
            sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
            sdbNodeHandle node          = SDB_INVALID_HANDLE ;
            const CHAR *pGroupName = NULL ;
            const CHAR *pNodeName  = NULL ;
            pGroupName = pName ;
            pNodeName  = ossStrchr( (CHAR *)pName, ':' ) ;
            if( pNodeName == NULL || pNodeName == pName )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
            pNodeName = '\0' ;
            ++pNodeName ;
            if( ossStrchr( (CHAR *)pNodeName, ':' ) == NULL )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
            rc = sdbGetReplicaGroup( connection, pGroupName, &group ) ;
            if( rc )
            {
               goto error ;
            }
            rc = sdbGetNodeByName( group, pNodeName, &node ) ;
            if( rc )
            {
               sdbReleaseReplicaGroup( group ) ;
               goto error ;
            }
            sdbReleaseReplicaGroup( group ) ;
            PHP_BUILD_CLASS( TRUE,
                             pThisObj,
                             pSequoiadbNode,
                             node,
                             nodeDesc ) ;
         }
      }
      break ;
   default:
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   
done:
   bson_destroy( &errMsg ) ;
   bson_destroy( &next ) ;
   return ;
error:
   PHP_RETVAL_STRING( pErrMsg, 1 ) ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
*/
}

//transaction
PHP_METHOD( SequoiaDB, transactionBegin )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbTransactionBegin( connection ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, transactionCommit )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbTransactionCommit( connection ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, transactionRollback )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbTransactionRollback( connection ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//backup
PHP_METHOD( SequoiaDB, backup )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbBackupOffline( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, listBackup )
{
   INT32 rc = SDB_OK ;
   zval *pOptions   = NULL ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson options ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &options ) ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|zzzz",
                            &pOptions,
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   /* 预留参数
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   */
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListBackup( connection,
                       &options,
                       &condition,
                       &selector,
                       &orderBy,
                       &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &options ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, removeBackup )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbRemoveBackup( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//task
PHP_METHOD( SequoiaDB, listTask )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSelector, &selector TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOrderBy, &orderBy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbListTasks( connection,
                      &condition,
                      &selector,
                      &orderBy,
                      &hint,
                      &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( TRUE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, waitTask )
{
   INT32 rc = SDB_OK ;
   SINT32 taskNum     = 0 ;
   zval *pTaskID      = NULL ;
   zval *pThisObj     = getThis() ;
   SINT64 *pTaskArray = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pTaskID ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   if( Z_TYPE_P( pTaskID ) == IS_ARRAY )
   {
      HashTable *pTable = NULL ;
      INT32 arrayType = php_getArrayType( pTaskID TSRMLS_CC ) ;
      if( arrayType == PHP_ASSOCIATIVE_ARRAY || arrayType == PHP_NOT_ARRAY )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pTable = HASH_OF( pTaskID ) ;
      if( pTable )
      {
         taskNum = zend_hash_num_elements( pTable ) ;
         if( taskNum > 0 )
         {
            SINT32 i = 0 ;
            zval *pValue = NULL ;
            pTaskArray = (SINT64 *)emalloc( sizeof( SINT64 ) * taskNum ) ;
            if( !pTaskArray )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            PHP_ARRAY_FOREACH_START( pTable )
            {
               PHP_ARRAY_FOREACH_VALUE( pTable, pValue ) ;
               if( i >= taskNum )
               {
                  break ;
               }
               rc = php_zval2Long( pValue,
                                   &pTaskArray[i] TSRMLS_CC ) ;
               if( rc )
               {
                  goto error ;
               }
               ++i ;
            }
            PHP_ARRAY_FOREACH_END()
            rc = sdbWaitTasks( connection, pTaskArray, taskNum ) ;
            if( rc )
            {
               goto error ;
            }
         }
      }
   }
   else
   {
      SINT64 taskID = 0 ;
      rc = php_zval2Long( pTaskID, &taskID TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = sdbWaitTasks( connection, &taskID, 1 ) ;
      if( rc )
      {
         goto error ;
      }
   }
done:
   if( pTaskArray )
   {
      efree( pTaskArray ) ;
   }
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, cancelTask )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isAsync = TRUE ;
   zval *pTaskID   = NULL ;
   zval *pThisObj  = getThis() ;
   SINT64 taskID   = 0 ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|b", &pTaskID, &isAsync ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_zval2Long( pTaskID, &taskID TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbCancelTask( connection, taskID, isAsync ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

//session
PHP_METHOD( SequoiaDB, setSessionAttr )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbSetSessionAttr( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, getSessionAttr )
{
   INT32 rc = SDB_OK ;
   BOOLEAN useCache = TRUE ;
   zval *pUseCache  = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson result ;

   bson_init( &result ) ;

   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "|z", &pUseCache ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_zval2Bool( pUseCache, &useCache TSRMLS_CC ) ;
   if ( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbGetSessionAttrEx( connection, useCache, &result ) ;

done:
   PHP_RETURN_AUTO_RECORD( TRUE,
                           pThisObj,
                           (rc == SDB_OK ? FALSE : TRUE),
                           result ) ;
   bson_destroy( &result ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, forceSession )
{
   INT32 rc = SDB_OK ;
   SINT64 sessionID = 0 ;
   zval *pSessionID = NULL ;
   zval *pOptions   = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;

   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|z", &pSessionID, &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_zval2Long( pSessionID, &sessionID TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbForceSession( connection, sessionID, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaDB, analyze )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( TRUE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;

   rc = sdbAnalyze( connection, &options ) ;
   if( rc )
   {
      goto error ;
   }

done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( TRUE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( TRUE, pThisObj, rc ) ;
   goto done ;
}
