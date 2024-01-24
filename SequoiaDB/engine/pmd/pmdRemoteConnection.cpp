/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = pmdRemoteConnection.cpp

   Descriptive Name = pmd remote Connection

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdRemoteConnection.hpp"
#include "msgMessageFormat.hpp"

namespace engine
{
   _pmdRemoteConnection::_pmdRemoteConnection()
   : _routeAgent( NULL ),
     _handle( NET_INVALID_HANDLE ),
     _isConnExtern( FALSE )
   {
      _routeID.value = MSG_INVALID_ROUTEID ;
   }

   _pmdRemoteConnection::~_pmdRemoteConnection()
   {
   }

   INT32 _pmdRemoteConnection::init( netRouteAgent *agent,
                                     BOOLEAN isConnExtern,
                                     const MsgRouteID &routeID,
                                     const NET_HANDLE &handle )
   {
      INT32 rc = SDB_OK ;

      if ( !agent || MSG_INVALID_ROUTEID == routeID.value )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _routeAgent = agent ;
      _routeID.value = routeID.value ;
      _handle = handle ;
      _isConnExtern = isConnExtern ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _pmdRemoteConnection::isConnected() const
   {
      return NET_INVALID_HANDLE != _handle ? TRUE : FALSE ;
   }

   const MsgRouteID& _pmdRemoteConnection::getRouteID() const
   {
      return _routeID ;
   }

   const NET_HANDLE& _pmdRemoteConnection::getNetHandle() const
   {
      return _handle ;
   }

   BOOLEAN _pmdRemoteConnection::isExtern() const
   {
      return _isConnExtern ;
   }

   netRouteAgent* _pmdRemoteConnection::getRouteAgent()
   {
      return _routeAgent ;
   }

   /**
    * Establish a TCP connection with the data source node, and send the system
    * information request message. This should always be done when connecting
    * to a SequoiaDB coordinator.
    */
   INT32 _pmdRemoteConnection::connect()
   {
      INT32 rc = SDB_OK ;

      if ( !_routeAgent || MSG_INVALID_ROUTEID == _routeID.value )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // Only call syncConnect for external connection(e.g., connection to data
      // source). A new connection will always be created. So they are not
      // affected by configurations like maxsocketperthread.
      // Connections from coordinator to data nodes will not call this function.
      if ( _isConnExtern && !isConnected() )
      {
         rc = _routeAgent->syncConnect( _routeID, &_handle ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Connect to node(%u.%u) failed, rc: %d",
                    _routeID.columns.groupID, _routeID.columns.nodeID,
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdRemoteConnection::disconnect()
   {
      if ( _isConnExtern && _routeAgent && NET_INVALID_HANDLE != _handle )
      {
         /// Send disconnect then close socket
         MsgOpDisconnect disMsg ;
         disMsg.header.messageLength = sizeof( MsgOpDisconnect ) ;
         disMsg.header.opCode = MSG_BS_DISCONNECT ;
         disMsg.header.requestID = 0 ;
         disMsg.header.routeID.value = 0 ;
         disMsg.header.TID = ossGetCurrentThreadID() ;

         if ( SDB_NET_INVALID_HANDLE !=
              _routeAgent->syncSend( _handle, (MsgHeader *)&disMsg ) )
         {
            _routeAgent->close( _handle ) ;
         }
      }
      _handle = NET_INVALID_HANDLE ;
   }

   void _pmdRemoteConnection::forceClose()
   {
      if ( _routeAgent && NET_INVALID_HANDLE != _handle )
      {
         _routeAgent->close( _handle ) ;
         _handle = NET_INVALID_HANDLE ;
      }
   }

   INT32 _pmdRemoteConnection::syncSend( MsgHeader *header )
   {
      INT32 rc = SDB_OK ;

      if ( _isConnExtern && !isConnected() )
      {
         rc = SDB_NET_NOT_CONNECT ;
         goto error ;
      }

      if ( NET_INVALID_HANDLE != _handle )
      {
         rc = _routeAgent->syncSend( _handle, (MsgHeader *)header, NULL, 0 ) ;
      }
      else
      {
         rc = _routeAgent->syncSend( _routeID, header, NULL, 0, &_handle ) ;
      }

      if ( SDB_NET_INVALID_HANDLE == rc )
      {
         _handle = NET_INVALID_HANDLE ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteConnection::syncSend( MsgHeader *header,
                                         void *body,
                                         UINT32 bodyLen )
   {
      INT32 rc = SDB_OK ;

      if ( _isConnExtern && !isConnected() )
      {
         rc = SDB_NET_NOT_CONNECT ;
         goto error ;
      }

      if ( NET_INVALID_HANDLE != _handle )
      {
         rc = _routeAgent->syncSend( _handle, header, body, bodyLen ) ;
      }
      else
      {
         rc = _routeAgent->syncSend( _routeID, header, body,
                                     bodyLen, &_handle ) ;
      }

      if ( SDB_NET_INVALID_HANDLE == rc )
      {
         _handle = NET_INVALID_HANDLE ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdRemoteConnection::syncSendv( MsgHeader *header,
                                          const netIOVec &iov )
   {
      INT32 rc = SDB_OK ;

      if ( _isConnExtern && !isConnected() )
      {
         rc = SDB_NET_NOT_CONNECT ;
         goto error ;
      }

      if ( NET_INVALID_HANDLE != _handle )
      {
         rc = _routeAgent->syncSendv( _handle, header, iov ) ;
      }
      else
      {
         rc = _routeAgent->syncSendv( _routeID, header, iov, &_handle ) ;
      }

      if ( SDB_NET_INVALID_HANDLE == rc )
      {
         _handle = NET_INVALID_HANDLE ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
