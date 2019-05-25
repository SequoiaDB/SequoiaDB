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

#include "class_regex.h"

PHP_METHOD( SequoiaRegex, __construct )
{
   zval *pRegex   = NULL ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   if( PHP_GET_PARAMETERS( "|zz", &pRegex, &pOptions ) == FAILURE )
   {
      goto error ;
   }
   if( pRegex && Z_TYPE_P( pRegex ) == IS_STRING )
   {
      PHP_SAVE_VAR_STRING( pThisObj, "$regex", Z_STRVAL_P( pRegex ) ) ;
   }
   if( pOptions && Z_TYPE_P( pOptions ) == IS_STRING )
   {
      PHP_SAVE_VAR_STRING( pThisObj, "$options", Z_STRVAL_P( pOptions ) ) ;
   }
done:
   return ;
error:
   goto done ;
}

PHP_METHOD( SequoiaRegex, __toString )
{
   INT32 length    = 0 ;
   zval *pRegex    = NULL ;
   zval *pOptions  = NULL ;
   zval *pThisObj  = getThis() ;
   CHAR *pRegexStr = NULL ;
   PHP_READ_VAR( pThisObj, "$regex", pRegex ) ;
   PHP_READ_VAR( pThisObj, "$options", pOptions ) ;
   length = Z_STRLEN_P( pRegex ) + Z_STRLEN_P( pOptions ) + 3 ;
   pRegexStr = (CHAR *)emalloc( length + 1 ) ;
   if( !pRegexStr )
   {
      goto error ;
   }
   ossMemset( pRegexStr, 0, length + 1 ) ;
   ossSnprintf( pRegexStr,
                length,
                "/%s/%s",
                Z_STRVAL_P( pRegex ),
                Z_STRVAL_P( pOptions ) ) ;
   PHP_RETVAL_STRING( pRegexStr, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}