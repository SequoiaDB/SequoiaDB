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

#include "class_cs.h"

extern zend_class_entry *pSequoiadbCl ;
extern zend_class_entry *pSequoiadbCursor ;

extern INT32 connectionDesc ;
extern INT32 csDesc ;
extern INT32 clDesc ;
extern INT32 cursorDesc ;

PHP_METHOD( SequoiaCS, __construct )
{
}

//cs
PHP_METHOD( SequoiaCS, drop )
{
   INT32 rc = SDB_OK ;
   zval *pSequoiadb = NULL ;
   zval *pThisObj = getThis() ;
   CHAR *pCsName  = NULL ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   pCsName = (CHAR *)emalloc( PHP_NAME_SIZE ) ;
   if( !pCsName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pCsName, 0, PHP_NAME_SIZE ) ;
   rc = sdbGetCSName( cs, pCsName, PHP_NAME_LEN ) ;
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
   rc = sdbDropCollectionSpace( connection, pCsName ) ;
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

PHP_METHOD( SequoiaCS, getName )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   CHAR *pCsName  = NULL ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   pCsName = (CHAR *)emalloc( PHP_NAME_SIZE ) ;
   if( !pCsName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pCsName, 0, PHP_NAME_SIZE ) ;
   rc = sdbGetCSName( cs, pCsName, PHP_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pCsName, 0 ) ;
done:
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_EMPTY_STRING() ;
   goto done ;
}

//cl
//e.g. Rename selectCollection
PHP_METHOD( SequoiaCS, selectCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG clNameLen = 0 ;
   zval *pOptions   = NULL ;
   zval *pThisObj   = getThis() ;
   CHAR *pClName    = NULL ;
   sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s|z",
                            &pClName,
                            &clNameLen,
                            &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbGetCollection1( cs, pClName, &cl ) ;
   if( rc == SDB_DMS_NOTEXIST )
   {
      rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = sdbCreateCollection1( cs, pClName, &options, &cl ) ;
      if( rc )
      {
         goto error ;
      }
   }
   else if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE, pThisObj, pSequoiadbCl, cl, clDesc ) ;
done:
   bson_destroy( &options ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_NULL() ;
   goto done ;
}

PHP_METHOD( SequoiaCS, createCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG clNameLen = 0 ;
   zval *pOptions   = NULL ;
   zval *pThisObj   = getThis() ;
   CHAR *pClName    = NULL ;
   sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "s|z",
                            &pClName,
                            &clNameLen,
                            &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;

   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = sdbCreateCollection1( cs, pClName, &options, &cl ) ;
   if( rc )
   {
      goto error ;
   }

   sdbReleaseCollection( cl ) ;

done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCS, getCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG clNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   CHAR *pClName   = NULL ;
   sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pClName,
                            &clNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbGetCollection1( cs, pClName, &cl ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE, pThisObj, pSequoiadbCl, cl, clDesc ) ;
done:
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_NULL() ;
   goto done ;
}

PHP_METHOD( SequoiaCS, listCL )
{
   INT32 rc                       = SDB_OK ;
   zval *pThisObj                 = getThis() ;
   sdbCSHandle cs                 = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbCSListCollections( cs, &cursor ) ;
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

//e.g. Rename dropCollection
PHP_METHOD( SequoiaCS, dropCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG clNameLen = 0 ;
   zval *pThisObj  = getThis() ;
   CHAR *pClName   = NULL ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "s",
                            &pClName,
                            &clNameLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
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

PHP_METHOD( SequoiaCS, renameCL )
{
   INT32 rc = SDB_OK ;
   PHP_LONG clOldNameLen = 0 ;
   PHP_LONG clNewNameLen = 0 ;
   zval *pThisObj = getThis() ;
   CHAR *pOldName = NULL ;
   CHAR *pNewName = NULL ;
   zval *pOptions = NULL ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   bson options ;

   bson_init( &options ) ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "ss|z", &pOldName, &clOldNameLen,
                                    &pNewName, &clNewNameLen,
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
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;

   rc = sdbRenameCollection( cs, pOldName, pNewName, &options ) ;
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

PHP_METHOD( SequoiaCS, alter )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbAlterCollectionSpace( cs, &options ) ;
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

PHP_METHOD( SequoiaCS, setDomain )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbCSSetDomain( cs, &options ) ;
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

PHP_METHOD( SequoiaCS, getDomainName )
{
   INT32 rc                           = SDB_OK ;
   zval *pThisObj                     = getThis() ;
   CHAR pDomainName [ PHP_NAME_SIZE ] = { 0 } ;
   sdbCSHandle cs                     = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbCSGetDomainName( cs, pDomainName, PHP_NAME_SIZE ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pDomainName, 1 ) ;
done:
   return ;
error:
   RETVAL_NULL() ;	
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCS, removeDomain )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbCSRemoveDomain( cs ) ;
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

PHP_METHOD( SequoiaCS, enableCapped )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbCSEnableCapped( cs ) ;
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

PHP_METHOD( SequoiaCS, disableCapped )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = sdbCSDisableCapped( cs ) ;
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

PHP_METHOD( SequoiaCS, setAttributes )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbCSHandle cs  = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    cs,
                    sdbCSHandle,
                    SDB_CS_HANDLE_NAME,
                    csDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbCSSetAttributes( cs, &options ) ;
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

