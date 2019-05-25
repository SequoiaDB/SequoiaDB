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

#ifndef PHP_DRIVER7_H__
#define PHP_DRIVER7_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fmpDef.h"
#include "client.h"
#include "php.h"
#include "php_ini.h"
#include "standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "ossUtil.h"
#include "ossMem.h"
#include "cJSON.h"
#include "base64c.h"
#include "php_function.h"
#include "php_resource.h"

#ifdef ZTS
#include "TSRM.h"
#endif

#ifndef PHP_FE_END
#define PHP_FE_END {NULL,NULL,NULL}
#endif

#define PHP_NAME_SIZE 128
#define PHP_NAME_LEN  (PHP_NAME_SIZE-1)

#define PHP_FULL_NAME_SIZE 256
#define PHP_FULL_NAME_LEN  (PHP_FULL_NAME_SIZE-1)

#define PHP_NOT_ARRAY         0
#define PHP_ASSOCIATIVE_ARRAY 1
#define PHP_INDEX_ARRAY       2

#define PHP_NOT_NUMBER 0
#define PHP_IS_INT32   1
#define PHP_IS_INT64   2
#define PHP_IS_DOUBLE  3

#define PHP_DATE        0
#define PHP_TIMESTAMP   1
#define PHP_REGEX       2
#define PHP_OID         3
#define PHP_BINARY      4
#define PHP_MINKEY      5
#define PHP_MAXKEY      6
#define PHP_NODOLLARCMD 7

#define PHP_TIMESTAMP_FORMAT "%d-%d-%d.%d.%d.%d.%d"
#define PHP_DATE_FORMAT      "%d-%d-%d"

#define PHP_INT32_MIN  (-2147483647-1)
#define PHP_INT32_MAX  (2147483647)

#define PHP_UINT32_MAX (4294967295)

#define PHP_LONG size_t

struct phpDate {
   INT64 milli ;
} ;

struct phpTimestamp {
   INT32 second ;
   INT32 micros ;
} ;

/* ========== PHP old function ========== */

#define ZEND_FETCH_RESOURCE_NO_RETURN(rsrc, rsrc_type, passed_id, default_id, resource_type_name, resource_type)\
{\
   rsrc = (rsrc_type) zend_fetch_resource(Z_RES_P(*passed_id), resource_type_name, resource_type) ;\
}

/* ========== PHP Return ========== */

#define PHP_RETVAL_STRING( str, isCopy )\
{\
   RETVAL_STRING( str ) ;\
}

#define PHP_RETURN_STRING( str, isCopy )\
{\
   RETURN_STRING( str ) ;\
}

#define PHP_IS_BOOLEAN( type ) (type == IS_TRUE || type == IS_FALSE)

#define PHP_ZVAL_STRING(z, s, isCopy)\
{\
   ZVAL_STRING( z, s ) ;\
}

#ifndef Z_BVAL_P
#define Z_BVAL_P( val ) (Z_TYPE_P(val)==IS_TRUE ? TRUE : FALSE)
#endif

/* ========== PHP Class ========== */

#define PHP_DECLARE_PROPERTY_NULL( pEntry, name )\
{\
   zend_declare_property_null( pEntry,\
                               ZEND_STRL( name ),\
                               ZEND_ACC_PROTECTED TSRMLS_CC ) ;\
}

#define PHP_DECLARE_PROPERTY_STRING( pEntry, name, value )\
{\
   zend_declare_property_string( pEntry,\
                                 ZEND_STRL( name ),\
                                 value,\
                                 ZEND_ACC_PROTECTED TSRMLS_CC ) ;\
}

#define PHP_DECLARE_PUBLIC_STRING( pEntry, name, value )\
{\
   zend_declare_property_string( pEntry,\
                                 ZEND_STRL( name ),\
                                 value,\
                                 ZEND_ACC_PUBLIC TSRMLS_CC ) ;\
}

#define PHP_DECLARE_PROPERTY_LONG( pEntry, name, value )\
{\
   zend_declare_property_long( pEntry,\
                               ZEND_STRL( name ),\
                               value,\
                               ZEND_ACC_PROTECTED TSRMLS_CC ) ;\
}

#define PHP_REGISTER_LONG_CONSTANT( name, value )\
{\
   REGISTER_LONG_CONSTANT ( name, value, CONST_CS | CONST_PERSISTENT ) ;\
}

#define PHP_REGISTER_INTERNAL( entryIn, enrtyOut)\
{\
   enrtyOut = zend_register_internal_class( &entryIn TSRMLS_CC ) ;\
}

