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

#include "php_function.h"
#include "timestamp.h"

extern zend_class_entry *pSequoiadbInt64 ;
extern zend_class_entry *pSequoiadbId ;
extern zend_class_entry *pSequoiadbData ;
extern zend_class_entry *pSequoiadbTimeStamp ;
extern zend_class_entry *pSequoiadbRegex ;
extern zend_class_entry *pSequoiadbBinary ;
extern zend_class_entry *pSequoiadbMinKey ;
extern zend_class_entry *pSequoiadbMaxKey ;
extern zend_class_entry *pSequoiadbDecimal ;

extern INT32 dateDesc ;
extern INT32 timestampDesc ;
extern INT32 decimalDesc ;

#define FUN_CONST_STR_COPY_NEW( pStrSource, pStrDest )\
{\
   INT32 sourceLen = ossStrlen( pStrSource ) ;\
   pStrDest = (CHAR *)emalloc( sourceLen + 1 ) ;\
   if( !pStrDest )\
   {\
      rc = SDB_OOM ;\
      goto error ;\
   }\
   ossMemset( pStrDest, 0, sourceLen + 1 ) ;\
   ossMemcpy( pStrDest, pStrSource, sourceLen ) ;\
}

#define IS_CLASS( object, classEntry ) instanceof_function(Z_OBJCE_P(object),classEntry TSRMLS_CC)

CHAR *php_strDup( const CHAR *pSource, INT32 sourceLen TSRMLS_DC )
{
   CHAR *pDest = (CHAR *)emalloc( sourceLen + 1 ) ;
   if( pDest )
   {
      ossMemset( pDest, 0, sourceLen + 1 ) ;
      ossMemcpy( pDest, pSource, sourceLen ) ;
   }
   return pDest ;
}

