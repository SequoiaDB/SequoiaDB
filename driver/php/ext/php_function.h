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

#ifndef PHP_FUNCTION_H__
#define PHP_FUNCTION_H__

#include "php_driver.h"

#define SDB_DEFAULT_HOSTNAME "127.0.0.1"
#define SDB_DEFAULT_SERVICENAME "11810"

INT32 php_split( CHAR *pStr,
                 CHAR del,
                 CHAR **ppLeft,
                 CHAR **ppRight TSRMLS_DC ) ;

INT32 php_getArrayType( zval *pArray TSRMLS_DC ) ;

/*
INT32 php_indexArrayFind( zval *pArray,
                          UINT32 index,
                          zval **ppValue TSRMLS_DC ) ;
*/

INT32 php_assocArrayFind( zval *pArray,
                          const CHAR *pKey,
                          zval **ppValue TSRMLS_DC ) ;

INT32 php_jsonFind( const CHAR *pStr,
                    const CHAR *pKey,
                    zval **ppValue TSRMLS_DC ) ;

INT32 php_zval2Int( zval *pValue, INT32 *pIntValue TSRMLS_DC ) ;

INT32 php_zval2Bool( zval *pValue, BOOLEAN *pBoolValue TSRMLS_DC ) ;

INT32 php_zval2Long( zval *pValue, INT64 *pLongValue TSRMLS_DC ) ;

INT32 php_assocArray2IntArray( zval *pArray, INT32 **ppIntArray,
                               INT32 *pEleNum TSRMLS_DC ) ;

INT32 php_assocArray2BsonArray( zval *pArray,
                                bson ***pppBsonArray,
                                INT32 *pEleNum TSRMLS_DC ) ;

INT32 php_assocArray2Bson( zval *pArray, bson *pBson TSRMLS_DC ) ;

INT32 php_json2Bson( zval *pJson, bson *pBson TSRMLS_DC ) ;

INT32 php_auto2Bson( zval *pZval, bson *pBson TSRMLS_DC ) ;

INT32 php_bson2Json( bson *pBson, CHAR **ppJson TSRMLS_DC ) ;

INT32 php_bson2Array( bson *pBson, zval **ppArray TSRMLS_DC ) ;


void php_parseNumber( CHAR *pBuffer,
                      INT32 size,
                      INT32 *pNumberType,
                      INT32 *pInt32,
                      INT64 *pInt64,
                      double *pDouble TSRMLS_DC ) ;

BOOLEAN php_date2Time( const CHAR *pDate, INT32 valType,
                       time_t *pTimestamp, INT32 *pMicros ) ;

INT32 driver_connect( CHAR *pAddress,
                      const CHAR *pUserName,
                      const CHAR *pPassword,
                      BOOLEAN useSSL,
                      sdbConnectionHandle *pConnection TSRMLS_DC ) ;

INT32 driver_batch_connect( zval *pAddress,
                            const CHAR *pUserName,
                            const CHAR *pPassword,
                            BOOLEAN useSSL,
                            sdbConnectionHandle *pConnection TSRMLS_DC ) ;

#endif