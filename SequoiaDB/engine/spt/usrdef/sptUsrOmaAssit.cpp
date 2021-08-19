/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sptUsrOmaAssit.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrOmaAssit.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "omagentDef.hpp"

namespace engine
{

   /*
      _sptUsrOmaAssit implement
   */
   _sptUsrOmaAssit::_sptUsrOmaAssit()
   {
      _groupHandle      = 0 ;
   }

   _sptUsrOmaAssit::~_sptUsrOmaAssit()
   {
   }

   INT32 _sptUsrOmaAssit::disconnect()
   {
      INT32 rc = SDB_OK ;

      rc = _sptUsrRemoteAssit::disconnect() ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to disconnect, rc: %d", rc ) ;
         goto error ;
      }
      if ( 0 != _groupHandle )
      {
         sdbReleaseReplicaGroup( _groupHandle ) ;
         _groupHandle = 0 ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::connect( const CHAR * pHostName,
                                   const CHAR * pServiceName )
   {
      INT32 rc = SDB_OK ;
      rc = _sptUsrRemoteAssit::connect( pHostName, pServiceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Connect to %s:%s failed, rc: %d",
                   pHostName, pServiceName, rc ) ;

      // get coord group
      rc = _getCoordGroupHandle( _groupHandle ) ;
      PD_RC_CHECK( rc, PDERROR, "Get group handle failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      disconnect() ;
      goto done ;
   }

   INT32 _sptUsrOmaAssit::createNode( const CHAR * pSvcName,
                                      const CHAR * pDBPath,
                                      const CHAR * pConfig )
   {
      INT32 rc = SDB_OK ;
      bson config ;
      bson_init( &config ) ;
      rc = bson_init_finished_data( &config, pConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to finish bons, rc: %d", rc ) ;

      rc = sdbCreateNode( _groupHandle, "", pSvcName,
                          pDBPath, &config ) ;
      PD_RC_CHECK( rc, PDERROR, "Create Node[%s] failed, rc: %d",
                   pSvcName, rc ) ;

   done:
      bson_destroy( &config ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::removeNode( const CHAR * pSvcName,
                                      const CHAR * pConfig )
   {
      INT32 rc = SDB_OK ;
      bson config ;
      bson_init( &config ) ;
      rc = bson_init_finished_data( &config, pConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to finish bons, rc: %d", rc ) ;

      rc = sdbRemoveNode( _groupHandle, "", pSvcName, &config ) ;
      PD_RC_CHECK( rc, PDERROR, "Remove Node[%s] failed, rc: %d",
                   pSvcName, rc ) ;

   done:
      bson_destroy( &config ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::startNode( const CHAR * pSvcName )
   {
      INT32 rc = SDB_OK ;
      ossValuePtr nodeHandle = 0 ;

      rc = _getNodeHandle( pSvcName, nodeHandle ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node[%s] handle, rc: %d",
                   pSvcName, rc ) ;

      rc = sdbStartNode( nodeHandle ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      _releaseNodeHandle( nodeHandle ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::stopNode( const CHAR * pSvcName )
   {
      INT32 rc = SDB_OK ;
      ossValuePtr nodeHandle = 0 ;

      rc = _getNodeHandle( pSvcName, nodeHandle ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node[%s] handle, rc: %d",
                   pSvcName, rc ) ;

      rc = sdbStopNode( nodeHandle ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      _releaseNodeHandle( nodeHandle ) ;
      return rc ;
   error:
      goto done ;
   }

   void _sptUsrOmaAssit::_releaseNodeHandle( ossValuePtr handle )
   {
      if ( 0 != handle )
      {
         sdbReleaseNode( handle ) ;
      }
   }

   INT32 _sptUsrOmaAssit::_getNodeHandle( const CHAR * pSvcName,
                                          ossValuePtr &handle )
   {
      INT32 rc = SDB_OK ;
      handle = 0 ;
      sdbRGStruct *s = NULL ;
      sdbRNStruct *r = NULL ;

      if ( 0 == _groupHandle )
      {
         PD_LOG( PDERROR, "Group handle is invalid" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      s = ( sdbRGStruct* )_groupHandle ;
      r = ( sdbRNStruct* )SDB_OSS_MALLOC( sizeof( sdbRNStruct ) ) ;
      if ( !r )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      handle = (ossValuePtr)r ;

      ossMemset( (void*)r, 0, sizeof( sdbRNStruct ) ) ;
      // set members
      r->_handleType = SDB_HANDLE_TYPE_REPLICANODE ;
      r->_connection = s->_connection ;
      r->_sock = s->_sock ;
      r->_endianConvert = s->_endianConvert ;
      ossStrncpy( r->_serviceName, pSvcName, CLIENT_MAX_SERVICENAME ) ;
      _regSocket( ( ossValuePtr )&r->_sock ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::_getCoordGroupHandle( ossValuePtr & handle )
   {
      INT32 rc = SDB_OK ;
      handle = 0 ;
      sdbRGStruct *r                   = NULL ;
      sdbConnectionStruct *connection  = NULL ;

      if ( 0 == getHandle() )
      {
         PD_LOG( PDERROR, "Collection handle is invalid" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      connection = ( sdbConnectionStruct* )getHandle() ;
      r = ( sdbRGStruct* )SDB_OSS_MALLOC( sizeof( sdbRGStruct ) ) ;
      if ( !r )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      handle = (ossValuePtr)r ;

      ossMemset( (void*)r, 0, sizeof( sdbRGStruct ) ) ;
      // set members
      r->_handleType = SDB_HANDLE_TYPE_REPLICAGROUP ;
      r->_connection = getHandle() ;
      r->_sock = connection->_sock ;
      r->_endianConvert = connection->_endianConvert ;
      r->_isCatalog = FALSE ;
      ossStrncpy( r->_replicaGroupName, COORD_GROUPNAME, CLIENT_RG_NAMESZ ) ;
      _regSocket( ( ossValuePtr )&r->_sock ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOmaAssit::_regSocket( ossValuePtr pSock )
   {
      SDB_ASSERT( 0 != pSock, "pSock can't be null" ) ;

      INT32 rc        = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      Node *p         = NULL ;
      Node **ptr      = NULL ;
      sdbConnectionStruct *connection = (sdbConnectionStruct *)getHandle() ;

      // pass invalid socket
      if ( NULL == *(Socket **)pSock )
      {
         goto done ;
      }

      ossMutexLock( &connection->_sockMutex ) ;
      hasLock = TRUE ;

      // if client has disconnected, stop registing
      if ( NULL == connection->_sock )
      {
         goto done ;
      }

      ptr = &connection->_sockets ;

      p = (Node*)SDB_OSS_MALLOC( sizeof(Node) ) ;
      if ( !p )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset ( p, 0, sizeof(Node) ) ;
      p->data = pSock ;
      p->next = NULL ;

      // if it's the 1st time to add handle
      if ( !(*ptr) )
         *ptr = p ;
      // add handle to the node header
      else
      {
         p->next = *ptr ;
         *ptr = p ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( TRUE == hasLock )
      {
         ossMutexUnlock( &connection->_sockMutex ) ;
      }
      return rc ;
   error :
      goto done ;
   }
}