INT32 php_split( CHAR *pStr,
                 CHAR del,
                 CHAR **ppLeft,
                 CHAR **ppRight TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   CHAR *pDel = NULL ;
   if( !pStr )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   pDel = ossStrchr( pStr, del ) ;
   if( !pDel )
   {
      if( ppLeft )
      {
         *ppLeft = pStr ;
      }
      goto done ;
   }
   if( ppLeft )
   {
      *ppLeft = php_strDup( pStr, ( pDel - pStr ) TSRMLS_CC ) ;
      if( *ppLeft == NULL )
      {
         rc = SDB_OOM ;
         goto error ;
      }
   }
   if( ppRight )
   {
      *ppRight = php_strDup( pDel + 1, ossStrlen( pDel + 1 ) TSRMLS_CC ) ;
      if( *ppRight == NULL )
      {
         rc = SDB_OOM ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 php_getArrayType( zval *pArray TSRMLS_DC )
{
   if( Z_TYPE_P( pArray ) == IS_ARRAY )
   {
      HashTable *pTable = HASH_OF( pArray ) ;
      if( pTable && zend_hash_num_elements( pTable ) > 0 )
      {
         INT32 type = PHP_INDEX_ARRAY ;
         INT32 keyType = HASH_KEY_NON_EXISTENT ;
         zend_hash_internal_pointer_reset( pTable ) ;
         for( ; ; zend_hash_move_forward( pTable ) )
         {
            keyType = zend_hash_get_current_key_type( pTable ) ;
            if( keyType == HASH_KEY_NON_EXISTENT )
            {
               break ;
            }
            else if( keyType == HASH_KEY_IS_STRING )
            {
               type = PHP_ASSOCIATIVE_ARRAY ;
               break ;
            }
         }
         return type ;
      }
      return PHP_INDEX_ARRAY ;
   }
   else
   {
      return PHP_NOT_ARRAY ;
   }
}

/*
INT32 php_indexArrayFind( zval *pArray,
                          UINT32 index,
                          zval **ppValue TSRMLS_DC )
{
}
*/

INT32 php_assocArrayFind( zval *pArray,
                          const CHAR *pKey,
                          zval **ppValue TSRMLS_DC )
{
   INT32 keyLength       = ossStrlen( pKey ) ;
   INT32 CursorKeyLength = 0 ;
   CHAR *pCursorKey      = NULL ;
   zval *pCursorVal      = NULL ;
   HashTable *pTable     = HASH_OF( pArray ) ;
   pTable = HASH_OF( pArray ) ;
   PHP_ARRAY_FOREACH_START( pTable )
   {
      PHP_ARRAY_FOREACH_KEY( pTable, pCursorKey ) ;
      CursorKeyLength = ossStrlen( pCursorKey ) ;
      if( keyLength == CursorKeyLength &&
          ossStrcmp( pKey, pCursorKey ) == 0 )
      {
         PHP_ARRAY_FOREACH_VALUE( pTable, pCursorVal ) ;
         *ppValue = pCursorVal ;
         return SUCCESS ;
      }
   }
   PHP_ARRAY_FOREACH_END()
   return FAILURE ;
}

INT32 php_jsonFind( const CHAR *pStr,
                    const CHAR *pKey,
                    zval **ppValue TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 keyLen = ossStrlen( pKey ) ;
   INT32 intVal = 0 ;
   INT64 longVal = 0 ;
   FLOAT64 doubleVal = 0 ;
   CJSON_VALUE_TYPE cJsonType = CJSON_NONE ;
   CJSON_MACHINE *pMachine = cJsonCreate() ;
   const cJson_iterator *pIter = NULL ;
   const CHAR *pJsonKey = NULL ;
   const CHAR *pString = NULL ;
   zval *pValue = *ppValue ;

   ZVAL_NULL( pValue ) ;

   if( pMachine == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   cJsonInit( pMachine, CJSON_RIGOROUS_PARSE, TRUE ) ;
   if( cJsonParse( pStr, pMachine ) == FALSE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pIter = cJsonIteratorInit( pMachine ) ;
   if( pIter == NULL )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   while( cJsonIteratorMore( pIter ) )
   {
      pJsonKey = cJsonIteratorKey( pIter ) ;
      if( !strncmp( pJsonKey, pKey, keyLen ) )
      {
         cJsonType = cJsonIteratorType( pIter ) ;
         switch( cJsonType )
         {
         case CJSON_STRING:
            pString = cJsonIteratorString( pIter ) ;
            PHP_ZVAL_STRING( pValue, pString, 1 ) ;
            break ;
         case CJSON_TRUE:
            ZVAL_BOOL( pValue, 1 ) ;
            break ;
         case CJSON_FALSE:
            ZVAL_BOOL( pValue, 0 ) ;
            break ;
         case CJSON_INT32:
            intVal = cJsonIteratorInt32( pIter ) ;
            ZVAL_LONG( pValue, intVal ) ;
            break ;
         case CJSON_INT64:
            longVal = cJsonIteratorInt64( pIter ) ;
            ZVAL_LONG( pValue, (INT32)longVal ) ;
            break ;
         case CJSON_DOUBLE:
            doubleVal = cJsonIteratorDouble( pIter ) ;
            ZVAL_DOUBLE( pValue, doubleVal ) ;
            break ;
         default:
            rc = SDB_INVALIDARG ;
            break ;
         }
         break ;
      }
      cJsonIteratorNext( pIter ) ;
   }
done:
   cJsonRelease( pMachine ) ;
   return rc ;
error:
   goto done ;
}

INT32 php_zval2Int( zval *pValue, INT32 *pIntValue TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( pValue )
   {
      if( Z_TYPE_P( pValue ) == IS_LONG )
      {
         *pIntValue = Z_LVAL_P( pValue ) ;
      }
      else if( Z_TYPE_P( pValue ) == IS_STRING )
      {
         const CHAR *pTmp = Z_STRVAL_P( pValue ) ;

         if( isdigit( pTmp[0] ) )
         {
            *pIntValue = ossAtoi( pTmp ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }
      else if( Z_TYPE_P( pValue ) == IS_OBJECT &&
               IS_CLASS( pValue, pSequoiadbInt64 ) )
      {
         zval *pNumber = NULL ;
         PHP_READ_VAR( pValue, "INT64", pNumber ) ;
         *pIntValue = ossAtoi( Z_STRVAL_P( pNumber ) ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   return rc ;
}

INT32 php_zval2Bool( zval *pValue, BOOLEAN *pBoolValue TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( pValue )
   {
      if( Z_TYPE_P( pValue ) == IS_LONG )
      {
         *pBoolValue = Z_LVAL_P( pValue ) == 0 ? FALSE : TRUE ;
      }
      else if( Z_TYPE_P( pValue ) == IS_STRING )
      {
         *pBoolValue = Z_STRLEN_P( pValue ) == 0 ? FALSE : TRUE ;
      }
      else if( PHP_IS_BOOLEAN( Z_TYPE_P( pValue ) ) == TRUE )
      {
         *pBoolValue = Z_BVAL_P( pValue ) == FALSE ? FALSE : TRUE ;
      }
      else if( Z_TYPE_P( pValue ) == IS_DOUBLE )
      {
         *pBoolValue = Z_DVAL_P( pValue ) == 0.0 ? FALSE : TRUE ;
      }
      else if( Z_TYPE_P( pValue ) == IS_NULL )
      {
         *pBoolValue = FALSE ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   return rc ;
}

INT32 php_zval2Long( zval *pValue, INT64 *pLongValue TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( pValue )
   {
      if( Z_TYPE_P( pValue ) == IS_LONG )
      {
         *pLongValue = (INT64)Z_LVAL_P( pValue ) ;
      }
      else if( Z_TYPE_P( pValue ) == IS_STRING )
      {
         const CHAR *pTmp = Z_STRVAL_P( pValue ) ;

         if( isdigit( pTmp[0] ) )
         {
            *pLongValue = ossAtoll( pTmp ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }
      else if( Z_TYPE_P( pValue ) == IS_OBJECT &&
               IS_CLASS( pValue, pSequoiadbInt64 ) )
      {
         zval *pNumber = NULL ;
         PHP_READ_VAR( pValue, "INT64", pNumber ) ;
         *pLongValue = ossAtoll( Z_STRVAL_P( pNumber ) ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   return rc ;
}

INT32 _object2Bson( const CHAR *pKey, zval *pValue, bson *pBson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( IS_CLASS( pValue, pSequoiadbInt64 ) )
   {
      INT64 numLong = 0 ;
      rc = php_zval2Long( pValue, &numLong TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
      rc = bson_append_long( pBson, pKey, numLong ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbId ) )
   {
      bson_oid_t oid ;
      zval *pOid = NULL ;
      PHP_READ_VAR( pValue, "$oid", pOid ) ;
      bson_oid_from_string( &oid, Z_STRVAL_P( pOid ) ) ;
      rc = bson_append_oid( pBson, pKey, &oid ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbData ) )
   {
      struct phpDate *pDate = NULL ;
      PHP_READ_RESOURCE( pValue,
                         "$date",
                         pDate,
                         struct phpDate *,
                         SDB_DATE_HANDLE_NAME,
                         dateDesc ) ;
      if( pDate == NULL )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = bson_append_date( pBson, pKey, (bson_date_t)pDate->milli ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbTimeStamp ) )
   {
      time_t second = 0 ;
      INT32 micros = 0 ;
      struct phpTimestamp *pTimestamp = NULL ;
      PHP_READ_RESOURCE( pValue,
                         "$timestamp",
                         pTimestamp,
                         struct phpTimestamp*,
                         SDB_TIMESTAMP_HANDLE_NAME,
                         timestampDesc ) ;
      if( pTimestamp == NULL )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      second = (time_t)pTimestamp->second ;
      micros = pTimestamp->micros ;
      rc = bson_append_timestamp2( pBson, pKey, (INT32)second, micros ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbRegex ) )
   {
      zval *pRegex   = NULL ;
      zval *pOptions = NULL ;
      PHP_READ_VAR( pValue, "$regex", pRegex ) ;
      PHP_READ_VAR( pValue, "$options", pOptions ) ;
      rc = bson_append_regex( pBson,
                              pKey,
                              Z_STRVAL_P( pRegex ),
                              Z_STRVAL_P( pOptions ) ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbBinary ) )
   {
      INT32 binDataSize = 0 ;
      CHAR *pBinData    = NULL ;
      zval *pBinary     = NULL ;
      zval *pType       = NULL ;
      PHP_READ_VAR( pValue, "$binary", pBinary ) ;
      PHP_READ_VAR( pValue, "$type", pType ) ;
      binDataSize = getDeBase64Size( Z_STRVAL_P( pBinary ) ) ;
      if( binDataSize < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if( binDataSize > 0 )
      {
         pBinData = (CHAR *)emalloc( binDataSize ) ;
         if( !pBinData )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( pBinData, 0, binDataSize ) ;
         if( base64Decode( Z_STRVAL_P( pBinary ), pBinData, binDataSize ) < 0 )
         {
            efree( pBinData ) ;
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         rc = bson_append_binary( pBson,
                                  pKey,
                                  (CHAR)Z_LVAL_P( pType ),
                                  pBinData,
                                  binDataSize - 1 ) ;
         if( rc != BSON_OK )
         {
            efree( pBinData ) ;
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         efree( pBinData ) ;
      }
      else
      {
         rc = bson_append_binary( pBson,
                                  pKey,
                                  (CHAR)Z_LVAL_P( pType ),
                                  "",
                                  0 ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbMinKey ) )
   {
      rc = bson_append_minkey( pBson, pKey ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbMaxKey ) )
   {
      rc = bson_append_maxkey( pBson, pKey ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if( IS_CLASS( pValue, pSequoiadbDecimal ) )
   {
      bson_decimal *pBsonDecimal = NULL ;
      PHP_READ_RESOURCE( pValue,
                         "$decimal",
                         pBsonDecimal,
                         bson_decimal*,
                         SDB_DECIMAL_HANDLE_NAME,
                         decimalDesc ) ;
      if( pBsonDecimal == NULL )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = bson_append_decimal( pBson,
                                pKey,
                                (const bson_decimal *)pBsonDecimal ) ;
      if( rc != BSON_OK )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _assocArray2Bson( zval *pArray, bson *pBson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 valueType = IS_NULL ;
   INT32 arrayType = PHP_NOT_ARRAY ;
   INT32 dollarType = PHP_NODOLLARCMD ;
   HashTable *pTable = NULL ;
   pTable = HASH_OF( pArray ) ;
   PHP_ARRAY_FOREACH_START( pTable )
   {
      CHAR *pKey = NULL ;
      zval *pValue = NULL ;
      PHP_ARRAY_FOREACH_VALUE( pTable, pValue ) ;
      PHP_ARRAY_FOREACH_KEY( pTable, pKey ) ;
      valueType = Z_TYPE_P( pValue ) ;
      switch( valueType )
      {
      case IS_NULL:
         rc = bson_append_null( pBson, pKey ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
      case IS_LONG:
         rc = bson_append_int( pBson, pKey, Z_LVAL_P( pValue ) ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
      case IS_DOUBLE:
         rc = bson_append_double( pBson, pKey, Z_DVAL_P( pValue ) ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
#ifdef __PHP7__
      case IS_TRUE:
         rc = bson_append_bool( pBson, pKey, TRUE ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
      case IS_FALSE:
         rc = bson_append_bool( pBson, pKey, FALSE ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
#else
      case IS_BOOL:
         rc = bson_append_bool( pBson, pKey, Z_BVAL_P( pValue ) ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
#endif
      case IS_STRING:
         rc = bson_append_string( pBson, pKey, Z_STRVAL_P( pValue ) ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         break ;
      case IS_ARRAY:
         arrayType = php_getArrayType( pValue TSRMLS_CC ) ;
         if( arrayType == PHP_ASSOCIATIVE_ARRAY )
         {
            rc = bson_append_start_object( pBson, pKey ) ;
            if( rc != BSON_OK )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
            rc = _assocArray2Bson( pValue, pBson TSRMLS_CC ) ;
            if( rc )
            {
               goto error ;
            }
            rc = bson_append_finish_object( pBson ) ;
            if( rc != BSON_OK )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
         else if( arrayType == PHP_INDEX_ARRAY )
         {
            rc = bson_append_start_array( pBson, pKey ) ;
            if( rc != BSON_OK )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
            rc = _assocArray2Bson( pValue, pBson TSRMLS_CC ) ;
            if( rc )
            {
               goto error ;
            }
            rc = bson_append_finish_array( pBson ) ;
            if( rc != BSON_OK )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
         break ;
      case IS_OBJECT:
         rc = _object2Bson( pKey, pValue, pBson TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
         break ;
      }
   }
   PHP_ARRAY_FOREACH_END()
done:
   return rc ;
error:
   goto done ;
}

INT32 php_assocArray2IntArray( zval *pArray, INT32 **ppIntArray,
                               INT32 *pEleNum TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 eleNum      = 0 ;
   INT32 i           = 0 ;
   INT32 arrayType = PHP_NOT_ARRAY ;
   INT32 *pIntArray = NULL ;
   HashTable *pTable = NULL ;
   zval *pValue = NULL ;

   if( !pArray )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( IS_ARRAY != Z_TYPE_P( pArray ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   arrayType = php_getArrayType( pArray TSRMLS_CC ) ;
   if( PHP_INDEX_ARRAY != arrayType )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pTable = HASH_OF( pArray ) ;
   if( pTable )
   {
      eleNum = zend_hash_num_elements( pTable ) ;
      if( eleNum > 0 )
      {
         pIntArray = (INT32 *)emalloc( sizeof( INT32 ) * eleNum ) ;
         if( !pIntArray )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         *pEleNum = eleNum ;

         i = 0 ;
         PHP_ARRAY_FOREACH_START( pTable )
         {
            PHP_ARRAY_FOREACH_VALUE( pTable, pValue ) ;
            if( i >= eleNum )
            {
               goto done ;
            }

            rc = php_zval2Int( pValue, &pIntArray[i] TSRMLS_CC ) ;
            if( rc )
            {
               goto error ;
            }

            ++i ;
         }
         PHP_ARRAY_FOREACH_END()
      }
   }

done:
   *ppIntArray = pIntArray ;
   return rc ;
error:
   goto done ;
}

INT32 php_assocArray2BsonArray( zval *pArray,
                                bson ***pppBsonArray,
                                INT32 *pEleNum TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 eleNum      = 0 ;
   INT32 i           = 0 ;
   INT32 arrayType   = PHP_NOT_ARRAY ;
   HashTable *pTable = NULL ;
   zval *pValue = NULL ;
   bson **ppBsonArray = NULL ;
   *pEleNum = 0 ;
   if( pArray )
   {
      if( Z_TYPE_P( pArray ) == IS_ARRAY )
      {
         arrayType = php_getArrayType( pArray TSRMLS_CC ) ;
         if( arrayType == PHP_ASSOCIATIVE_ARRAY )
         {
            ppBsonArray = (bson **)emalloc( sizeof( bson * ) ) ;
            if( !ppBsonArray )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            ppBsonArray[0] = (bson *)emalloc( sizeof( bson ) ) ;
            if( !ppBsonArray[0] )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            *pEleNum = 1 ;
            rc = php_assocArray2Bson( pArray, ppBsonArray[0] TSRMLS_CC ) ;
            if( rc )
            {
               goto error ;
            }
         }
         else if( arrayType == PHP_INDEX_ARRAY )
         {
            pTable = HASH_OF( pArray ) ;
            if( pTable )
            {
               eleNum = zend_hash_num_elements( pTable ) ;
               if( eleNum > 0 )
               {
                  ppBsonArray = (bson **)emalloc( sizeof( bson * ) * eleNum ) ;
                  if( !ppBsonArray )
                  {
                     rc = SDB_OOM ;
                     goto error ;
                  }
                  *pEleNum = 0 ;
                  for( i = 0; i < eleNum; ++i )
                  {
                     ppBsonArray[i] = (bson *)emalloc( sizeof( bson ) ) ;
                     if( !ppBsonArray[i] )
                     {
                        rc = SDB_OOM ;
                        goto error ;
                     }
                     bson_init( ppBsonArray[i] ) ;
                     *pEleNum = i + 1 ;
                  }
                  i = 0 ;
                  PHP_ARRAY_FOREACH_START( pTable )
                  {
                     PHP_ARRAY_FOREACH_VALUE( pTable, pValue ) ;
                     if( i >= eleNum )
                     {
                        goto done ;
                     }
                     rc = php_auto2Bson( pValue,
                                         ppBsonArray[i] TSRMLS_CC ) ;
                     if( rc )
                     {
                        goto error ;
                     }
                     bson_finish( ppBsonArray[i] ) ;
                     ++i ;
                  }
                  PHP_ARRAY_FOREACH_END()
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if( Z_TYPE_P( pArray ) == IS_STRING )
      {
         ppBsonArray = (bson **)emalloc( sizeof( bson * ) ) ;
         if( !ppBsonArray )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ppBsonArray[0] = (bson *)emalloc( sizeof( bson ) ) ;
         if( !ppBsonArray[0] )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         *pEleNum = 1 ;
         rc = php_json2Bson( pArray, ppBsonArray[0] TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done:
   *pppBsonArray = ppBsonArray ;
   return rc ;
error:
   goto done ;
}

INT32 php_assocArray2Bson( zval *pArray, bson *pBson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 arrayType = PHP_NOT_ARRAY ;
   HashTable *pTable = NULL ;
   if( !pBson )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bson_init( pBson ) ;
   if( pArray )
   {
      if( Z_TYPE_P( pArray ) == IS_ARRAY )
      {
         rc = _assocArray2Bson( pArray, pBson TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      pBson = bson_empty( pBson ) ;
   }
done:
   if( pBson )
   {
      bson_finish( pBson ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 php_json2Bson( zval *pJson, bson *pBson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( !pBson )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bson_init( pBson ) ;
   if( pJson )
   {
      if( Z_TYPE_P( pJson ) == IS_STRING )
      {
         if( !jsonToBson( pBson, Z_STRVAL_P( pJson ) ) )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else
      {
         pBson = bson_empty( pBson ) ;
      }
   }
   else
   {
      pBson = bson_empty( pBson ) ;
   }
done:
   if( pBson )
   {
      bson_finish( pBson ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 php_auto2Bson( zval *pZval, bson *pBson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   bson_init( pBson ) ;
   if( pZval )
   {
      if( Z_TYPE_P( pZval ) == IS_STRING )
      {
         rc = php_json2Bson( pZval, pBson TSRMLS_CC ) ;
      }
      else if( Z_TYPE_P( pZval ) == IS_ARRAY )
      {
         rc = php_assocArray2Bson( pZval, pBson TSRMLS_CC ) ;
      }
      else if( Z_TYPE_P( pZval ) == IS_NULL )
      {
         pBson = bson_empty( pBson ) ;
         bson_finish( pBson ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   else
   {
      pBson = bson_empty( pBson ) ;
      bson_finish( pBson ) ;
   }
   return rc ;
}

INT32 php_bson2Json( bson *pBson, CHAR **ppJson TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 jsonLen = bson_sprint_length( pBson ) * 2 ;
   CHAR *pJson = NULL ;
   if( jsonLen <= 0 )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   pJson = (CHAR *)emalloc( jsonLen ) ;
   if( !pJson )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pJson, 0, jsonLen ) ;
   if( !bsonToJson( pJson, jsonLen, pBson, FALSE, TRUE ) )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   *ppJson = pJson ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _bson2Array( const CHAR *pBsonBuf,
                   zval *pArray,
                   BOOLEAN isObj TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 strValLen = 0 ;
   INT32 strValLen2 = 0 ;
   time_t timep = 0 ;
   const CHAR *pKey = NULL ;
   const CHAR *pStrVal = NULL ;
   CHAR *pStrVal2 = NULL ;
   zval *pChild = NULL ;
   zval *pClass = NULL ;
   struct phpDate *pDriverDate = NULL ;
   struct phpTimestamp *pDriverTimestamp = NULL ;
   bson_decimal *pDecimal = NULL ;
   bson_timestamp_t ts ;
   bson_type bsonType ;
   bson_iterator item ;
   CHAR type = 0 ;
   CHAR oidHex[25] ;
   bson_iterator_from_buffer( &item, pBsonBuf ) ;
   while( bson_iterator_next( &item ) )
   {
      bsonType = bson_iterator_type( &item ) ;
      if( isObj )
      {
         pKey = bson_iterator_key( &item ) ;
      }
      if( bsonType == BSON_UNDEFINED )
      {
         continue ;
      }
      switch( bsonType )
      {
      case BSON_INT:
         if( isObj )
         {
            add_assoc_long( pArray, pKey, bson_iterator_int( &item ) ) ;
         }
         else
         {
            add_next_index_long( pArray, bson_iterator_int( &item ) ) ;
         }
         break ;
      case BSON_LONG:
         PHP_NEW_CLASS( pClass, pSequoiadbInt64 ) ;
         pStrVal2 = (CHAR *)emalloc( 512 ) ;
         if( !pStrVal2 )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemset( pStrVal2, 0, 512 ) ;
         ossSnprintf( pStrVal2, 512, "%lld", (UINT64)bson_iterator_long( &item ) ) ;
         PHP_SAVE_VAR_STRING( pClass, "INT64", pStrVal2 ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_DOUBLE:
         if( isObj )
         {
            add_assoc_double( pArray, pKey, bson_iterator_double( &item ) ) ;
         }
         else
         {
            add_next_index_double( pArray, bson_iterator_double( &item ) ) ;
         }
         break ;
      case BSON_STRING:
         pStrVal = bson_iterator_string( &item ) ;
         FUN_CONST_STR_COPY_NEW( pStrVal, pStrVal2 ) ;
#ifdef __PHP7__
         if( isObj )
         {
            add_assoc_string( pArray,
                              pKey,
                              pStrVal2 ) ;
         }
         else
         {
            add_next_index_string( pArray, pStrVal2 ) ;
         }
#else
         if( isObj )
         {
            add_assoc_string( pArray,
                              pKey,
                              pStrVal2,
                              0 ) ;
         }
         else
         {
            add_next_index_string( pArray, pStrVal2, 0 ) ;
         }
#endif
         break ;
      case BSON_BOOL:
         if( isObj )
         {
            add_assoc_bool( pArray, pKey, bson_iterator_bool( &item ) ) ;
         }
         else
         {
            add_next_index_bool( pArray, bson_iterator_bool( &item ) ) ;
         }
         break ;
      case BSON_NULL:
         if( isObj )
         {
            add_assoc_null( pArray, pKey ) ;
         }
         else
         {
            add_next_index_null( pArray ) ;
         }
         break ;
      case BSON_OID:
         PHP_NEW_CLASS( pClass, pSequoiadbId ) ;
         bson_oid_to_string( bson_iterator_oid( &item ), oidHex ) ;
         FUN_CONST_STR_COPY_NEW( oidHex, pStrVal2 ) ;
         PHP_SAVE_VAR_STRING( pClass, "$oid", pStrVal2 ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_DATE:
         PHP_NEW_CLASS( pClass, pSequoiadbData ) ;
         pDriverDate = (struct phpDate *)\
               emalloc( sizeof( struct phpDate ) ) ;
         if( !pDriverDate )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         pDriverDate->milli = (INT64)bson_iterator_date( &item ) ;
         PHP_SAVE_RESOURCE( pClass, "$date", pDriverDate, dateDesc ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_TIMESTAMP:
         PHP_NEW_CLASS( pClass, pSequoiadbTimeStamp ) ;
         pDriverTimestamp = (struct phpTimestamp *)\
               emalloc( sizeof( struct phpTimestamp ) ) ;
         if( !pDriverTimestamp )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ts = bson_iterator_timestamp( &item ) ;
         pDriverTimestamp->second = ts.t ;
         pDriverTimestamp->micros = ts.i ;
         PHP_SAVE_RESOURCE( pClass,
                            "$timestamp",
                            pDriverTimestamp,
                            timestampDesc ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_REGEX:
         PHP_NEW_CLASS( pClass, pSequoiadbRegex ) ;
         pStrVal = bson_iterator_regex( &item ) ;
         FUN_CONST_STR_COPY_NEW( pStrVal, pStrVal2 ) ;
         PHP_SAVE_VAR_STRING( pClass, "$regex", pStrVal2 ) ;
         pStrVal = bson_iterator_regex_opts( &item ) ;
         FUN_CONST_STR_COPY_NEW( pStrVal, pStrVal2 ) ;
         PHP_SAVE_VAR_STRING( pClass, "$options", pStrVal2 ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_BINDATA:
         PHP_NEW_CLASS( pClass, pSequoiadbBinary ) ;
         strValLen = bson_iterator_bin_len( &item ) ;
         type = bson_iterator_bin_type( &item ) ;
         if( strValLen > 0 )
         {
            strValLen2 = getEnBase64Size( strValLen ) ;
            pStrVal2 = (CHAR *)emalloc( strValLen2 + 1 ) ;
            if( !pStrVal2 )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            ossMemset( pStrVal2, 0, strValLen2 + 1 ) ;
            pStrVal = (CHAR *)bson_iterator_bin_data( &item ) ;
            if( base64Encode( pStrVal, strValLen, pStrVal2, strValLen2 ) < 0 )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            PHP_SAVE_VAR_STRING( pClass, "$binary", pStrVal2 ) ;
            PHP_SAVE_VAR_INT( pClass, "$type", type ) ;
         }
         else
         {
            PHP_SAVE_VAR_STRING( pClass, "$binary", "" ) ;
            PHP_SAVE_VAR_INT( pClass, "$type", type ) ;
         }
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_MINKEY:
         PHP_NEW_CLASS( pClass, pSequoiadbMinKey ) ;
         PHP_SAVE_VAR_INT( pClass, "$minKey", 1 ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_MAXKEY:
         PHP_NEW_CLASS( pClass, pSequoiadbMaxKey ) ;
         PHP_SAVE_VAR_INT( pClass, "$maxKey", 1 ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_DECIMAL:
         PHP_NEW_CLASS( pClass, pSequoiadbDecimal ) ;
         pDecimal = (bson_decimal *)emalloc( sizeof( bson_decimal ) ) ;
         if( !pDecimal )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         decimal_init( pDecimal ) ;
         rc = bson_iterator_decimal( &item, pDecimal ) ;
         if( rc != BSON_OK )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         PHP_SAVE_RESOURCE( pClass, "$decimal", pDecimal, decimalDesc ) ;
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pClass ) ;
         }
         else
         {
            add_next_index_zval( pArray, pClass ) ;
         }
         break ;
      case BSON_OBJECT:
      case BSON_ARRAY:
      {
         MAKE_STD_ZVAL( pChild ) ;
         array_init( pChild ) ;
         rc = _bson2Array( bson_iterator_value( &item ),
                           pChild,
                           bsonType == BSON_OBJECT ? TRUE : FALSE TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
         if( isObj )
         {
            add_assoc_zval( pArray, pKey, pChild ) ;
         }
         else
         {
            add_next_index_zval( pArray, pChild ) ;
         }
         break ;
      }
      default:
         break ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 php_bson2Array( bson *pBson, zval **ppArray TSRMLS_DC )
{
   return _bson2Array( pBson->data, *ppArray, TRUE TSRMLS_CC ) ;
}

void php_parseNumber( CHAR *pBuffer,
                      INT32 size,
                      INT32 *pNumberType,
                      INT32 *pInt32,
                      INT64 *pInt64,
                      double *pDouble TSRMLS_DC )
{
   INT32 type = PHP_IS_INT32 ;
   double n = 0 ;
   double sign = 1 ;
   double scale = 0 ;
   double subscale = 0 ;
   double signsubscale = 1 ;
   INT32 n1 = 0 ;
   INT64 n2 = 0 ;
   volatile INT64 n3 = 0 ;

   if( 0 == size )
   {
      type = PHP_NOT_NUMBER ;
      goto done ;
   }

   if( *pBuffer != '+' && *pBuffer != '-' &&
       ( *pBuffer < '0' || *pBuffer >'9' ) )
   {
      type = PHP_NOT_NUMBER ;
      goto done ;
   }

   if( '-' == *pBuffer )
   {
      sign = -1 ;
      --size ;
      ++pBuffer ;
   }
   else if( '+' == *pBuffer )
   {
      sign = 1 ;
      --size ;
      ++pBuffer ;
   }

   while( size > 0 && '0' == *pBuffer )
   {
      ++pBuffer ;
      --size ;
   }

   if( size > 0 && *pBuffer >= '1' && *pBuffer <= '9' )
   {
      do
      {
         n3 = ( n2 * 10 ) + ( *pBuffer - '0' ) ;
         if( ( n3 - ( *pBuffer - '0' ) ) / 10 != n2 )
         {
            type = PHP_IS_DOUBLE ;
         }
         n  = ( n  * 10.0 ) + ( *pBuffer - '0' ) ;
         n1 = ( n1 * 10 )   + ( *pBuffer - '0' ) ;
         n2 = n3 ;
         --size ;
         ++pBuffer ;
         if( type == PHP_IS_INT32 && (INT64)n1 != n2 )
         {
            type = PHP_IS_INT64 ;
         }
      }
      while( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' ) ;
   }

   if( size > 0 && *pBuffer == '.' &&
       pBuffer[1] >= '0' && pBuffer[1] <= '9' )
   {
      type = PHP_IS_DOUBLE ;
      --size ;
      ++pBuffer ;
      while( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' )
      {
         n = ( n ) + ( *pBuffer - '0' ) / pow( 10.0, ++scale ) ;
         --size ;
         ++pBuffer ;
      }
   }
   else if( size == 1 && *pBuffer == '.' )
   {
      ++pBuffer ;
      --size ;
   }

   if( size > 0 && ( *pBuffer == 'e' || *pBuffer == 'E' ) )
   {
      --size ;
      ++pBuffer ;
      if( size > 0 && '+' == *pBuffer )
      {
         --size ;
         ++pBuffer ;
         signsubscale = 1 ;
      }
      else if( size > 0 && '-' == *pBuffer )
      {
         type = PHP_IS_DOUBLE ;
         --size ;
         ++pBuffer;
         signsubscale = -1 ;
      }
      while( size > 0 && *pBuffer >= '0' && *pBuffer <= '9' )
      {
         subscale = ( subscale * 10 ) + ( *pBuffer - '0' ) ;
         --size ;
         ++pBuffer ;
      }
   }

   if( size == 0 )
   {
      if( PHP_IS_DOUBLE == type )
      {
         n = sign * n * pow ( 10.0, ( subscale * signsubscale * 1.0 ) ) ;
      }
      else if( PHP_IS_INT64 == type )
      {
         if ( 0 != subscale )
         {
            n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
         }
         else
         {
            n2 = ( ( (INT64) sign ) * n2 ) ;
         }
      }
      else if ( PHP_IS_INT32 == type )
      {
          n1 = (INT32)( sign * n1 * pow( 10.0, subscale * 1.00 ) ) ;
          n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
          if ( (INT64)n1 != n2 )
          {
             type = PHP_IS_INT64 ;
          }
      }
   }
   else
   {
      type = PHP_NOT_NUMBER ;
   }
done:
   *pNumberType = type ;
   if ( pInt32 )
   {
      (*pInt32) = n1 ;
   }
   if ( pInt64 )
   {
      (*pInt64) = n2 ;
   }
   if( pDouble )
   {
      (*pDouble) = n ;
   }
}

#define RELATIVE_YEAR 1900
#define TIME_FORMAT  "%d-%d-%d-%d.%d.%d.%d"
#define TIME_FORMAT2 "%d-%d-%d-%d:%d:%d.%d"
#define DATE_FORMAT  "%d-%d-%d"
/*
 * valType: 0 date
 *          1 timestamp
*/
BOOLEAN php_date2Time( const CHAR *pDate, INT32 valType,
                       time_t *pTimestamp, INT32 *pMicros )
{
   /*
      eg. before 1927-12-31-23.54.07,
      will be more than 352 seconds
      UTC time
      date min 0000-01-01-00.00.00.000000
      date max 9999-12-31-23.59.59.999999
      timestamp min 1901-12-13-20.45.52.000000 +/- TZ
      timestamp max 2038-01-19-03.14.07.999999 +/- TZ
   */
   /* date and timestamp */
   BOOLEAN flag = TRUE ;
   INT32 year   = 0 ;
   INT32 month  = 0 ;
   INT32 day    = 0 ;
   INT32 hour   = 0 ;
   INT32 minute = 0 ;
   INT32 second = 0 ;
   INT32 micros = 0 ;
   time_t timep = 0 ;
   struct tm t ;

   ossMemset( &t, 0, sizeof( t ) ) ;
   if( ossStrchr( pDate, 'T' ) ||
       ossStrchr( pDate, 't' ) )
   {
      /* for mongo date type, iso8601 */
      sdbTimestamp sdbTime ;
      if( timestampParse( pDate,
                          ossStrlen( pDate ),
                          &sdbTime ) )
      {
         goto error ;
      }
      timep = (time_t)sdbTime.sec ;
      micros = sdbTime.nsec / 1000 ;
   }
   else
   {
      if( valType == 1 )
      {
         /* for timestamp type, we provide yyyy-mm-dd-hh.mm.ss.uuuuuu */
         BOOLEAN hasColon = FALSE ;

         if( ossStrchr( pDate, ':' ) )
         {
            hasColon = TRUE ;
         }

         if( !sscanf ( pDate,
                       hasColon ? TIME_FORMAT2 : TIME_FORMAT,
                       &year,
                       &month,
                       &day,
                       &hour,
                       &minute,
                       &second,
                       &micros ) )
         {
            goto error ;
         }
      }
      else
      {
         if( !sscanf ( pDate,
                       DATE_FORMAT,
                       &year,
                       &month,
                       &day ) )
         {
            goto error ;
         }
      }

      --month ;
      year -= RELATIVE_YEAR ;
      /* construct tm */
      t.tm_year  = year   ;
      t.tm_mon   = month  ;
      t.tm_mday  = day    ;
      t.tm_hour  = hour   ;
      t.tm_min   = minute ;
      t.tm_sec   = second ;
      /* create integer time representation */
      timep = mktime( &t ) ;
   }

   if( valType == 1 )
   {
      if ( !ossIsTimestampValid( timep ) )
      {
         goto error ;
      }
   }

   *pTimestamp = timep ;
   *pMicros = micros ;

done:
   return flag ;
error:
   flag = FALSE ;
   goto done ;
}

INT32 driver_connect( CHAR *pAddress,
                      const CHAR *pUserName,
                      const CHAR *pPassword,
                      BOOLEAN useSSL,
                      sdbConnectionHandle *pConnection TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   CHAR *pHostName = NULL ;
   CHAR *pPort = NULL ;
   if( pAddress )
   {
      rc = php_split( pAddress, ':', &pHostName, &pPort TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
   }
   if( pHostName == NULL )
   {
      pHostName = SDB_DEFAULT_HOSTNAME ;
   }
   if( pPort == NULL )
   {
      pPort = SDB_DEFAULT_SERVICENAME ;
   }
   if( pUserName == NULL )
   {
      pUserName = "" ;
   }
   if( pPassword == NULL )
   {
      pPassword = "" ;
   }
   if( useSSL == FALSE )
   {
      rc = sdbConnect( pHostName, pPort, pUserName, pPassword, pConnection ) ;
   }
   else
   {
      rc = sdbSecureConnect( pHostName, pPort, pUserName, pPassword,
                             pConnection ) ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 driver_batch_connect( zval *pAddress,
                            const CHAR *pUserName,
                            const CHAR *pPassword,
                            BOOLEAN useSSL,
                            sdbConnectionHandle *pConnection TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   if( pAddress )
   {
      if( Z_TYPE_P( pAddress ) == IS_ARRAY )
      {
         INT32 arrayType = php_getArrayType( pAddress TSRMLS_CC ) ;
         if( arrayType == PHP_INDEX_ARRAY )
         {
            HashTable *pAddressArray = HASH_OF( pAddress ) ;
            PHP_ARRAY_FOREACH_START( pAddressArray )
            {
               zval *pValue = NULL ;
               PHP_ARRAY_FOREACH_VALUE( pAddressArray, pValue ) ;
               if( Z_TYPE_P( pValue ) == IS_STRING )
               {
                  rc = driver_connect( Z_STRVAL_P( pValue ),
                                       pUserName,
                                       pPassword,
                                       useSSL,
                                       pConnection TSRMLS_CC ) ;
                  if( rc == SDB_OK )
                  {
                     break ;
                  }
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
            PHP_ARRAY_FOREACH_END()
            if( rc )
            {
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if( Z_TYPE_P( pAddress ) == IS_STRING )
      {
         rc = driver_connect( Z_STRVAL_P( pAddress ),
                              pUserName,
                              pPassword,
                              useSSL,
                              pConnection TSRMLS_CC ) ;
         if( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = driver_connect( NULL,
                           pUserName,
                           pPassword,
                           useSSL,
                           pConnection TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}