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

#ifndef PHP_RESOURCE_H__
#define PHP_RESOURCE_H__

#ifdef __PHP7__

#include "php_resource_7.h"

#else

#include "php_resource_5.h"

#endif

#define SDB_HANDLE_NAME "SequoiaDB Handle"
#define SDB_CS_HANDLE_NAME "SequoiaDB Collection Space Handle"
#define SDB_CL_HANDLE_NAME "SequoiaDB Collection Handle"
#define SDB_CURSOR_HANDLE_NAME  "SequoiaDB Cursor Handle"
#define SDB_GROUP_HANDLE_NAME  "SequoiaDB Group Handle"
#define SDB_NODE_HANDLE_NAME  "SequoiaDB Node Handle"
#define SDB_DOMAIN_HANDLE_NAME  "SequoiaDB Domain Handle"
#define SDB_LOB_HANDLE_NAME  "SequoiaDB Lob Handle"
#define SDB_DATE_HANDLE_NAME  "SequoiaDB Date Handle"
#define SDB_TIMESTAMP_HANDLE_NAME  "SequoiaDB Timestamp Handle"
#define SDB_DECIMAL_HANDLE_NAME  "SequoiaDB Decimal Handle"

void php_sdb_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_cs_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_cl_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_cursor_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_group_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_node_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_domain_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_lob_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_date_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_timestamp_destroy( zend_resource *pRsrc TSRMLS_DC ) ;
void php_decimal_destroy( zend_resource *pRsrc TSRMLS_DC ) ;

#endif