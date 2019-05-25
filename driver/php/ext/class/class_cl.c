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

#include "class_cl.h"

extern zend_class_entry *pSequoiadbCursor ;
extern zend_class_entry *pSequoiadbLob ;
extern zend_class_entry *pSequoiadbInt64 ;

extern INT32 connectionDesc ;
extern INT32 clDesc ;
extern INT32 cursorDesc ;
extern INT32 lobDesc ;

PHP_METHOD( SequoiaCL, __construct )
{
}

PHP_METHOD( SequoiaCL, drop )
{
   INT32 rc = SDB_OK ;
   zval *pSequoiadb = NULL ;
   zval *pThisObj = getThis() ;
   CHAR *pCsName = NULL ;
   CHAR *pClName = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl         = SDB_INVALID_HANDLE ;
   CHAR pFullName[ PHP_FULL_NAME_SIZE ] = { 0 } ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   ossMemset( pFullName, 0, PHP_FULL_NAME_SIZE ) ;
   rc = sdbGetCLFullName( cl, pFullName, PHP_FULL_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_split( pFullName, '.', &pCsName, &pClName TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_VAR( pThisObj, "_SequoiaDB", pSequoiadb ) ;
   PHP_READ_HANDLE( pSequoiadb,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   rc = sdbGetCollectionSpace( connection, pCsName, &cs ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDropCollection( cs, pClName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, alter )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbAlterCollection( cl, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &options ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, split )
{
   INT32 rc = SDB_OK ;
   PHP_LONG sourceLen = 0 ;
   PHP_LONG targetLen = 0 ;
   PHP_LONG argsNum   = ZEND_NUM_ARGS() ;
   CHAR *pSource     = NULL ;
   CHAR *pTarget     = NULL ;
   zval *pParameter1 = NULL ;
   zval *pParameter2 = NULL ;
   zval *pThisObj    = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson startCondition ;
   bson endCondition ;
   bson_init( &startCondition ) ;
   bson_init( &endCondition ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "ssz|z",
                            &pSource,
                            &sourceLen,
                            &pTarget,
                            &targetLen,
                            &pParameter1,
                            &pParameter2 ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   if( argsNum == 3 && ( Z_TYPE_P( pParameter1 ) == IS_LONG ||
                         Z_TYPE_P( pParameter1 ) == IS_DOUBLE ) )
   {
      if( Z_TYPE_P( pParameter1 ) == IS_LONG )
      {
         rc = sdbSplitCollectionByPercent( cl,
                                           pSource,
                                           pTarget,
                                           (FLOAT64)Z_LVAL_P( pParameter1 ) ) ;
      }
      else
      {
         rc = sdbSplitCollectionByPercent( cl,
                                           pSource,
                                           pTarget,
                                           Z_DVAL_P( pParameter1 ) ) ;
      }
      if( rc )
      {
         goto error ;
      }
   }
   else
   {
      rc = php_auto2Bson( pParameter1, &startCondition TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = php_auto2Bson( pParameter2, &endCondition TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = sdbSplitCollection( cl,
                               pSource,
                               pTarget,
                               &startCondition,
                               &endCondition ) ;
      if( rc )
      {
         goto error ;
      }
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &startCondition ) ;
   bson_destroy( &endCondition ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, splitAsync )
{
   INT32 rc = SDB_OK ;
   PHP_LONG sourceLen = 0 ;
   PHP_LONG targetLen = 0 ;
   INT32 argsNum     = ZEND_NUM_ARGS() ;
   SINT64 taskID     = 0 ;
   CHAR *pSource     = NULL ;
   CHAR *pTarget     = NULL ;
   zval *pParameter1 = NULL ;
   zval *pParameter2 = NULL ;
   zval *pThisObj    = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson startCondition ;
   bson endCondition ;
   bson_init( &startCondition ) ;
   bson_init( &endCondition ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "ssz|z",
                            &pSource,
                            &sourceLen,
                            &pTarget,
                            &targetLen,
                            &pParameter1,
                            &pParameter2 ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   if( argsNum == 3 && ( Z_TYPE_P( pParameter1 ) == IS_LONG ||
                         Z_TYPE_P( pParameter1 ) == IS_DOUBLE ) )
   {
      if( Z_TYPE_P( pParameter1 ) == IS_LONG )
      {
         rc = sdbSplitCLByPercentAsync( cl,
                                        pSource,
                                        pTarget,
                                        (FLOAT64)Z_LVAL_P( pParameter1 ),
                                        &taskID ) ;
      }
      else
      {
         rc = sdbSplitCLByPercentAsync( cl,
                                        pSource,
                                        pTarget,
                                        Z_DVAL_P( pParameter1 ),
                                        &taskID ) ;
      }
      if( rc )
      {
         goto error ;
      }
   }
   else
   {
      rc = php_auto2Bson( pParameter1, &startCondition TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = php_auto2Bson( pParameter2, &endCondition TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = sdbSplitCLAsync( cl,
                            pSource,
                            pTarget,
                            &startCondition,
                            &endCondition,
                            &taskID ) ;
      if( rc )
      {
         goto error ;
      }
   }
done:
   PHP_RETURN_AUTO_ERROR_TASKID( FALSE, pThisObj, rc, taskID ) ;
   bson_destroy( &startCondition ) ;
   bson_destroy( &endCondition ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, getFullName )
{
   INT32 rc = SDB_OK ;
   CHAR *pFullName    = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   pFullName = (CHAR *)emalloc( PHP_FULL_NAME_SIZE ) ;
   if( !pFullName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pFullName, 0, PHP_FULL_NAME_SIZE ) ;
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbGetCLFullName( cl, pFullName, PHP_FULL_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pFullName, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, getCSName )
{
   INT32 rc = SDB_OK ;
   CHAR *pCsName  = NULL ;
   zval *pThisObj = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   CHAR pFullName[ PHP_FULL_NAME_SIZE ] ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   ossMemset( pFullName, 0, PHP_FULL_NAME_SIZE ) ;
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbGetCLFullName( cl, pFullName, PHP_FULL_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_split( pFullName, '.', &pCsName, NULL TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pCsName, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, getName )
{
   INT32 rc = SDB_OK ;
   CHAR *pClName      = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   pClName = (CHAR *)emalloc( PHP_NAME_SIZE ) ;
   if( !pClName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pClName, 0, PHP_NAME_SIZE ) ;
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbGetCLName( cl, pClName, PHP_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pClName, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, attachCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG fullNameLen = 0 ;
   CHAR *pFullName   = NULL ;
   zval *pOptions    = NULL ;
   zval *pThisObj    = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "sz",
                           &pFullName,
                           &fullNameLen,
                           &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbAttachCollection( cl, pFullName, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &options ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, detachCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG fullNameLen = 0 ;
   CHAR *pFullName   = NULL ;
   zval *pThisObj    = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pFullName, &fullNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbDetachCollection( cl, pFullName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, insert )
{
   INT32 rc = SDB_OK ;
   zval *pRecord = NULL ;
   zval *pThisObj = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson record ;
   bson_iterator id ;
   bson_init( &record ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pRecord ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_auto2Bson( pRecord, &record TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbInsert1( cl, &record, &id ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR_ID( FALSE, pThisObj, rc, id ) ;
   bson_destroy( &record ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, bulkInsert )
{
   INT32 rc = SDB_OK ;
   INT32 i          = 0 ;
   SINT32 insertNum = 0 ;
   SINT32 flags     = 0 ;
   zval *pRecords   = NULL ;
   zval *pFlags     = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson **ppBsonRecords = NULL ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|z", &pRecords, &pFlags ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_assocArray2BsonArray( pRecords,
                                  &ppBsonRecords,
                                  (INT32 *)&insertNum TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pFlags, (INT32 *)&flags TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbBulkInsert( cl, flags, ppBsonRecords, insertNum ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   if( ppBsonRecords )
   {
      for( i = 0; i < insertNum; ++i )
      {
         if( ppBsonRecords[i] )
         {
            bson_destroy( ppBsonRecords[i] ) ;
            efree( ppBsonRecords[i] ) ;
         }
      }
      efree( ppBsonRecords ) ;
   }
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, remove )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson condition ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zz", &pCondition, &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDelete( cl, &condition, &hint ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, update )
{
   INT32 rc = SDB_OK ;
   INT32 flag       = 0 ;
   zval *pRule      = NULL ;
   zval *pCondition = NULL ;
   zval *pHint      = NULL ;
   zval *pFlag      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson rule ;
   bson condition ;
   bson hint ;
   bson_init( &rule ) ;
   bson_init( &condition ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzz",
                            &pRule,
                            &pCondition,
                            &pHint,
                            &pFlag ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pRule, &rule TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbUpdate1( cl, &rule, &condition, &hint, flag ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, upsert )
{
   INT32 rc               = SDB_OK ;
   INT32 flag             = 0 ;
   zval *pRule            = NULL ;
   zval *pCondition       = NULL ;
   zval *pHint            = NULL ;
   zval *pSetOnInsert     = NULL ;
   zval *pFlag            = NULL ;
   zval *pThisObj         = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson rule ;
   bson condition ;
   bson hint ;
   bson setOnInsert ;
   bson_init( &rule ) ;
   bson_init( &condition ) ;
   bson_init( &hint ) ;
   bson_init( &setOnInsert ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzzz",
                            &pRule,
                            &pCondition,
                            &pHint,
                            &pSetOnInsert,
                            &pFlag ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pRule, &rule TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pSetOnInsert, &setOnInsert TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbUpsert2( cl, &rule, &condition, &hint, &setOnInsert, flag ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &hint ) ;
   bson_destroy( &setOnInsert ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, find )
{
   INT32 rc = SDB_OK ;
   INT32 flag         = 0 ;
   INT64 numToSkip    = 0 ;
   INT64 numToReturn  = -1 ;
   zval *pCondition   = NULL ;
   zval *pSelector    = NULL ;
   zval *pOrderBy     = NULL ;
   zval *pHint        = NULL ;
   zval *pNumToSkip   = NULL ;
   zval *pNumToReturn = NULL ;
   zval *pFlag        = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzzzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn,
                            &pFlag ) == FAILURE )
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
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbQuery1( cl,
                   &condition,
                   &selector,
                   &orderBy,
                   &hint,
                   numToSkip,
                   numToReturn,
                   flag,
                   &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
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
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, findAndUpdate )
{
   INT32 rc = SDB_OK ;
   INT32 flag         = 0 ;
   BOOLEAN returnNew  = FALSE ;
   INT64 numToSkip    = 0 ;
   INT64 numToReturn  = -1 ;
   zval *pCondition   = NULL ;
   zval *pSelector    = NULL ;
   zval *pOrderBy     = NULL ;
   zval *pHint        = NULL ;
   zval *pUpdate      = NULL ;
   zval *pNumToSkip   = NULL ;
   zval *pNumToReturn = NULL ;
   zval *pFlag        = NULL ;
   zval *pReturnNew   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson update ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   bson_init( &update ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z|zzzzzzzz",
                            &pUpdate,
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn,
                            &pFlag,
                            &pReturnNew ) == FAILURE )
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
   rc = php_auto2Bson( pUpdate, &update TSRMLS_CC ) ;
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
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Bool( pReturnNew, &returnNew TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbQueryAndUpdate( cl,
                           &condition,
                           &selector,
                           &orderBy,
                           &hint,
                           &update,
                           numToSkip,
                           numToReturn,
                           flag,
                           returnNew,
                           &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   bson_destroy( &update ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, findAndRemove )
{
   INT32 rc = SDB_OK ;
   INT32 flag         = 0 ;
   INT64 numToSkip    = 0 ;
   INT64 numToReturn  = -1 ;
   zval *pCondition   = NULL ;
   zval *pSelector    = NULL ;
   zval *pOrderBy     = NULL ;
   zval *pHint        = NULL ;
   zval *pNumToSkip   = NULL ;
   zval *pNumToReturn = NULL ;
   zval *pFlag        = NULL ;
   zval *pReturnNew   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzzzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn,
                            &pFlag ) == FAILURE )
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
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbQueryAndRemove( cl,
                           &condition,
                           &selector,
                           &orderBy,
                           &hint,
                           numToSkip,
                           numToReturn,
                           flag,
                           &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
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
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, explain )
{
   INT32 rc = SDB_OK ;
   INT32 flag         = 0 ;
   INT64 numToSkip    = 0 ;
   INT64 numToReturn  = -1 ;
   zval *pCondition   = NULL ;
   zval *pSelector    = NULL ;
   zval *pOrderBy     = NULL ;
   zval *pHint        = NULL ;
   zval *pNumToSkip   = NULL ;
   zval *pNumToReturn = NULL ;
   zval *pFlag        = NULL ;
   zval *pOptions     = NULL ;
   zval *pThisObj     = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson options ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zzzzzzzz",
                            &pCondition,
                            &pSelector,
                            &pOrderBy,
                            &pHint,
                            &pNumToSkip,
                            &pNumToReturn,
                            &pFlag,
                            &pOptions ) == FAILURE )
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
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
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
   rc = php_zval2Int( pFlag, &flag TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbExplain( cl,
                    &condition,
                    &selector,
                    &orderBy,
                    &hint,
                    flag,
                    numToSkip,
                    numToReturn,
                    &options,
                    &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &selector ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;
   bson_destroy( &options ) ;
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}


PHP_METHOD( SequoiaCL, count )
{
   INT32 rc = SDB_OK ;
   SINT64 count     = 0 ;
   zval *pCondition = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson condition ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|zz", &pCondition, &pHint ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_auto2Bson( pCondition, &condition TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_auto2Bson( pHint, &hint TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbGetCount1( cl, &condition, &hint, &count ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETURN_INT_OR_LONG( FALSE, pThisObj, pSequoiadbInt64, count ) ;
done:
   bson_destroy( &condition ) ;
   bson_destroy( &hint ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_LONG( -1 ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, aggregate )
{
   INT32 rc = SDB_OK ;
   INT32 i        = 0 ;
   SINT32 num     = 0 ;
   zval *pRecords = NULL ;
   zval *pThisObj = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson **ppBsonRecords = NULL ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|z", &pRecords ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_assocArray2BsonArray( pRecords,
                                  &ppBsonRecords,
                                  (INT32 *)&num TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbAggregate( cl, ppBsonRecords, num, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   if( ppBsonRecords )
   {
      for( i = 0; i < num; ++i )
      {
         if( ppBsonRecords[i] )
         {
            bson_destroy( ppBsonRecords[i] ) ;
            efree( ppBsonRecords[i] ) ;
         }
      }
      efree( ppBsonRecords ) ;
   }
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}



PHP_METHOD( SequoiaCL, createIndex )
{
   INT32 rc = SDB_OK ;
   PHP_LONG indexNameLen  = 0 ;
   INT32 sortBufferSize   = 64 ;
   BOOLEAN isUnique       = FALSE ;
   BOOLEAN isEnforced     = FALSE ;
   CHAR *pIndexName       = NULL ;
   zval *pIndexDef        = NULL ;
   zval *pIsUnique        = NULL ;
   zval *pIsEnforced      = NULL ;
   zval *pSortBufferSize  = NULL ;
   zval *pThisObj         = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson indexDef ;
   bson_init( &indexDef ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "zs|zzz",
                            &pIndexDef,
                            &pIndexName,
                            &indexNameLen,
                            &pIsUnique,
                            &pIsEnforced,
                            &pSortBufferSize ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pIndexDef, &indexDef TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Bool( pIsUnique, &isUnique TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Bool( pIsEnforced, &isEnforced TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pSortBufferSize, &sortBufferSize TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbCreateIndex1( cl,
                         &indexDef,
                         pIndexName,
                         isUnique,
                         isEnforced,
                         sortBufferSize ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   bson_destroy( &indexDef ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, dropIndex )
{
   INT32 rc = SDB_OK ;
   PHP_LONG indexNameLen  = 0 ;
   CHAR *pIndexName       = NULL ;
   zval *pThisObj         = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s", &pIndexName, &indexNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbDropIndex( cl, pIndexName ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, getIndex )
{
   INT32 rc = SDB_OK ;
   PHP_LONG indexNameLen  = 0 ;
   CHAR *pIndexName       = NULL ;
   zval *pThisObj         = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "|s", &pIndexName, &indexNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   if( pIndexName && ossStrlen( pIndexName ) == 0 )
   {
      rc = sdbGetIndexes( cl, NULL, &cursor ) ;
   }
   else
   {
      rc = sdbGetIndexes( cl, pIndexName, &cursor ) ;
   }
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbCursor,
                    cursor,
                    cursorDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, createIdIndex )
{
   INT32 rc = SDB_OK ;
   zval *pOptions         = NULL ;
   zval *pThisObj         = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
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
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbCreateIdIndex( cl, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, dropIdIndex )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbDropIdIndex( cl ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, openLob )
{
   INT32 rc = SDB_OK ;
   INT32 mode      = 0 ;
   PHP_LONG oidLen = 0 ;
   CHAR *pOid      = NULL ;
   zval *pMode     = NULL ;
   zval *pThisObj  = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   bson_oid_t bot ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "sz",
                            &pOid,
                            &oidLen,
                            &pMode ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = php_zval2Int( pMode, &mode TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   bson_oid_from_string( &bot, pOid ) ;
   rc = sdbOpenLob( cl, &bot, mode, &lob ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbLob,
                    lob,
                    lobDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, removeLob )
{
   INT32 rc = SDB_OK ;
   PHP_LONG oidLen = 0 ;
   CHAR *pOid      = NULL ;
   zval *pThisObj  = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson_oid_t bot ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pOid,
                            &oidLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   bson_oid_from_string( &bot, pOid ) ;
   rc = sdbRemoveLob( cl, &bot ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, truncateLob )
{
   INT32 rc = SDB_OK ;
   PHP_LONG oidLen = 0 ;
   INT64 length    = 0 ;
   CHAR *pOid      = NULL ;
   zval *pLength   = NULL ;
   zval *pThisObj  = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson_oid_t bot ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "sz", &pOid, &oidLen, &pLength ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_zval2Long( pLength, &length TSRMLS_CC ) ;
   if ( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj, cl, sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME, clDesc ) ;
   bson_oid_from_string( &bot, pOid ) ;
   rc = sdbTruncateLob( cl, &bot, length ) ;
   if( rc )
   {
      goto error ;
   }

done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, listLob )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
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
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbListLobs( cl, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
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
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCL, listLobPieces )
{
   INT32 rc = SDB_OK ;
   zval *pCondition = NULL ;
   zval *pSelector  = NULL ;
   zval *pOrderBy   = NULL ;
   zval *pHint      = NULL ;
   zval *pThisObj   = getThis() ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
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
                    cl,
                    sdbCollectionHandle,
                    SDB_CL_HANDLE_NAME,
                    clDesc ) ;
   rc = sdbListLobPieces( cl, &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
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
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}
