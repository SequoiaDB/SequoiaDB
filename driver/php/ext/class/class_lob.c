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

#include "class_lob.h"

extern zend_class_entry *pSequoiadbInt64 ;

extern INT32 lobDesc ;

PHP_METHOD( SequoiaLob, __construct )
{
}

PHP_METHOD( SequoiaLob, close )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_DEL_HANDLE( pThisObj ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
}

PHP_METHOD( SequoiaLob, getSize )
{
   INT32 rc = SDB_OK ;
   SINT64 size      = 0 ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;
   rc = sdbGetLobSize( lob, &size ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETURN_INT_OR_LONG( FALSE, pThisObj, pSequoiadbInt64, size ) ;
done:
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_LONG( -1 ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, getCreateTime )
{
   INT32 rc = SDB_OK ;
   UINT64 millis    = 0 ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;
   rc = sdbGetLobCreateTime( lob, &millis ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETURN_INT_OR_LONG( FALSE, pThisObj, pSequoiadbInt64, ((INT64)millis) ) ;
done:
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_LONG( -1 ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, getModificationTime )
{
   INT32 rc = SDB_OK ;
   UINT64 millis    = 0 ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;

   rc = sdbGetLobModificationTime( lob, &millis ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_RETURN_INT_OR_LONG( FALSE, pThisObj, pSequoiadbInt64, ((INT64)millis) ) ;

done:
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   RETVAL_LONG( -1 ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, write )
{
   INT32 rc = SDB_OK ;
   PHP_LONG bufferLen  = 0 ;
   CHAR *pBuffer    = NULL ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "s", &pBuffer, &bufferLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;
   rc = sdbWriteLob( lob, pBuffer, (UINT32)bufferLen ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, read )
{
   INT32 rc = SDB_OK ;
   INT64 length     = 0 ;
   UINT32 readLen   = 0 ;
   CHAR *pBuffer    = NULL ;
   zval *pLength    = NULL ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "z", &pLength ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_zval2Long( pLength, &length TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   if( length > (INT64)PHP_UINT32_MAX || length < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   pBuffer = (CHAR *)emalloc( (UINT32)length + 1 ) ;
   if( !pBuffer )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pBuffer, 0, (UINT32)length + 1 ) ;
   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;
   rc = sdbReadLob( lob, (UINT32)length, pBuffer, &readLen ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pBuffer, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, seek )
{
   INT32 rc = SDB_OK ;
   SDB_LOB_SEEK 	whence = SDB_LOB_SEEK_SET ;
   SINT64 offset        = 0 ;
   zval *pOffset        = NULL ;
   zval *pWhence        = NULL ;
   zval *pThisObj       = getThis() ;
   sdbLobHandle lob     = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "z|z", &pOffset, &pWhence ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_zval2Long( pOffset, &offset TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pWhence, ((INT32 *)&whence) TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;
   rc = sdbSeekLob( lob, offset, whence ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, lock )
{
   INT32 rc = SDB_OK ;
   INT64 offset     = 0 ;
   INT64 length     = 0 ;
   zval *pOffset    = NULL ;
   zval *pLength    = NULL ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if( PHP_GET_PARAMETERS( "zz", &pOffset, &pLength ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_zval2Long( pOffset, &offset TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_zval2Long( pLength, &length TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;

   rc = sdbLockLob( lob, offset, length ) ;
   if( rc )
   {
      goto error ;
   }

done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaLob, lockAndSeek )
{
   INT32 rc = SDB_OK ;
   INT64 offset     = 0 ;
   INT64 length     = 0 ;
   zval *pOffset    = NULL ;
   zval *pLength    = NULL ;
   zval *pThisObj   = getThis() ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if( PHP_GET_PARAMETERS( "zz", &pOffset, &pLength ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = php_zval2Long( pOffset, &offset TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   rc = php_zval2Long( pLength, &length TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj,
                    lob,
                    sdbLobHandle,
                    SDB_LOB_HANDLE_NAME,
                    lobDesc ) ;

   rc = sdbLockAndSeekLob( lob, offset, length ) ;
   if( rc )
   {
      goto error ;
   }

done:
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

