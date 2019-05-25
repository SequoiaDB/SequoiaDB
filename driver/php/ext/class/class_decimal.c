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

#include "class_decimal.h"

extern INT32 decimalDesc ;

PHP_METHOD( SequoiaDecimal, __construct )
{
   INT32 rc = SDB_OK ;
   INT32 precision  = -1 ;
   INT32 scale      = -1 ;
   zval *pDecimal   = NULL ;
   zval *pPrecision = NULL ;
   zval *pScale     = NULL ;
   zval *pThisObj   = getThis() ;
   bson_decimal *pBsonDecimal = NULL ;
   pBsonDecimal = (bson_decimal*)emalloc( sizeof( bson_decimal ) ) ;
   if( !pBsonDecimal )
   {
      goto error ;
   }
   if( PHP_GET_PARAMETERS( "z|zz",
                           &pDecimal,
                           &pPrecision,
                           &pScale ) == FAILURE )
   {
      goto error ;
   }
   if( pPrecision && pScale )
   {
      php_zval2Int( pPrecision, &precision TSRMLS_CC ) ;
      php_zval2Int( pScale, &scale TSRMLS_CC ) ;
   }
   if( precision == -1 && scale == -1 )
   {
      decimal_init( pBsonDecimal ) ;
   }
   else
   {
      rc = decimal_init1( pBsonDecimal, precision, scale ) ;
      if( rc )
      {
         goto error ;
      }
   }
   if( Z_TYPE_P( pDecimal ) == IS_STRING )
   {
      rc = decimal_from_str( Z_STRVAL_P( pDecimal ), pBsonDecimal ) ;
   }
   else if( Z_TYPE_P( pDecimal ) == IS_LONG )
   {
      rc = decimal_from_int( Z_LVAL_P( pDecimal ), pBsonDecimal ) ;
   }
   else if( Z_TYPE_P( pDecimal ) == IS_DOUBLE )
   {
      rc = decimal_from_double( Z_DVAL_P( pDecimal ), pBsonDecimal ) ;
   }
   else
   {
      decimal_free( pBsonDecimal ) ;
      goto error ;
   }
   if( rc )
   {
      decimal_free( pBsonDecimal ) ;
      goto error ;
   }
done:
   PHP_SAVE_RESOURCE( pThisObj, "$decimal", pBsonDecimal, decimalDesc ) ;
   return ;
error:
   pBsonDecimal = NULL ;
   goto done ;
}

PHP_METHOD( SequoiaDecimal, __toString )
{
   INT32 rc = SDB_OK ;
   INT32 valueSize = 0 ;
   zval *pThisObj = getThis() ;
   CHAR *pValue   = NULL ;
   bson_decimal *pBsonDecimal = NULL ;
   PHP_READ_RESOURCE( pThisObj,
                      "$decimal",
                      pBsonDecimal,
                      bson_decimal*,
                      SDB_DECIMAL_HANDLE_NAME,
                      decimalDesc ) ;
   if( !pBsonDecimal )
   {
      goto error ;
   }
   rc = decimal_to_str_get_len( pBsonDecimal, &valueSize ) ;
   if( rc )
   {
      goto error ;
   }
   pValue = (CHAR *)emalloc( valueSize ) ;
   if( pValue == NULL )
   {
      goto error ;
   }
   ossMemset( pValue, 0, valueSize ) ;
   rc = decimal_to_str( pBsonDecimal, pValue, valueSize ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pValue, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}