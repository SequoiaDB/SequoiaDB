/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = sptSshSession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptSshSession.hpp"
#include "pd.hpp"

#define SPT_SOCKET_TIMEOUT      3000

namespace engine
{
   _sptSshSession::_sptSshSession( const CHAR *host,
                                   const CHAR *usrname,
                                   const CHAR *passwd,
                                   INT32 *port )
   :_sock( NULL )
   {
      SDB_ASSERT( NULL != host && NULL != usrname && NULL != passwd,
                  "can not be null" ) ;
      _host.assign( host ) ;
      _usr.assign( usrname ) ;
      _passwd.assign( passwd ) ;
      _isOpen = FALSE ;

      _port = SPT_SSH_PORT ;
      if ( port )
      {
         _port = *port ;
      }
   }

   _sptSshSession::~_sptSshSession()
   {
      if ( NULL != _sock )
      {
         _sock->close() ;
         SAFE_OSS_DELETE( _sock ) ;
      }
   }

   void _sptSshSession::close()
   {
      if ( NULL != _sock )
      {
         _sock->close() ;
         SAFE_OSS_DELETE( _sock ) ;
      }
      _isOpen = FALSE ;
   }

   string _sptSshSession::getLocalIPAddr()
   {
      CHAR ipAddr[ 50 ] = { 0 } ;
      if ( _sock )
      {
         _sock->getLocalAddress( ipAddr, sizeof( ipAddr ) ) ;
         return string( ipAddr ) ;
      }
      return "" ;
   }

   string _sptSshSession::getPeerIPAddr()
   {
      CHAR ipAddr[ 50 ] = { 0 } ;
      if ( _sock )
      {
         _sock->getPeerAddress( ipAddr, sizeof( ipAddr ) ) ;
         return string( ipAddr ) ;
      }
      return "" ;
   }

   INT32 _sptSshSession::open()
   {
      INT32 rc = SDB_OK ;

      _sock = SDB_OSS_NEW _ossSocket( _host.c_str(), _port ) ;
      if ( NULL == _sock )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _sock->initSocket() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init socket:%d", rc ) ;
         goto error ;
      }

      rc = _sock->setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                                OSS_SOCKET_KEEP_INTERVAL,
                                OSS_SOCKET_KEEP_CONTER ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set keep alive:%d", rc ) ;
         goto error ;
      }
      
      rc = _sock->connect( SPT_SOCKET_TIMEOUT ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "can not connect to host:%s:%d, rc:%d",
                 _host.c_str(), _port, rc ) ;
         goto error ;
      }

      rc = _openSshSession() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open ssh session:%d", rc ) ;
         goto error ;
      }
      _isOpen = TRUE ;

   done:
      return rc ;
   error:
      if ( NULL != _sock )
      {
         _sock->close() ;
         SAFE_OSS_DELETE( _sock ) ;
      }
      goto done ;
   }
}

