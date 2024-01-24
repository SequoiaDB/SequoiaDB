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

#include "class_domain.h"

extern zend_class_entry *pSequoiadbCursor ;

extern INT32 connectionDesc ;
extern INT32 cursorDesc ;
extern INT32 domainDesc ;

PHP_METHOD( SequoiaDomain, __construct )
{
}

PHP_METHOD( SequoiaDomain, alter )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbAlterDomain( domain, &options ) ;
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

PHP_METHOD( SequoiaDomain, addGroups )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDomainAddGroups( domain, &options ) ;
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

PHP_METHOD( SequoiaDomain, setGroups )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDomainSetGroups( domain, &options ) ;
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

PHP_METHOD( SequoiaDomain, removeGroups )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDomainRemoveGroups( domain, &options ) ;
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

PHP_METHOD( SequoiaDomain, setAttributes )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if ( PHP_GET_PARAMETERS( "z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbDomainSetAttributes( domain, &options ) ;
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

PHP_METHOD( SequoiaDomain, listCS )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
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
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = sdbListCollectionSpacesInDomain( domain, &cursor ) ;
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

PHP_METHOD( SequoiaDomain, listCL )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
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
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = sdbListCollectionsInDomain( domain, &cursor ) ;
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

PHP_METHOD( SequoiaDomain, listGroup )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   bson condition ;
   bson selector ;
   bson orderBy ;
   bson hint ;
   bson_init( &condition ) ;
   bson_init( &selector ) ;
   bson_init( &orderBy ) ;
   bson_init( &hint ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
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
                    domain,
                    sdbDomainHandle,
                    SDB_DOMAIN_HANDLE_NAME,
                    domainDesc ) ;
   rc = sdbListGroupsInDomain( domain, &cursor ) ;
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
