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

   Source File Name = pmdInnerClient.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/11/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdInnerClient.hpp"
#include "netRouteAgent.hpp"
#include "pmdEDU.hpp"
#include "msgDef.hpp"
#include "msgAuth.hpp"

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _pmdInnerClient implement
   */
   _pmdInnerClient::_pmdInnerClient()
   {
      _isAuthed      = TRUE ;
      _pRTAgent      = NULL ;
      _pEDUCB        = NULL ;
      _netHandle     = NET_INVALID_HANDLE ;
      _localPort     = 0 ;
      _peerPort      = 0 ;
      _fromPort      = 0 ;
      ossMemset( _localIP, 0, sizeof( _localIP ) ) ;
      ossMemset( _peerIP, 0, sizeof( _peerIP ) ) ;
      ossMemset( _fromIP, 0, sizeof( _fromIP ) ) ;
      ossMemset( _clientName, 0, sizeof( _clientName ) ) ;

      _makeName() ;
   }

   _pmdInnerClient::~_pmdInnerClient()
   {
      _pRTAgent      = NULL ;
      _pEDUCB        = NULL ;
   }

   void _pmdInnerClient::setClientInfo( _netRouteAgent *pNetRouter,
                                        NET_HANDLE handle )
   {
      SDB_ASSERT( pNetRouter, "net router can't be NULL" ) ;
      SDB_ASSERT( handle != NET_INVALID_HANDLE,
                  "net handle can't be invalid" ) ;

      _pRTAgent = pNetRouter ;
      _netHandle = handle ;

      // init ip and port infomation
      NET_EH eh = _pRTAgent->getFrame()->getEventHandle( handle ) ;
      if ( eh.get() )
      {
         _localPort = eh->localPort() ;
         _peerPort = eh->remotePort() ;
         ossSnprintf( _localIP, PMD_IPADDR_LEN, "%s",
                      eh->localAddr().c_str() ) ;
         ossSnprintf( _peerIP, PMD_IPADDR_LEN, "%s",
                      eh->remoteAddr().c_str() ) ;

         _makeName() ;
      }
   }

   void _pmdInnerClient::setFromInfo( const CHAR *ip, UINT16 port )
   {
      if ( ip )
      {
         ossStrncpy( _fromIP, ip, PMD_IPADDR_LEN ) ;
      }
      _fromPort = port ;
   }

   SDB_CLIENT_TYPE _pmdInnerClient::clientType() const
   {
      return SDB_CLIENT_INNER ;
   }

   const CHAR* _pmdInnerClient::clientName() const
   {
      return _clientName ;
   }

   /** \fn INT32 authenticate( MsgHeader *pMsg, const CHAR **authBuf )
       \brief Authentication.
       \param [in] pMsg The message sent by client.
       \param [out] authBuf The bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _pmdInnerClient::authenticate( MsgHeader * pMsg,
                                        const CHAR **authBuf )
   {
      INT32 rc = SDB_OK ;
      BSONObj authObj ;
      BSONElement user, pass ;
      rc = extractAuthMsg( pMsg, authObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Client[%s] extract auth msg failed, rc: %d",
                 clientName(), rc ) ;
         goto error ;
      }
      user = authObj.getField( SDB_AUTH_USER ) ;
      pass = authObj.getField( SDB_AUTH_PASSWD ) ;

      _isAuthed = TRUE ;
      _username = user.valuestrsafe() ;
      if ( !_username.empty() )
      {
         _password = pass.valuestrsafe() ;
      }
      _pEDUCB->setUserInfo( _username, _password ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdInnerClient::authenticate( const CHAR *username,
                                        const CHAR *password )
   {
      if ( !username || !password )
      {
         return SDB_INVALIDARG ;
      }
      _isAuthed = TRUE ;
      _username = username ;
      if ( !_username.empty() )
      {
         _password = password ;
      }
      _pEDUCB->setUserInfo( username, password ) ;
      return SDB_OK ;
   }

   void _pmdInnerClient::logout()
   {
   }

   INT32 _pmdInnerClient::disconnect()
   {
      return SDB_OK ;
   }

   BOOLEAN _pmdInnerClient::isAuthed() const
   {
      return _isAuthed ;
   }

   BOOLEAN _pmdInnerClient::isConnected() const
   {
      return NET_INVALID_HANDLE == _netHandle ? FALSE : TRUE ;
   }

   BOOLEAN _pmdInnerClient::isClosed() const
   {
      return NET_INVALID_HANDLE == _netHandle ? TRUE : FALSE ;
   }

   UINT16 _pmdInnerClient::getLocalPort() const
   {
      return _localPort ;
   }

   UINT16 _pmdInnerClient::getPeerPort() const
   {
      return _peerPort ;
   }

   const CHAR* _pmdInnerClient::getLocalIPAddr() const
   {
      return _localIP ;
   }

   const CHAR* _pmdInnerClient::getPeerIPAddr() const
   {
      return _peerIP ;
   }

   const CHAR* _pmdInnerClient::getFromIPAddr() const
   {
      return _fromIP[0] != 0 ? _fromIP : _peerIP ;
   }

   UINT16 _pmdInnerClient::getFromPort() const
   {
      return _fromPort != 0 ? _fromPort : _peerPort ;
   }

   const CHAR* _pmdInnerClient::getUsername() const
   {
      return _username.c_str() ;
   }

   const CHAR* _pmdInnerClient::getPassword() const
   {
      return _password.c_str() ;
   }

   void _pmdInnerClient::_makeName()
   {
      if ( 0 == _peerIP[ 0 ] )
      {
         ossStrcpy( _clientName, "noip-Inner" ) ;
      }
      else if ( !getUsername() || 0 == *getUsername() )
      {
         ossSnprintf( _clientName, PMD_CLIENTNAME_LEN, "%s:%u-Inner",
                      _peerIP, _peerPort ) ;
      }
      else
      {
         ossSnprintf( _clientName, PMD_CLIENTNAME_LEN, "%s@%s:%u-Inner",
                      getUsername(), _peerIP, _peerPort ) ;
      }
   }

}


