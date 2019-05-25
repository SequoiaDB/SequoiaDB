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

#include "php_resource.h"

void php_sdb_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbConnectionHandle connection = (sdbConnectionHandle)pRsrc->ptr ;
   sdbDisconnect( connection ) ;
   sdbReleaseConnection( connection ) ;
}

void php_cs_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbCSHandle cs = (sdbCSHandle)pRsrc->ptr ;
   sdbReleaseCS( cs ) ;
}

void php_cl_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbCollectionHandle cl = (sdbCollectionHandle)pRsrc->ptr ;
   sdbReleaseCollection( cl ) ;
}

void php_cursor_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbCursorHandle cursor = (sdbCursorHandle)pRsrc->ptr ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
}

void php_group_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbReplicaGroupHandle group = (sdbCollectionHandle)pRsrc->ptr ;
   sdbReleaseReplicaGroup( group ) ;
}

void php_node_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbNodeHandle node = (sdbNodeHandle)pRsrc->ptr ;
   sdbReleaseNode( node ) ;
}

void php_domain_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbDomainHandle domain = (sdbCursorHandle)pRsrc->ptr ;
   sdbReleaseDomain( domain ) ;
}

void php_lob_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   sdbLobHandle lob = (sdbLobHandle)pRsrc->ptr ;
   sdbCloseLob( &lob ) ;
}

void php_date_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   struct phpDate *pDate = (struct phpDate *)pRsrc->ptr ;
   efree( pDate ) ;
}

void php_timestamp_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   struct phpTimestamp *pTimestamp = (struct phpTimestamp *)pRsrc->ptr ;
   efree( pTimestamp ) ;
}

void php_decimal_destroy( zend_resource *pRsrc TSRMLS_DC )
{
   bson_decimal *pBsonDecimal = (bson_decimal *)pRsrc->ptr ;
   if( pBsonDecimal )
   {
      decimal_free( pBsonDecimal ) ;
   }
   efree( pBsonDecimal ) ;
}