#define PHP_REGISTER_INTERNAL_EX( entryIn1, entryIn2, name, entryOut )\
{\
   entryOut = zend_register_internal_class_ex( &entryIn1,\
                                               entryIn2 TSRMLS_CC ) ;\
}

#define PHP_NEW_CLASS( pZval, classEntry )\
{\
   ALLOC_STD_ZVAL( pZval ) ;\
   object_init_ex( pZval, classEntry ) ;\
}

#define PHP_SAVE_PARENT_CLASS( thisObj, parentClass )\
{\
   zend_update_property( Z_OBJCE_P( thisObj ),\
                         thisObj,\
                         ZEND_STRL( "_SequoiaDB" ),\
                         parentClass TSRMLS_CC ) ;\
}

#define PHP_BUILD_CLASS( isSequoiaDB, thisObj, classEntry, handle, resourceId )\
{\
   zval *pNewClass = NULL ;\
   zval *pSequoiadb = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiadb = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiadb ) ;\
   }\
   PHP_NEW_CLASS( pNewClass, classEntry ) ;\
   PHP_SAVE_HANDLE( pNewClass, handle, resourceId ) ;\
   PHP_SAVE_PARENT_CLASS( pNewClass, pSequoiadb ) ;\
   RETVAL_ZVAL( pNewClass, 0, 0 ) ;\
}

#define PHP_BUILD_INT64( isSequoiaDB, thisObj, classEntry, number )\
{\
   zval *pNewClass = NULL ;\
   zval *pSequoiadb = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiadb = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiadb ) ;\
   }\
   PHP_NEW_CLASS( pNewClass, classEntry ) ;\
   PHP_SAVE_VAR_STRING( pNewClass, "INT64", number ) ;\
   RETVAL_ZVAL( pNewClass, 0, 0 ) ;\
}

#define PHP_RETURN_INT_OR_LONG( isSequoiaDB, thisObj, classEntry, number )\
{\
   if( number < PHP_INT32_MIN || number > PHP_INT32_MAX )\
   {\
      CHAR temp[ 512 ] ;\
      ossMemset( temp, 0, 512 ) ;\
      ossSnprintf( temp, 512, "%lld", (UINT64)number ) ;\
      PHP_BUILD_INT64( isSequoiaDB, thisObj, classEntry, temp )\
   }\
   else\
   {\
      RETVAL_LONG( ((INT32)number) ) ;\
   }\
}

/*
#define PHP_SET_VAR_STRING( thisClass, name, value, isDup )\
{\
   add_property_string( thisClass, name, value, isDup ) ;\
}
*/

#define PHP_SAVE_VAR_INT( thisObj, name, value )\
{\
   zend_update_property_long( Z_OBJCE_P( thisObj ),\
                              thisObj,\
                              ZEND_STRL( name ),\
                              value TSRMLS_CC ) ;\
}

#define PHP_SAVE_VAR_STRING( thisObj, name, value )\
{\
   zend_update_property_string( Z_OBJCE_P( thisObj ),\
                                thisObj,\
                                ZEND_STRL( name ),\
                                value TSRMLS_CC ) ;\
}

#define PHP_READ_VAR( thisObj, name, value )\
{\
   value = zend_read_property( Z_OBJCE_P( thisObj ),\
                               thisObj,\
                               ZEND_STRL( name ),\
                               0, NULL TSRMLS_CC ) ;\
}

/* ========== PHP Function ========== */

#define PHP_GET_PARAMETERS( format, ... ) \
zend_parse_parameters ( ZEND_NUM_ARGS(), format, ##__VA_ARGS__ )


#define PHP_SET_ERROR( isSequoiaDB, thisObj, errno )\
{\
   zval *pSequoiaDB = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiaDB = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiaDB ) ;\
   }\
   zend_update_property_long( Z_OBJCE_P( pSequoiaDB ),\
                              pSequoiaDB,\
                              ZEND_STRL( "_error" ),\
                              errno TSRMLS_CC ) ;\
}

#define PHP_SET_ERRNO_OK( isSequoiaDB, thisObj )\
{\
   PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OK ) ;\
}

