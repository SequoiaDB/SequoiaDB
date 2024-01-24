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

#include "class_cursor.h"

extern INT32 connectionDesc ;
extern INT32 cursorDesc ;

PHP_METHOD( SequoiaCursor, __construct )
{
}

//e.g. Rename getNext
PHP_METHOD( SequoiaCursor, next )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson record ;
   bson_init( &record ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cursor,
                    sdbCursorHandle,
                    SDB_CURSOR_HANDLE_NAME,
                    cursorDesc ) ;
   bson_init( &record ) ;
   rc = sdbNext( cursor, &record ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_RECORD( FALSE,
                           pThisObj,
                           (rc == SDB_OK ? FALSE : TRUE),
                           record ) ;
   bson_destroy( &record ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaCursor, current )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson record ;
   bson_init( &record ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    cursor,
                    sdbCursorHandle,
                    SDB_CURSOR_HANDLE_NAME,
                    cursorDesc ) ;
   bson_init( &record ) ;
   rc = sdbCurrent( cursor, &record ) ;
   if( rc )
   {
      goto error ;
   }
done:
   PHP_RETURN_AUTO_RECORD( FALSE,
                           pThisObj,
                           (rc == SDB_OK ? FALSE : TRUE),
                           record ) ;
   bson_destroy( &record ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}