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

   Source File Name = netRouteAgent.cpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "netRouteAgent.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"

namespace engine
{
   _netRouteAgent::_netRouteAgent( _netMsgHandler *handler ):
                                   _frame( handler, &_route )
   {

   }

   // this updateRoute only change the old routeID to new one. It does NOT
   // change the hostname and servicename, so we do not need to restart services
   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_UPRT, "_netRouteAgent::updateRoute" )
   INT32 _netRouteAgent::updateRoute ( const _MsgRouteID &oldID,
                                       const _MsgRouteID &newID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_UPRT );
      rc = _route.update ( oldID, newID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      /// close the old connections
      _frame.close( oldID ) ;

   done :
      PD_TRACE_EXITRC ( SDB__NETRTAG_UPRT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_UPRT2, "_netRouteAgent::updateRoute" )
   INT32 _netRouteAgent::updateRoute( const _MsgRouteID &id,
                                      const CHAR *host,
                                      const CHAR *service )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN newAdd = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_UPRT2 );
      rc = _route.update( id, host, service, &newAdd ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // new node don't close the exist connect
      if ( FALSE == newAdd )
      {
         _frame.close( id ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_UPRT2, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_UPRT3, "_netRouteAgent::updateRoute" )
   INT32 _netRouteAgent::updateRoute( const _MsgRouteID &id,
                                      const _netRouteNode &node )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN newAdd = FALSE ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_UPRT3 );
      rc = _route.update( id, node, &newAdd ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // new node don't close the existed connect
      if ( FALSE == newAdd )
      {
         _frame.close( id ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_UPRT3, rc );
      return rc ;
   error:
      goto done ;
   }

   void _netRouteAgent::delRoute( const _MsgRouteID &id )
   {
      BOOLEAN hasDel = FALSE ;
      _route.del( id, hasDel ) ;
      if ( hasDel )
      {
         _frame.close( id ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_LSTN, "_netRouteAgent::listen" )
   INT32 _netRouteAgent::listen( const _MsgRouteID &id,
                                 UINT32 protocolMask,
                                 INetUDPMsgHandler *udpHandler,
                                 UINT32 udpBufferSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_LSTN );

      CHAR host[ OSS_MAX_HOSTNAME + 1 ] = { 0 } ;
      CHAR service[ OSS_MAX_SERVICENAME + 1] = { 0 } ;
      rc = _route.route( id, host, OSS_MAX_HOSTNAME,
                         service, OSS_MAX_SERVICENAME ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "can not find the route of %d, %d, %d",
                 id.columns.groupID, id.columns.nodeID,
                 id.columns.serviceID ) ;
         goto error ;
      }

      rc = _frame.listen( host, service, protocolMask, udpHandler,
                          udpBufferSize ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_LSTN, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netRouteAgent::syncConnect( const _MsgRouteID &id,
                                      NET_HANDLE *pHandle )
   {
      return _frame.syncConnect( id, pHandle ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_SYNCSND, "_netRouteAgent::syncSend" )
   INT32 _netRouteAgent::syncSend( const _MsgRouteID &id,
                                   void *header,
                                   NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_SYNCSND );
      rc = _frame.syncSend( id, header, pHandle ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_SYNCSND, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_SYNCSNDUDP, "_netRouteAgent::syncSendUDP" )
   INT32 _netRouteAgent::syncSendUDP( const MsgRouteID &id,
                                      void *header )
   {
      SDB_ASSERT( NULL != header, "should not be NULL" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__NETRTAG_SYNCSNDUDP ) ;

      rc = _frame.syncSendUDP( id, header ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__NETRTAG_SYNCSNDUDP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _netRouteAgent::syncSend( const NET_HANDLE &handle,
                                   void *header )
   {
     SDB_ASSERT( NULL != header,
                  "should not be NULL" ) ;

      return _frame.syncSend( handle, header ) ;
   }

   INT32 _netRouteAgent::syncSendRaw( const NET_HANDLE & handle,
                                      const CHAR * pBuff,
                                      UINT32 buffSize )
   {
      return _frame.syncSendRaw( handle, pBuff, buffSize ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_SYNCSND2, "_netRouteAgent::syncSend" )
   INT32 _netRouteAgent::syncSend( const _MsgRouteID &id,
                                   MsgHeader *header, void *body,
                                   UINT32 bodyLen,
                                   NET_HANDLE *pHandle )
   {
      SDB_ASSERT( NULL != header && NULL != body, "should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_SYNCSND2 );
      rc = _frame.syncSend( id, header, body, bodyLen, pHandle ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_SYNCSND2, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__NETRTAG_SYNCSND3, "_netRouteAgent::syncSend" )
   INT32 _netRouteAgent::syncSend( const NET_HANDLE &handle,
                                   MsgHeader *header, void *body,
                                   UINT32 bodyLen )
   {
      SDB_ASSERT( NULL != header && NULL != body, "should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__NETRTAG_SYNCSND3 );

      rc = _frame.syncSend( handle, header, body, bodyLen ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__NETRTAG_SYNCSND3, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _netRouteAgent::syncSendv( const _MsgRouteID &id,
                                    MsgHeader *header,
                                    const netIOVec &iov,
                                    NET_HANDLE *pHandle )
   {
      return _frame.syncSendv( id, header, iov, pHandle ) ;
   }

   INT32 _netRouteAgent::syncSendv( const NET_HANDLE & handle,
                                    MsgHeader * header,
                                    const netIOVec & iov )
   {
      return _frame.syncSendv( handle, header, iov ) ;
   }

   INT64 _netRouteAgent::netIn()
   {
      return _frame.netIn() ;
   }

   INT64 _netRouteAgent::netOut()
   {
      return _frame.netOut() ;
   }

}

