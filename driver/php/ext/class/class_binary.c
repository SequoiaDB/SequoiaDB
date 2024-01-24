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

#include "class_binary.h"

PHP_METHOD( SequoiaBinary, __construct )
{
   CHAR type      = 0 ;
   zval *pBinary  = NULL ;
   zval *pType    = NULL ;
   zval *pThisObj = getThis() ;
   if( PHP_GET_PARAMETERS( "|zz", &pBinary, &pType ) == FAILURE )
   {
      goto error ;
   }
   if( pBinary && Z_TYPE_P( pBinary ) == IS_STRING )
   {
      PHP_SAVE_VAR_STRING( pThisObj, "$binary", Z_STRVAL_P( pBinary ) ) ;
   }
   if( pType && Z_TYPE_P( pType ) == IS_STRING )
   {
      type = (CHAR)( ossAtoi( Z_STRVAL_P( pType ) ) ) ;
      PHP_SAVE_VAR_INT( pThisObj, "$type", type ) ;
   }
   else if( pType && Z_TYPE_P( pType ) == IS_LONG )
   {
      type = (CHAR)Z_LVAL_P( pType ) ;
      PHP_SAVE_VAR_INT( pThisObj, "$type", type ) ;
   }
done:
   return ;
error:
   goto done ;
}

PHP_METHOD( SequoiaBinary, __toString )
{
   PHP_LONG length  = 0 ;
   zval *pBinary    = NULL ;
   zval *pType      = NULL ;
   zval *pThisObj   = getThis() ;
   CHAR *pBinaryStr = NULL ;
   PHP_READ_VAR( pThisObj, "$binary", pBinary ) ;
   PHP_READ_VAR( pThisObj, "$type", pType ) ;
   length = Z_STRLEN_P( pBinary ) + 5 ;
   pBinaryStr = (CHAR *)emalloc( length + 1 ) ;
   if( !pBinaryStr )
   {
      goto error ;
   }
   ossMemset( pBinaryStr, 0, length + 1 ) ;
   ossSnprintf( pBinaryStr,
                length,
                "(%d)%s",
                Z_LVAL_P( pType ),
                Z_STRVAL_P( pBinary ) ) ;
   PHP_RETVAL_STRING( pBinaryStr, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}