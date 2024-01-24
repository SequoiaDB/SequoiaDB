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

#include "class_int64.h"

PHP_METHOD( SequoiaINT64, __construct )
{
   zval *pNumber = NULL ;
   zval *pThisObj = getThis() ;
   if( PHP_GET_PARAMETERS( "|z", &pNumber ) == FAILURE )
   {
      goto error ;
   }
   if( pNumber && Z_TYPE_P( pNumber ) == IS_STRING )
   {
      PHP_SAVE_VAR_STRING( pThisObj, "INT64", Z_STRVAL_P( pNumber ) ) ;
   }
done:
   return ;
error:
   goto done ;
}

PHP_METHOD( SequoiaINT64, __toString )
{
   zval *pNumber  = NULL ;
   zval *pThisObj = getThis() ;
   PHP_READ_VAR( pThisObj, "INT64", pNumber ) ;
   PHP_RETVAL_STRING( Z_STRVAL_P( pNumber ), 1 ) ;
   return ;
}