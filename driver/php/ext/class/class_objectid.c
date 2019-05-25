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

#include "class_objectid.h"

INT32 php_pad( CHAR *pStr, CHAR **ppNewStr TSRMLS_DC )
{
   INT32 rc = SDB_OK ;
   INT32 len = ossStrlen( pStr ) ;
   INT32 i = 0 ;
   INT32 k = 0 ;
   CHAR *pNewStr = *ppNewStr ;
   if( len < 24 )
   {
      pNewStr = (CHAR *)emalloc( 25 ) ;
      if( !pNewStr )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( pNewStr, 0, 25 ) ;
      for( i = 0, k = 0; i < 24; ++i )
      {
         if( i < 24 - len )
         {
            pNewStr[i] = '0' ;
         }
         else
         {
            pNewStr[i] = pStr[k] ;
            ++k ;
         }
      }
   }
   else
   {
      *ppNewStr = pStr ;
   }
done:
   return rc ;
error:
   goto done ;
}

PHP_METHOD( SequoiaID, __construct )
{
   INT32 rc = SDB_OK ;
   CHAR *pObjIdStr = NULL ;
   CHAR *pNewObjId = NULL ;
   zval *pObjId    = NULL ;
   zval *pThisObj  = getThis() ;
   if( PHP_GET_PARAMETERS( "|z", &pObjId ) == FAILURE )
   {
      goto error ;
   }
   if( pObjId && Z_TYPE_P( pObjId ) == IS_STRING )
   {
      pObjIdStr = Z_STRVAL_P( pObjId ) ;
      rc = php_pad( pObjIdStr, &pNewObjId TSRMLS_CC ) ;
      if( rc )
      {
         PHP_SAVE_VAR_STRING( pThisObj, "$oid", "000000000000000000000000" ) ;
      }
      else
      {
         PHP_SAVE_VAR_STRING( pThisObj, "$oid", pObjIdStr ) ;
      }
   }
   else
   {
      PHP_SAVE_VAR_STRING( pThisObj, "$oid", "000000000000000000000000" ) ;
   }
done:
   return ;
error:
   goto done ;
}

PHP_METHOD( SequoiaID, __toString )
{
   zval *pObjId    = NULL ;
   zval *pThisObj  = getThis() ;
   PHP_READ_VAR( pThisObj, "$oid", pObjId ) ;
   PHP_RETVAL_STRING( Z_STRVAL_P( pObjId ), 1 ) ;
   return ;
}