#define PHP_RETURN_AUTO_ERROR( isSequoiaDB, thisObj, errno )\
{\
   zval *pSequoiaDB = NULL ;\
   zval *pReturnModel = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiaDB = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiaDB ) ;\
   }\
   PHP_READ_VAR( pSequoiaDB, "_return_model", pReturnModel ) ;\
   if( Z_LVAL_P( pReturnModel ) == 1 )\
   {\
      array_init( return_value ) ;\
      add_assoc_long( return_value, "errno", errno ) ;\
   }\
   else\
   {\
      if( errno == SDB_OK )\
      {\
         PHP_RETURN_STRING( "{\"error\":0}", 1 ) ;\
      }\
      else\
      {\
         CHAR *pErrorMsg = (CHAR *)emalloc( 64 ) ;\
         if( !pErrorMsg )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OOM ) ;\
            PHP_RETVAL_STRING( "{\"error\":-2}", 1 ) ;\
         }\
         else\
         {\
            ossMemset( pErrorMsg, 0, 64 ) ;\
            ossSnprintf( pErrorMsg, 64, "{\"error\":%d}", errno ) ;\
            PHP_RETVAL_STRING( pErrorMsg, 0 ) ;\
            efree( pErrorMsg ) ;\
         }\
      }\
   }\
}

#define PHP_RETURN_AUTO_ERROR_ID( isSequoiaDB, thisObj, errno, id )\
{\
   zval *pSequoiaDB = NULL ;\
   zval *pReturnModel = NULL ;\
   CHAR pId[25] = { 0 } ;\
   ossMemset( pId, 0, 25 ) ;\
   if( isSequoiaDB )\
   {\
      pSequoiaDB = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiaDB ) ;\
   }\
   PHP_READ_VAR( pSequoiaDB, "_return_model", pReturnModel ) ;\
   if( Z_LVAL_P( pReturnModel ) == 1 )\
   {\
      array_init( return_value ) ;\
      add_assoc_long( return_value, "errno", errno ) ;\
      if( errno == SDB_OK )\
      {\
         bson_oid_to_string( bson_iterator_oid( &id ), pId ) ;\
         add_assoc_string( return_value, "_id", pId ) ;\
      }\
   }\
   else\
   {\
      if( errno == SDB_OK )\
      {\
         CHAR *pSuccessMsg = (CHAR *)emalloc( 128 ) ;\
         if( !pSuccessMsg )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OOM ) ;\
            PHP_RETVAL_STRING( "{\"error\":-2}", 1 ) ;\
         }\
         else\
         {\
            bson_oid_to_string( bson_iterator_oid( &id ), pId ) ;\
            ossMemset( pSuccessMsg, 0, 128 ) ;\
            ossSnprintf( pSuccessMsg, 128, "{\"error\":%d,\"_id\":\"%s\"}", errno, pId ) ;\
            PHP_RETVAL_STRING( pSuccessMsg, 0 ) ;\
            efree( pSuccessMsg ) ;\
         }\
      }\
      else\
      {\
         CHAR *pErrorMsg = (CHAR *)emalloc( 64 ) ;\
         if( !pErrorMsg )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OOM ) ;\
            PHP_RETVAL_STRING( "{\"error\":-2}", 1 ) ;\
         }\
         else\
         {\
            ossMemset( pErrorMsg, 0, 64 ) ;\
            ossSnprintf( pErrorMsg, 64, "{\"error\":%d}", errno ) ;\
            PHP_RETVAL_STRING( pErrorMsg, 0 ) ;\
            efree( pErrorMsg ) ;\
         }\
      }\
   }\
}

#define PHP_RETURN_AUTO_RECORD( isSequoiaDB, thisObj, isEmpty, record )\
{\
   CHAR *pJson = NULL ;\
   zval *pSequoiaDB = NULL ;\
   zval *pReturnModel = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiaDB = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiaDB ) ;\
   }\
   PHP_READ_VAR( pSequoiaDB, "_return_model", pReturnModel ) ;\
   if( Z_LVAL_P( pReturnModel ) == 1 )\
   {\
      array_init( return_value ) ;\
      if( isEmpty == FALSE )\
      {\
         rc = php_bson2Array( &record, &return_value TSRMLS_CC ) ;\
         if( rc )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, rc ) ;\
            array_init( return_value ) ;\
         }\
      }\
   }\
   else\
   {\
      if( isEmpty )\
      {\
         RETVAL_EMPTY_STRING() ;\
      }\
      else\
      {\
         rc = php_bson2Json( &record, &pJson TSRMLS_CC ) ;\
         if( rc )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, rc ) ;\
            RETVAL_EMPTY_STRING() ;\
         }\
         else\
         {\
            PHP_RETVAL_STRING( pJson, 1 ) ;\
         }\
      }\
   }\
}

