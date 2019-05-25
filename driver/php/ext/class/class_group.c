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

#include "class_group.h"

extern zend_class_entry *pSequoiadbNode ;

extern INT32 connectionDesc ;
extern INT32 groupDesc ;
extern INT32 nodeDesc ;

PHP_METHOD( SequoiaGroup, __construct )
{
}

PHP_METHOD( SequoiaGroup, isCatalog )
{
   zval *pThisObj = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   RETVAL_BOOL( sdbIsReplicaGroupCatalog( group ) ) ;
}

PHP_METHOD( SequoiaGroup, getName )
{
   INT32 rc = SDB_OK ;
   CHAR *pGroupName   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   pGroupName = (CHAR *)emalloc( PHP_NAME_SIZE ) ;
   if( !pGroupName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( pGroupName, 0, PHP_NAME_SIZE ) ;
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbGetRGName( group, pGroupName, PHP_NAME_LEN ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_RETVAL_STRING( pGroupName, 0 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, start )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbStartReplicaGroup( group ) ;
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

PHP_METHOD( SequoiaGroup, stop )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbStopReplicaGroup( group ) ;
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

PHP_METHOD( SequoiaGroup, reelect )
{
   INT32 rc = SDB_OK ;
   zval *pOptions = NULL ;
   zval *pThisObj = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   bson options ;
   bson_init( &options ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "|z", &pOptions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pOptions, &options TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbReelect( group, &options ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &options ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, getNodeNum )
{
   INT32 rc = SDB_OK ;
   INT32 status     = 0 ;
   INT32 count      = 0 ;
   zval *pStatus    = NULL ;
   zval *pGroupName = NULL ;
   zval *pSequoiadb = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson_type iteType              = BSON_EOO ;
   bson condition ;
   bson record ;
   bson_iterator it ;
   bson_init( &condition ) ;
   bson_init( &record ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "|z", &pStatus ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   /* 没有实现status
   rc = php_zval2Int( pStatus, &status TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   */
   PHP_READ_VAR( pThisObj, "_SequoiaDB", pSequoiadb ) ;
   PHP_READ_HANDLE( pSequoiadb,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   PHP_READ_VAR( pThisObj, "_name", pGroupName ) ;
   rc = bson_append_string( &condition,
                            FIELD_NAME_GROUPNAME,
                            Z_STRVAL_P( pGroupName ) ) ;
   if( rc != BSON_OK )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   bson_finish( &condition ) ;
   rc = sdbGetList( connection,
                    SDB_LIST_GROUPS,
                    &condition,
                    NULL,
                    NULL,
                    &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbNext( cursor, &record ) ;
   if( rc )
   {
      goto error ;
   }
   iteType = bson_find( &it, &record, FIELD_NAME_GROUP ) ;
   if( iteType == BSON_ARRAY )
   {
      while( bson_iterator_more( &it ) )
      {
         ++count ;
      }
   }
   RETVAL_LONG( count ) ;
done:
   if( cursor != SDB_INVALID_HANDLE )
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
   if( connection != SDB_INVALID_HANDLE )
   {
      sdbReleaseConnection( connection ) ;
   }
   bson_destroy( &condition ) ;
   bson_destroy( &record ) ;
   return ;
error:
   RETVAL_LONG( -1 ) ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, getDetail )
{
   INT32 rc = SDB_OK ;
   zval *pGroupName = NULL ;
   zval *pSequoiadb = NULL ;
   zval *pThisObj   = getThis() ;
   sdbConnectionHandle connection = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor         = SDB_INVALID_HANDLE ;
   bson condition ;
   bson record ;
   bson_init( &condition ) ;
   bson_init( &record ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_VAR( pThisObj, "_SequoiaDB", pSequoiadb ) ;
   PHP_READ_HANDLE( pSequoiadb,
                    connection,
                    sdbConnectionHandle,
                    SDB_HANDLE_NAME,
                    connectionDesc ) ;
   PHP_READ_VAR( pThisObj, "_name", pGroupName ) ;
   rc = bson_append_string( &condition,
                            FIELD_NAME_GROUPNAME,
                            Z_STRVAL_P( pGroupName ) ) ;
   if( rc != BSON_OK )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   bson_finish( &condition ) ;
   rc = sdbGetList( connection,
                    SDB_LIST_GROUPS,
                    &condition,
                    NULL,
                    NULL,
                    &cursor ) ;
   if( rc )
   {
      goto error ;
   }
   rc = sdbNext( cursor, &record ) ;
   if( rc )
   {
      goto error ;
   }
done:
   if( cursor != SDB_INVALID_HANDLE )
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
   PHP_RETURN_AUTO_RECORD( FALSE,
                           pThisObj,
                           (rc == SDB_OK ? FALSE : TRUE),
                           record ) ;
   bson_destroy( &condition ) ;
   bson_destroy( &record ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, getMaster )
{
   INT32 rc = SDB_OK ;
   zval *pThisObj              = getThis() ;
   const CHAR *pNodeName       = NULL ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   sdbNodeHandle node          = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbGetNodeMaster( group, &node ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbNode,
                    node,
                    nodeDesc ) ;
   rc = sdbGetNodeAddr( node, NULL, NULL, &pNodeName, NULL ) ;
   if( rc == SDB_OK )
   {
      PHP_SAVE_VAR_STRING( return_value, "_name", pNodeName ) ;
   }
   rc = SDB_OK ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, getSlave )
{
   INT32 rc = SDB_OK ;
   INT32 i  = 0 ;
   INT32 positionsCount        = 0 ;
   INT32 *pPositionsArray      = NULL ;
   zval *pThisObj              = getThis() ;
   zval *pPositions            = NULL ;
   const CHAR *pNodeName       = NULL ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   sdbNodeHandle node          = SDB_INVALID_HANDLE ;

   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;

   if ( PHP_GET_PARAMETERS( "|z", &pPositions ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   PHP_READ_HANDLE( pThisObj, group, sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME, groupDesc ) ;

   if ( pPositions )
   {
      rc = php_assocArray2IntArray( pPositions, &pPositionsArray,
                                    &positionsCount TSRMLS_CC ) ;
      if( rc )
      {
         goto error ;
      }
   }

   if( positionsCount > 0 )
   {
      rc = sdbGetNodeSlave1( group, pPositionsArray, positionsCount, &node ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      rc = sdbGetNodeSlave( group, &node ) ;
      if( rc )
      {
         goto error ;
      }
   }

   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbNode,
                    node,
                    nodeDesc ) ;

   rc = sdbGetNodeAddr( node, NULL, NULL, &pNodeName, NULL ) ;
   if( rc == SDB_OK )
   {
      PHP_SAVE_VAR_STRING( return_value, "_name", pNodeName ) ;
   }

   rc = SDB_OK ;

done:
   if( pPositionsArray )
   {
      efree( pPositionsArray ) ;
   }
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, getNode )
{
   INT32 rc = SDB_OK ;
   PHP_LONG nodeLen = 0 ;
   CHAR *pNodeName = NULL ;
   zval *pThisObj  = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   sdbNodeHandle node          = SDB_INVALID_HANDLE ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "s", &pNodeName, &nodeLen ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbGetNodeByName( group, pNodeName, &node ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_BUILD_CLASS( FALSE,
                    pThisObj,
                    pSequoiadbNode,
                    node,
                    nodeDesc ) ;
   PHP_SAVE_VAR_STRING( return_value, "_name", pNodeName ) ;
done:
   return ;
error:
   RETVAL_NULL() ;
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, createNode )
{
   INT32 rc = SDB_OK ;
   PHP_LONG hostLen    = 0 ;
   PHP_LONG serviceLen = 0 ;
   PHP_LONG pathLen    = 0 ;
   CHAR *pHostName    = NULL ;
   CHAR *pServiceName = NULL ;
   CHAR *pPath        = NULL ;
   zval *pConfigure   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   bson configure ;
   bson_init( &configure ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "sss|z",
                           &pHostName,
                           &hostLen,
                           &pServiceName,
                           &serviceLen,
                           &pPath,
                           &pathLen,
                           &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pConfigure, &configure TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbCreateNode( group,
                       pHostName,
                       pServiceName,
                       pPath,
                       &configure ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configure ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, removeNode )
{
   INT32 rc = SDB_OK ;
   PHP_LONG hostLen    = 0 ;
   PHP_LONG serviceLen = 0 ;
   CHAR *pHostName    = NULL ;
   CHAR *pServiceName = NULL ;
   zval *pConfigure   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   bson configure ;
   bson_init( &configure ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "ss|z",
                           &pHostName,
                           &hostLen,
                           &pServiceName,
                           &serviceLen,
                           &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pConfigure, &configure TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbRemoveNode( group,
                       pHostName,
                       pServiceName,
                       &configure ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configure ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, attachNode )
{
   INT32 rc = SDB_OK ;
   PHP_LONG hostLen    = 0 ;
   PHP_LONG serviceLen = 0 ;
   CHAR *pHostName    = NULL ;
   CHAR *pServiceName = NULL ;
   zval *pConfigure   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   bson configure ;
   bson_init( &configure ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "ss|z",
                           &pHostName,
                           &hostLen,
                           &pServiceName,
                           &serviceLen,
                           &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pConfigure, &configure TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbAttachNode( group,
                       pHostName,
                       pServiceName,
                       &configure ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configure ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}

PHP_METHOD( SequoiaGroup, detachNode )
{
   INT32 rc = SDB_OK ;
   PHP_LONG hostLen    = 0 ;
   PHP_LONG serviceLen = 0 ;
   CHAR *pHostName    = NULL ;
   CHAR *pServiceName = NULL ;
   zval *pConfigure   = NULL ;
   zval *pThisObj     = getThis() ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   bson configure ;
   bson_init( &configure ) ;
   PHP_SET_ERRNO_OK( FALSE, pThisObj ) ;
   if( PHP_GET_PARAMETERS( "ss|z",
                           &pHostName,
                           &hostLen,
                           &pServiceName,
                           &serviceLen,
                           &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = php_auto2Bson( pConfigure, &configure TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   PHP_READ_HANDLE( pThisObj,
                    group,
                    sdbReplicaGroupHandle,
                    SDB_GROUP_HANDLE_NAME,
                    groupDesc ) ;
   rc = sdbDetachNode( group,
                       pHostName,
                       pServiceName,
                       &configure ) ;
   if( rc )
   {
      goto error ;
   }
done:
   bson_destroy( &configure ) ;
   PHP_RETURN_AUTO_ERROR( FALSE, pThisObj, rc ) ;
   return ;
error:
   PHP_SET_ERROR( FALSE, pThisObj, rc ) ;
   goto done ;
}