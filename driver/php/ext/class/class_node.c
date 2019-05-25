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

#include "class_node.h"

extern zend_class_entry *pSequoiadbSdb ;

extern INT32 connectionDesc ;
extern INT32 nodeDesc ;

PHP_METHOD( SequoiaNode, __construct )
{
}

PHP_METHOD( SequoiaNode, getName )
{
   zval *pNodeName = NULL ;
   zval *pThisObj  = getThis() ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_VAR( pThisObj, "_name", pNodeName ) ;
   PHP_RETVAL_STRING( Z_STRVAL_P( pNodeName ), 1 ) ;
}

PHP_METHOD( SequoiaNode, getHostName )
{
   INT32 rc = SDB_OK ;
   CHAR *pHostName = NULL ;
   zval *pNodeName = NULL ;
   zval *pThisObj  = getThis() ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_VAR( pThisObj, "_name", pNodeName ) ;
   rc = php_split( Z_STRVAL_P( pNodeName ), ':', &pHostName, NULL TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pHostName, 1 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaNode, getServiceName )
{
   INT32 rc = SDB_OK ;
   CHAR *pService  = NULL ;
   zval *pNodeName = NULL ;
   zval *pThisObj  = getThis() ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_VAR( pThisObj, "_name", pNodeName ) ;
   rc = php_split( Z_STRVAL_P( pNodeName ), ':', NULL, &pService TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   if( pService )
   {
      PHP_RETVAL_STRING( pService, 1 ) ;
   }
   else
   {
      RETVAL_EMPTY_STRING() ;
   }
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaNode, connect )
{
   INT32 rc = SDB_OK ;
   CHAR *pHostName = NULL ;
   CHAR *pService  = NULL ;
   zval *pNodeName = NULL ;
   zval *pThisObj  = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_VAR( pThisObj, "_name", pNodeName ) ;
   rc = php_split( Z_STRVAL_P( pNodeName ), ':', &pHostName, &pService TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   if( pHostName == NULL || pService == NULL )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = sdbConnect( pHostName, pService, "", "", &connection ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbSdb,
                    connection,
                    connectionDesc ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaNode, start )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    node,
                    sdbNodeHandle,
                    SDB_NODE_HANDLE_NAME,
                    nodeDesc ) ;
   rc = sdbStartNode( node ) ;
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

PHP_METHOD( SequoiaNode, stop )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    node,
                    sdbNodeHandle,
                    SDB_NODE_HANDLE_NAME,
                    nodeDesc ) ;
   rc = sdbStopNode( node ) ;
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