#define PHP_RETURN_AUTO_ERROR_TASKID( isSequoiaDB, thisObj, errno, taskId )\
{\
   CHAR *pStr = NULL ;\
   zval *pClass = NULL ;\
   zval *pSequoiaDB = NULL ;\
   zval *pReturnModel = NULL ;\
   if( isSequoiaDB )\
   {\
      pSequoiaDB = thisObj ;\
   }\
   else\
   {\
      PHP_READ_VAR( thisObj, "_SequoiaDB", pSequoiaDB ) ;\
   }\
   PHP_READ_VAR( pSequoiaDB, "_return_model", pReturnModel ) ;\
   if( Z_LVAL_P( pReturnModel ) == 1 )\
   {\
      array_init( return_value ) ;\
      add_assoc_long( return_value, "errno", errno ) ;\
      if( errno == SDB_OK )\
      {\
         PHP_NEW_CLASS( pClass, pSequoiadbInt64 ) ;\
         pStr = (CHAR *)emalloc( 512 ) ;\
         if( !pStr )\
         {\
            array_init( return_value ) ;\
            add_assoc_long( return_value, "errno", SDB_OOM ) ;\
         }\
         else\
         {\
            ossMemset( pStr, 0, 512 ) ;\
            ossSnprintf( pStr, 512, "%lld", (UINT64)taskId ) ;\
            PHP_SAVE_VAR_STRING( pClass, "INT64", pStr ) ;\
            add_assoc_zval( return_value, "taskID", pClass ) ;\
            efree( pStr ) ;\
         }\
      }\
   }\
   else\
   {\
      if( errno == SDB_OK )\
      {\
         CHAR *pSuccessMsg = (CHAR *)emalloc( 128 ) ;\
         if( !pSuccessMsg )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OOM ) ;\
            PHP_RETVAL_STRING( "{\"error\":-2}", 1 ) ;\
         }\
         else\
         {\
            ossMemset( pSuccessMsg, 0, 128 ) ;\
            ossSnprintf( pSuccessMsg, 128, "{\"error\":%d,\"taskID\":%lld}", errno, (UINT64)taskId ) ;\
            PHP_RETVAL_STRING( pSuccessMsg, 0 ) ;\
            efree( pSuccessMsg ) ;\
         }\
      }\
      else\
      {\
         CHAR *pErrorMsg = (CHAR *)emalloc( 64 ) ;\
         if( !pErrorMsg )\
         {\
            PHP_SET_ERROR( isSequoiaDB, thisObj, SDB_OOM ) ;\
            PHP_RETVAL_STRING( "{\"error\":-2}", 1 ) ;\
         }\
         else\
         {\
            ossMemset( pErrorMsg, 0, 64 ) ;\
            ossSnprintf( pErrorMsg, 64, "{\"error\":%d}", errno ) ;\
            PHP_RETVAL_STRING( pErrorMsg, 0 ) ;\
            efree( pErrorMsg ) ;\
         }\
      }\
   }\
}

#define PHP_RETURN_OPTIONS( thisObj )\
{\
   zval *pReturnModel = NULL ;\
   PHP_READ_VAR( thisObj, "_return_model", pReturnModel ) ;\
   if( Z_LVAL_P( pReturnModel ) == 1 )\
   {\
      array_init( return_value ) ;\
      add_assoc_bool( return_value, "install", TRUE ) ;\
   }\
   else\
   {\
      PHP_RETVAL_STRING( "{ \"install\": false }", 1 ) ;\
   }\
}

/* ========== PHP Array ========== */

#define PHP_ARRAY_FOREACH_START( pTable )\
for( zend_hash_internal_pointer_reset( pTable ) ; \
   HASH_KEY_NON_EXISTENT != zend_hash_get_current_key_type( pTable ) ; \
   zend_hash_move_forward( pTable ) )

#define PHP_ARRAY_FOREACH_END()

#define PHP_ARRAY_FOREACH_VALUE( pTable, value )\
{\
   value = zend_hash_get_current_data( pTable ) ;\
}

#define PHP_ARRAY_FOREACH_KEY( pTable, key )\
{\
   zend_ulong tempIndex = 0 ;\
   zend_string *pTmpKey = NULL ;\
   INT32 tempType = zend_hash_get_current_key( pTable, &pTmpKey, &tempIndex ) ;\
   if( tempType == HASH_KEY_IS_LONG )\
   {\
      key = (CHAR *)emalloc( 64 ) ;\
      if( key )\
      {\
         ossMemset( key, 0, 64 ) ;\
         ossSnprintf( key, 64, "%d", tempIndex ) ;\
      }\
   }\
   else\
   {\
      key = ZSTR_VAL( pTmpKey ) ;\
   }\
}

#endif