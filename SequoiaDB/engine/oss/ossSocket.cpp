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

   Source File Name = ossSocket.cpp

   Descriptive Name = Operating System Services Socket

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for socket
   operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossSocket.hpp"
#include <stdio.h>
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined (_WINDOWS)
#include <mstcpip.h>
#endif
#ifdef SDB_SSL
#include "ossSSLWrapper.h"
#endif

const UINT32 MAX_INTR_RETRIES = 5000 ;

// Create a listening socket
// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSSK__OSSSK, "_ossSocket::_ossSocket" )
_ossSocket::_ossSocket ( UINT32 port, INT32 timeoutMilli )
{
   PD_TRACE_ENTRY ( SDB__OSSSK__OSSSK );
   _init = FALSE ;
   _fd = SOCKET_INVALIDSOCKET ;
   _timeout = timeoutMilli ;
   _closeWhenDestruct = TRUE ;
   ossMemset ( &_sockAddress, 0, sizeof(sockaddr_in) ) ;
   ossMemset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof (_peerAddress) ;

   ossInitSocket() ;

   _sockAddress.sin_family = AF_INET ;
   _sockAddress.sin_addr.s_addr = htonl ( INADDR_ANY ) ;
   _sockAddress.sin_port = htons ( port ) ;
   _addressLen = sizeof ( _sockAddress ) ;
#ifdef SDB_SSL
   _sslHandle = NULL;
#endif
   PD_TRACE_EXIT ( SDB__OSSSK__OSSSK );
}

// Create a connecting socket
// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSSK__OSSSK2, "_ossSocket::_ossSocket" )
_ossSocket::_ossSocket ( const CHAR *pHostname, UINT32 port,
                         INT32 timeoutMilli )
{
   PD_TRACE_ENTRY ( SDB__OSSSK__OSSSK2 );
   struct hostent *hp ;
   _init = FALSE ;
   _closeWhenDestruct = TRUE;
   _timeout = timeoutMilli ;
   _fd = SOCKET_INVALIDSOCKET ;
   ossMemset ( &_sockAddress, 0, sizeof(sockaddr_in) ) ;
   ossMemset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof (_peerAddress) ;

   ossInitSocket() ;

   _sockAddress.sin_family = AF_INET ;
#if defined (_WINDOWS)
   if ( (hp = gethostbyname ( pHostname )))
#elif defined (_LINUX)
   struct hostent hent ;
   struct hostent *retval = NULL ;
   INT32 error            = 0 ;
   CHAR hbuf[8192]        = {0} ;
   hp                     = &hent ;

   if ( (0 == gethostbyname_r ( pHostname, &hent, hbuf, sizeof(hbuf),
                                &retval, &error )) &&
         NULL != retval )
#elif defined (_AIX)
   struct hostent hent ;
   struct hostent_data hent_data ;
   hp = &hent ;
   if ( (0 == gethostbyname_r ( pHostname, &hent, &hent_data ) ) )
#endif
   {
      UINT32 *pAddr = (UINT32 *)hp->h_addr_list[0] ;
      if ( pAddr )
      {
         _sockAddress.sin_addr.s_addr = *pAddr ;
      }
   }
   else
   {
      _sockAddress.sin_addr.s_addr = inet_addr ( pHostname ) ;
   }
   _sockAddress.sin_port = htons ( port ) ;
   _addressLen = sizeof ( _sockAddress ) ;
#ifdef SDB_SSL
   _sslHandle = NULL;
#endif
   PD_TRACE_EXIT ( SDB__OSSSK__OSSSK2 );
}
// Create from a existing socket
// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSSK__OSSSK3, "_ossSocket::_ossSocket" )
_ossSocket::_ossSocket ( SOCKET *sock, INT32 timeoutMilli )
{
   SDB_ASSERT ( sock, "Input sock is NULL" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSSK__OSSSK3 );
   _fd = *sock ;
   _init = TRUE ;
   _timeout = timeoutMilli ;
   _closeWhenDestruct = TRUE ;
   _addressLen = sizeof ( _sockAddress ) ;

   ossMemset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof ( _peerAddress ) ;

   ossInitSocket() ;

   rc = getsockname ( _fd, (sockaddr*)&_sockAddress, &_addressLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to getsockname from socket %d", _fd ) ;
      _init = FALSE ;
   }
   else
   {
      //get peer address
      rc = getpeername ( _fd, (sockaddr*)&_peerAddress, &_peerAddressLen ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to getpeername from socket %d", _fd ) ;
      }
   }
   setTimeout ( _timeout ) ;
#ifdef SDB_SSL
   _sslHandle = NULL;
#endif
   PD_TRACE_EXIT ( SDB__OSSSK__OSSSK3 );
}

_ossSocket::~_ossSocket ()
{
   if ( _closeWhenDestruct )
   {
      close () ;
   }

#ifdef SDB_SSL
   if ( NULL != _sslHandle )
   {
      ossSSLFreeHandle ( &_sslHandle ) ;
   }
#endif
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_INITTSK, "ossSocket::initSocket" )
INT32 _ossSocket::initSocket ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_INITTSK );
   if ( _init )
   {
      goto done ;
   }
   ossMemset ( &_peerAddress, 0, sizeof(sockaddr_in) ) ;
   _peerAddressLen = sizeof ( _peerAddress ) ;

   _fd = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ;
   if ( SOCKET_INVALIDSOCKET == _fd )
   {
      PD_LOG ( PDERROR, "Failed to initialize socket, errno: %d( %s )",
               SOCKET_GETLASTERROR,
               ossGetLastErrorMsg( SOCKET_GETLASTERROR ) ) ;
      rc = ( SOCKET_EMFILE == SOCKET_GETLASTERROR ) ?
           SDB_TOO_MANY_OPEN_FD : SDB_NETWORK ;
      goto error ;
   }
   _init = TRUE ;
   // settimeout should always return SDB_OK
   setTimeout ( _timeout ) ;
done :
   PD_TRACE_EXITRC ( SDB_OSSSK_INITTSK, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_SETSKLI, "ossSocket::setSocketLi" )
INT32 _ossSocket::setSocketLi ( INT32 lOnOff, INT32 linger )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_SETSKLI );
   SDB_ASSERT ( _init, "socket is not initialized" ) ;

   struct linger _linger ;
   _linger.l_onoff = lOnOff ;
   _linger.l_linger = linger ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   rc = setsockopt ( _fd, SOL_SOCKET, SO_LINGER,
                     (const char*)&_linger, sizeof (_linger) ) ;

done:
   PD_TRACE_EXITRC ( SDB_OSSSK_SETSKLI, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_KPAL, "ossSocket::setKeepAlive" )
INT32 _ossSocket::setKeepAlive( INT32 keepAlive, INT32 keepIdle,
                                INT32 keepInterval, INT32 keepCount )
{
   INT32 rc = SDB_OK ;
#if defined (_WINDOWS)
   struct tcp_keepalive alive_in ;
   DWORD ulBytesReturn       = 0 ;
#endif
   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   // set keep alive options
#if defined (_WINDOWS)
   alive_in.onoff             = keepAlive ;
   alive_in.keepalivetime     = keepIdle * 1000 ; // ms
   alive_in.keepaliveinterval = keepInterval * 1000 ; // ms
   rc = setsockopt( _fd, SOL_SOCKET, SO_KEEPALIVE,
                    ( CHAR *)&keepAlive, sizeof(keepAlive) ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   rc = WSAIoctl( _fd, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
                  NULL, 0, &ulBytesReturn, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
#else
   rc = setsockopt( _fd, SOL_SOCKET, SO_KEEPALIVE,
                    ( void *)&keepAlive, sizeof(keepAlive) ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   #if defined (_AIX)
      #define SOL_TCP IPPROTO_TCP
   #endif
   rc = setsockopt( _fd, SOL_TCP, TCP_KEEPIDLE,
                    ( void *)&keepIdle, sizeof(keepIdle) ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   rc = setsockopt( _fd, SOL_TCP, TCP_KEEPINTVL,
                    ( void *)&keepInterval, sizeof(keepInterval) ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   rc = setsockopt( _fd, SOL_TCP, TCP_KEEPCNT,
                    ( void *)&keepCount, sizeof(keepCount) ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
#endif // _WINDOWS

done:
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_BIND_LSTN, "ossSocket::bind_listen" )
INT32 _ossSocket::bind_listen ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_BIND_LSTN );
   INT32 temp = 1 ;
   SDB_ASSERT ( _init, "socket is not initialized" ) ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   // Allows the socket to be bound to an address that is already in use.
   // For database shutdown and restart right away, before socket close
   rc = setsockopt ( _fd, SOL_SOCKET, SO_REUSEADDR,
                     (char*)&temp, sizeof (INT32) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt SO_REUSEADDR, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   rc = setSocketLi( 1, 30 ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt SO_LINGER, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
   rc = ::bind ( _fd, (struct sockaddr *)&_sockAddress, _addressLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to bind socket, rc = %d",
               SOCKET_GETLASTERROR ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = listen ( _fd, SOMAXCONN ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to listen socket, rc = %d",
               SOCKET_GETLASTERROR ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_OSSSK_BIND_LSTN, rc );
   return rc ;
error :
   close () ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_SEND, "ossSocket::send" )
INT32 _ossSocket::send ( const CHAR *pMsg, INT32 len,
                         INT32 &sentLen,
                         INT32 timeout, INT32 flags,
                         BOOLEAN block )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_SEND );
   SDB_ASSERT ( pMsg, "message is NULL" ) ;

   UINT32 retries = 0 ;
   sentLen = 0 ;
   SOCKET maxFD = _fd ;
   struct timeval maxSelectTime ;
   fd_set fds ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   maxSelectTime.tv_sec = timeout / 1000 ;
   maxSelectTime.tv_usec = ( timeout % 1000 ) * 1000 ;
   // if we don't expect to receive anything, no need to continue
   if ( 0 == len )
   {
      return SDB_OK ;
   }

   // wait loop until the socket is ready
   while ( TRUE )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( _fd, &fds ) ;
      rc = select ( maxFD + 1, NULL, &fds, NULL,
                    timeout>=0?&maxSelectTime:NULL ) ;

      // 0 means timeout
      if ( 0 == rc )
      {
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      // if < 0, means something wrong
      if ( 0 > rc )
      {
         rc = SOCKET_GETLASTERROR ;
         // if we failed due to interrupt, let's continue
         if ( SOCKET_EINTR == rc )
         {
            continue ;
         }
         PD_LOG ( PDERROR, "Failed to select from socket, errno: %d( %s )",
                  rc, ossGetLastErrorMsg( rc ) ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }

      // if the socket we interested is not receiving anything, let's continue
      if ( FD_ISSET ( _fd, &fds ) )
      {
         break ;
      }
   }
   while ( len > 0 )
   {
#ifdef SDB_SSL
      if ( NULL != _sslHandle )
      {
         rc = ossSSLWrite ( _sslHandle, pMsg, len ) ;
         if ( rc <= 0 )
         {
            if ( SSL_AGAIN == rc )
            {
               rc = SDB_TIMEOUT ;
               goto error;
            }

            INT32 error = ossSSLGetError ( _sslHandle ) ;
            char* errorMsg = ossSSLGetErrorMessage ( error ) ;
            PD_LOG ( PDERROR, "SSL failed to send, errno: %d( %s )",
                     error, errorMsg ) ;
            rc = SDB_NETWORK ;
            goto error;
         }
      }
      else
#endif /* SDB_SSL */
      {
#if defined (_WINDOWS)
      rc = ::send ( _fd, pMsg, len, flags ) ;
      if ( SOCKET_ERROR == rc )
#else
      // MSG_NOSIGNAL : Requests not to send SIGPIPE on errors on stream
      // oriented sockets when the other end breaks the connection. The EPIPE
      // error is still returned.
      rc = ::send ( _fd, pMsg, len, MSG_NOSIGNAL|flags ) ;
      if ( -1 == rc )
#endif
      {
         rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
         if ( WSAETIMEDOUT == rc && _timeout > 0 )
#else
         if ( (EAGAIN == rc || EWOULDBLOCK == rc || ETIMEDOUT == rc ) &&
              _timeout > 0 )
#endif
         {
            rc = SDB_TIMEOUT ;
            goto error ;
         }
         if ( SOCKET_EINTR == rc && retries < MAX_INTR_RETRIES )
         {
            // less than max_recv_retries number, let's retry
            retries ++ ;
            continue ;
         }
         PD_LOG ( PDERROR, "Failed to send, errno: %d( %s )",
                  SOCKET_GETLASTERROR,
                  ossGetLastErrorMsg( SOCKET_GETLASTERROR ) ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }
      }

      sentLen += rc ;
      len -= rc ;
      pMsg += rc ;

      // non-block
      if ( !block )
      {
         break ;
      }
   }
   rc = SDB_OK ;
done :
   PD_TRACE_EXITRC ( SDB_OSSSK_SEND, rc );
   return rc ;
error :
   if ( SDB_NETWORK == rc )
   {
      close() ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_ISCONN, "ossSocket::isConnected" )
BOOLEAN _ossSocket::isConnected ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_ISCONN );

   if ( !_init )
   {
      PD_TRACE_EXIT ( SDB_OSSSK_ISCONN );
      return FALSE ;
   }

#if defined (_WINDOWS)
   rc = ::send ( _fd, "", 0, 0 ) ;
   if ( SOCKET_ERROR == rc )
#elif defined (_AIX)
   rc = ::send ( _fd, "", 0, 0 ) ;
   if ( 0 == rc )
#else
   // MSG_NOSIGNAL : Requests not to send SIGPIPE on errors on stream
   // oriented sockets when the other end breaks the connection. The EPIPE
   // error is still returned.
   //rc = ::send ( _fd, "", 0, MSG_NOSIGNAL ) ;
   //if ( 0 > rc )
   rc = ::recv ( _fd, NULL, 0, MSG_DONTWAIT ) ;
   if ( 0 == rc )
#endif
   {
      PD_TRACE_EXIT ( SDB_OSSSK_ISCONN );
      return FALSE ;
   }
   PD_TRACE_EXIT ( SDB_OSSSK_ISCONN );
   return TRUE ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_RECV, "ossSocket::recv" )
INT32 _ossSocket::recv ( CHAR *pMsg, INT32 len,
                         INT32 &receivedLen,
                         INT32 timeout, INT32 flags,
                         BOOLEAN block, BOOLEAN recvRawData )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_RECV );
   SDB_ASSERT ( pMsg, "message is NULL" ) ;
   UINT32 retries = 0 ;
   SOCKET maxFD = _fd ;
   struct timeval maxSelectTime ;
   fd_set fds ;
   receivedLen = 0 ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   // if we don't expect to receive anything, no need to continue
   if ( 0 == len )
   {
      goto done ;
   }

#ifdef SDB_SSL
   if ( NULL != _sslHandle && !recvRawData )
   {
      while ( len > 0 )
      {
         if ( flags & MSG_PEEK )
         {
            rc = ossSSLPeek ( _sslHandle, pMsg, len ) ;
         }
         else
         {
            rc = ossSSLRead ( _sslHandle, pMsg, len ) ;
         }

         if ( rc <= 0 )
         {
            if ( SSL_AGAIN == rc || SSL_TIMEOUT == rc)
            {
               rc = SDB_TIMEOUT ;
               goto error;
            }

            INT32 err = ossSSLGetError ( _sslHandle ) ;
            char* errMsg = ossSSLGetErrorMessage ( err ) ;
            PD_LOG ( PDERROR, "SSL failed to recv, errno: %d( %s )",
                     err, errMsg ) ;
            rc = SDB_NETWORK ;
            goto error;
         }

         receivedLen += rc ;
         len -= rc ;
         pMsg += rc ;
         rc = SDB_OK;

         if ( flags & MSG_PEEK )
         {
            goto done;
         }

         // non-block
         if ( !block )
         {
            goto done ;
         }
      }

      rc = SDB_OK;
      goto done;
   }
#endif /* SDB_SSL */

   maxSelectTime.tv_sec = timeout / 1000 ;
   maxSelectTime.tv_usec = ( timeout % 1000 ) * 1000 ;
   // wait loop until either we timeout or get a message
   while ( true )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( _fd, &fds ) ;
      rc = select ( maxFD + 1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL ) ;

      // 0 means timeout
      if ( 0 == rc )
      {
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      // if < 0, means something wrong
      if ( 0 > rc )
      {
         rc = SOCKET_GETLASTERROR ;
         // if we failed due to interrupt, let's continue
         if ( SOCKET_EINTR == rc )
         {
            continue ;
         }
         PD_LOG ( PDERROR, "Failed to select from socket, rc = %d", rc) ;
         rc = SDB_NETWORK ;
         goto error ;
      }

      // if the socket we interested is not receiving anything, let's continue
      if ( FD_ISSET ( _fd, &fds ) )
      {
         break ;
      }
   }
   // Once we start receiving message, there's no chance to timeout, in order to
   // prevent partial read
   while ( len > 0 )
   {
#if defined (_WINDOWS)
      rc = ::recv ( _fd, pMsg, len, flags ) ;
#else
      // MSG_NOSIGNAL : Requests not to send SIGPIPE on errors on stream
      // oriented sockets when the other end breaks the connection. The EPIPE
      // error is still returned.
      rc = ::recv ( _fd, pMsg, len, MSG_NOSIGNAL|flags ) ;
#endif
      if ( rc > 0 )
      {
         if ( flags & MSG_PEEK )
         {
            goto done ;
         }
         receivedLen += rc ;
         len -= rc ;
         pMsg += rc ;

         // non-block
         if ( !block )
         {
            break ;
         }
      }
      else if ( rc == 0 )
      {
         PD_LOG ( PDERROR, "Peer unexpected shutdown" ) ;
         rc = SDB_NETWORK_CLOSE ;
         goto error ;
      }
      else
      {
         // if rc < 0
         rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
         if ( WSAETIMEDOUT == rc && _timeout > 0 )
#else
         if ( (EAGAIN == rc || EWOULDBLOCK == rc || ETIMEDOUT == rc ) &&
              _timeout > 0 )
#endif
         {
            rc = SDB_TIMEOUT ;
            goto error ;
         }
         if ( SOCKET_EINTR == rc && retries < MAX_INTR_RETRIES )
         {
            // less than max_recv_retries number, let's retry
            retries ++ ;
            continue ;
         }
         // something bad when get here
         PD_LOG ( PDERROR, "Recv() Failed: rc = %d", rc ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }
   }
   // Everything is fine when get here
   rc = SDB_OK ;
done :
   PD_TRACE_EXITRC ( SDB_OSSSK_RECV, rc );
   return rc ;
error :
   if ( SDB_NETWORK == rc || SDB_NETWORK_CLOSE == rc )
   {
      close() ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_CONNECT, "ossSocket::connect" )
INT32 _ossSocket::connect ( INT32 timeout )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_CONNECT );

   SDB_ASSERT ( !_peerAddress.sin_addr.s_addr,
                "Cannot connect without close/init" ) ;

#if defined (_LINUX) || defined (_AIX)

   INT32 flags = fcntl( native(), F_GETFL, 0) ;
   if ( fcntl( native(), F_SETFL, flags | O_NONBLOCK ) <0 )
   {
      PD_LOG( PDERROR, "failed to fcntl sock:%d",native() ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = ::connect ( _fd, (struct sockaddr *) &_sockAddress, _addressLen ) ;
   if ( rc != SDB_OK )
   {
      if ( SOCKET_GETLASTERROR == EINPROGRESS )
      {
         rc = _complete( timeout ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to complete connect, rc = %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG ( PDERROR, "Failed to connect, rc = %d", SOCKET_GETLASTERROR ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }
   }
   else
   {
      /// do nothing.
   }

   if ( fcntl( native(), F_SETFL, flags & ~O_NONBLOCK ) <0 )
   {
      PD_LOG( PDERROR, "failed to fcntl sock: %d",native() ) ;
      rc = SDB_SYS ;
      goto error ;
   }
#elif defined (_WINDOWS)

   rc = ::connect ( _fd, (struct sockaddr *) &_sockAddress, _addressLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to connect, rc = %d", SOCKET_GETLASTERROR ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
#endif

   //get local address
   rc = getsockname ( _fd, (sockaddr*)&_sockAddress, &_addressLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get local address, rc = %d", rc ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
   //get peer address
   rc = getpeername ( _fd, (sockaddr*)&_peerAddress, &_peerAddressLen ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get peer address, rc = %d", rc ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
   // if the local addr is the same with remote addr
   if ( _sockAddress.sin_port == _peerAddress.sin_port &&
        _sockAddress.sin_addr.s_addr == _peerAddress.sin_addr.s_addr )
   {
      PD_LOG( PDERROR, "Local addr is the same with remote addr, "
              "local port: %u, remote port: %u", getLocalPort(),
              getPeerPort() ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_OSSSK_CONNECT, rc );
   return rc ;
error :
   close() ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_CLOSE, "ossSocket::close" )
void _ossSocket::close ()
{
   PD_TRACE_ENTRY ( SDB_OSSSK_CLOSE );
   if ( _init )
   {
#ifdef SDB_SSL
      if ( NULL != _sslHandle )
      {
         ossSSLShutdown ( _sslHandle ) ;
         ossSSLFreeHandle ( &_sslHandle ) ;
      }
#endif

#if defined (_WINDOWS)
      closesocket ( _fd ) ;
#else
      ::close ( _fd ) ;
#endif
      _fd = SOCKET_INVALIDSOCKET ;
      _init = FALSE ;
   }
   PD_TRACE_EXIT ( SDB_OSSSK_CLOSE );
}

INT32 _ossSocket::accept ( SOCKET *sock, struct sockaddr *addr, socklen_t
                           *addrlen, INT32 timeout )
{
   INT32 rc = SDB_OK ;
   SOCKET maxFD = _fd ;
   INT32 sysError = 0 ;
   struct timeval maxSelectTime ;
   SDB_ASSERT ( sock, "Output sock is NULL" ) ;

   fd_set fds ;
   maxSelectTime.tv_sec = timeout / 1000 ;
   maxSelectTime.tv_usec = ( timeout % 1000 ) * 1000 ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   while ( true )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( _fd, &fds ) ;
      rc = select ( maxFD + 1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL ) ;

      // 0 means timeout
      if ( 0 == rc )
      {
         *sock = 0 ;
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      // if < 0, means something wrong
      if ( 0 > rc )
      {
         sysError = SOCKET_GETLASTERROR ;
         // if we failed due to interrupt, let's continue
         if ( SOCKET_EINTR == sysError )
         {
            continue ;
         }
         PD_LOG ( PDERROR, "Failed to select from socket, rc = %d",
                  sysError );
         rc = SDB_NETWORK ;
         goto error ;
      }

      // if the socket we interested is not receiving anything, let's continue
      if ( FD_ISSET ( _fd, &fds ) )
      {
         break ;
      }
   }
   // reset rc back to SDB_OK, since the rc now is the result from select()
   rc = SDB_OK ;
   *sock = ::accept ( _fd, addr, addrlen ) ;
   if ( SOCKET_INVALIDSOCKET == *sock )
   {
      sysError = SOCKET_GETLASTERROR ;
      rc = ( SOCKET_EMFILE == sysError ) ? SDB_TOO_MANY_OPEN_FD : SDB_NETWORK ;
      PD_LOG ( ( rc == SDB_NETWORK ? PDERROR : PDINFO ) ,
               "Failed to accept socket, errno: %d( %s ), rc: %d",
               sysError, ossGetLastErrorMsg( sysError ), rc ) ;
      goto error ;
   }

done :
   return rc ;
error :
   if ( rc != SDB_TOO_MANY_OPEN_FD )
   {
      close () ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_DISNAG, "ossSocket::disableNagle" )
INT32 _ossSocket::disableNagle ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_DISNAG );
   INT32 temp = 1 ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   rc = setsockopt ( _fd, IPPROTO_TCP, TCP_NODELAY, (CHAR *) &temp,
                     sizeof ( INT32 ) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }

done:
   PD_TRACE_EXITRC ( SDB_OSSSK_DISNAG, rc );
   return rc ;
error:
   goto done ;
}

void _ossSocket::quickAck ()
{
#if defined( _LINUX ) || defined (_AIX)
   #if defined (_AIX)
         #define TCP_QUICKACK TCP_NODELAYACK
   #endif
   if ( _init )
   {
      INT32 i = 1 ;
      setsockopt( _fd, IPPROTO_TCP, TCP_QUICKACK, (void*)&i, sizeof(i) ) ;
   }
#endif
}

#ifdef SDB_SSL

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_SECURE, "ossSocket::secure" )
INT32 _ossSocket::secure ()
{
   SSLContext* ctx = NULL ;
   INT32 ret ;

   PD_TRACE_ENTRY ( SDB_OSSSK_SECURE );

   if ( NULL != _sslHandle )
   {
      PD_LOG ( PDERROR, "_sslHandle already exists" ) ;
      ret = SDB_NETWORK ;
      // do not free _sslHandle
      goto error2 ;
   }

   if ( SOCKET_INVALIDSOCKET == _fd )
   {
      PD_LOG ( PDERROR, "_fd can't be invalid" ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   ctx = ossSSLGetContext () ;
   if ( NULL == ctx )
   {
      PD_LOG ( PDERROR, "failed to get SSL context" ) ;
      ret = SDB_NETWORK ;
      goto error;
   }

   ret = ossSSLNewHandle ( &_sslHandle, ctx, _fd, NULL, 0 ) ;
   if ( SSL_OK != ret )
   {
      INT32 error = ossSSLERRGetError () ;
      char* errorMsg = ossSSLERRGetErrorMessage ( error ) ;
      PD_LOG ( PDERROR, "failed to create SSL handle, errno: %d( %s )",
               error, errorMsg ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   ret = ossSSLConnect ( _sslHandle ) ;
   if ( SSL_OK != ret )
   {
      INT32 error = ossSSLGetError ( _sslHandle ) ;
      char* errorMsg = ossSSLGetErrorMessage ( error ) ;
      PD_LOG ( PDERROR, "SSL failed to connect, errno: %d( %s )",
               error, errorMsg ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   PD_LOG ( PDEVENT, "create a SSL connection" ) ;
   ret = SDB_OK ;

done:
   PD_TRACE_EXITRC ( SDB_OSSSK_SECURE, ret );
   return ret;
error:
   if ( NULL != _sslHandle )
   {
      ossSSLFreeHandle ( &_sslHandle ) ;
   }
   goto done;
error2: // do not free _sslHandle
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_DOSSLHANDSHAKE, "ossSocket::doSSLHandshake" )
INT32 _ossSocket::doSSLHandshake ( const CHAR* initialBytes, INT32 len )
{
   SSLContext* ctx = NULL ;
   INT32 ret ;

   PD_TRACE_ENTRY ( SDB_OSSSK_DOSSLHANDSHAKE );

   SDB_ASSERT ( len >= 0, "len must be >= 0" ) ;

   if ( NULL != _sslHandle )
   {
      PD_LOG ( PDERROR, "_sslHandle already exists" ) ;
      ret = SDB_NETWORK ;
      // do not free _sslHandle
      goto error2 ;
   }

   if ( SOCKET_INVALIDSOCKET == _fd )
   {
      PD_LOG ( PDERROR, "_fd can't be invalid" ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   ctx = ossSSLGetContext () ;
   if ( NULL == ctx )
   {
      PD_LOG ( PDERROR, "failed to get SSL context" ) ;
      ret = SDB_NETWORK ;
      goto error;
   }

   ret = ossSSLNewHandle ( &_sslHandle, ctx, _fd, initialBytes, len ) ;
   if ( SSL_OK != ret )
   {
      INT32 error = ossSSLERRGetError () ;
      char* errorMsg = ossSSLERRGetErrorMessage ( error ) ;
      PD_LOG ( PDERROR, "Failed to create SSL handle, errno: %d( %s )",
               error, errorMsg ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   ret = ossSSLAccept ( _sslHandle ) ;
   if ( SSL_OK != ret )
   {
      INT32 error = ossSSLGetError ( _sslHandle ) ;
      char* errorMsg = ossSSLGetErrorMessage ( error ) ;
      PD_LOG ( PDERROR, "SSL failed to accept, errno: %d( %s )",
               error, errorMsg ) ;
      ret = SDB_NETWORK ;
      goto error ;
   }

   PD_LOG ( PDEVENT, "accept a SSL connection" ) ;
   ret = SDB_OK ;

done:
   PD_TRACE_EXITRC ( SDB_OSSSK_DOSSLHANDSHAKE, ret );
   return ret;
error:
   if ( NULL != _sslHandle )
   {
      ossSSLFreeHandle ( &_sslHandle ) ;
   }
   goto done;
error2: // do not free _sslHandle
   goto done;
}

#endif /* SDB_SSL */

UINT32 _ossSocket::_getPort ( sockaddr_in *addr )
{
   return ntohs ( addr->sin_port ) ;
}

UINT32 _ossSocket::_getIP( sockaddr_in * addr )
{
   return ntohl( addr->sin_addr.s_addr ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK__GETADDR, "ossSocket::_getAddress" )
INT32 _ossSocket::_getAddress ( sockaddr_in *addr, CHAR *pAddress,
                                UINT32 length )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK__GETADDR );

   length = length < NI_MAXHOST ? length : NI_MAXHOST ;
   rc = getnameinfo ( (struct sockaddr *)addr, sizeof(sockaddr), pAddress,
                      length, NULL, 0, NI_NUMERICHOST ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to getnameinfo, rc = %d",
               SOCKET_GETLASTERROR ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSSK__GETADDR, rc );
   return rc ;
error :
   goto done ;
}

UINT32 _ossSocket::getLocalPort ()
{
   return _getPort ( &_sockAddress ) ;
}

UINT32 _ossSocket::getPeerPort ()
{
   return _getPort ( &_peerAddress ) ;
}

UINT32 _ossSocket::getPeerIP()
{
   return _getIP( &_peerAddress ) ;
}

UINT32 _ossSocket::getLocalIP()
{
   return _getIP( &_sockAddress ) ;
}

INT32 _ossSocket::getLocalAddress ( CHAR * pAddress, UINT32 length )
{
   return _getAddress ( &_sockAddress, pAddress, length ) ;
}

INT32 _ossSocket::getPeerAddress ( CHAR * pAddress, UINT32 length )
{
   return _getAddress ( &_peerAddress, pAddress, length ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSK_SETTMOUT, "ossSocket::setTimeout" )
INT32 _ossSocket::setTimeout ( INT32 milliSeconds )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSSK_SETTMOUT );
   struct timeval tv ;

   PD_CHECK( _init, SDB_SYS, error, PDWARNING, "Socket is not init" ) ;

   tv.tv_sec = milliSeconds / 1000 ;
   tv.tv_usec = ( milliSeconds % 1000 ) * 1000 ;
   // windows take milliseconds as parameter
   // but linux takes timeval as input
#if defined (_WINDOWS)
   // convert microseconds to milliseconds in DWORD
   tv.tv_sec = milliSeconds ;
   rc = setsockopt ( _fd, SOL_SOCKET, SO_RCVTIMEO, ( char* ) &tv.tv_sec,
                     sizeof ( INT32 ) ) ;
   if ( SOCKET_ERROR == rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }

   rc = setsockopt ( _fd, SOL_SOCKET, SO_SNDTIMEO, ( char* ) &tv.tv_sec,
                     sizeof ( INT32 ) ) ;
   if ( SOCKET_ERROR == rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
#else
   rc = setsockopt ( _fd, SOL_SOCKET, SO_RCVTIMEO, ( char* ) &tv,
                     sizeof ( tv ) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }

   rc = setsockopt ( _fd, SOL_SOCKET, SO_SNDTIMEO, ( char* ) &tv,
                     sizeof ( tv ) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d",
               SOCKET_GETLASTERROR ) ;
   }
#endif

done:
   PD_TRACE_EXITRC ( SDB_OSSSK_SETTMOUT, rc );
   return rc ;
error:
   goto done ;
}

INT32 _ossSocket::getHostName ( CHAR *pName, INT32 nameLen )
{
   return ossGetHostName( pName, nameLen ) ;
}

INT32 _ossSocket::getPort ( const CHAR *pServiceName, UINT16 &port )
{
   return ossGetPort( pServiceName, port ) ;
}

#if defined (_LINUX) || defined (_AIX)

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSSK__COMPLETE, "_ossSocket::_complete" )
INT32 _ossSocket::_complete( INT32 timeout )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB__OSSSK__COMPLETE ) ;

   INT32 err = 0 ;
   socklen_t errlen = sizeof(err) ;
   timeval tv ;
   tv.tv_sec = timeout / 1000 ; ;
   tv.tv_usec = ( timeout % 1000 ) * 1000 ;
   fd_set wfd ;
   FD_ZERO( &wfd ) ;
   FD_SET( _fd, &wfd ) ;

retry:
   if ( -1 == ::select( _fd + 1, NULL, &wfd, NULL, &tv ) )
   {
      rc = SOCKET_GETLASTERROR ;
      // if we failed due to interrupt, let's retry
      if ( SOCKET_EINTR == rc )
      {
         goto retry ;
      }
      PD_LOG( PDERROR, "select(2), error: %d( %s )",
              rc, ossGetLastErrorMsg( rc ) ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !FD_ISSET( _fd, &wfd ) )
   {
      errno = ETIMEDOUT ;
      PD_LOG( PDERROR, "connect timeout" ) ;
      rc = SDB_TIMEOUT ;
      goto error ;
   }

   if ( ::getsockopt( _fd, SOL_SOCKET, SO_ERROR,
                      &err, &errlen ) < 0 )
   {
      PD_LOG( PDERROR, "failed to getsockopt" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( SDB_OK != err )
   {
      errno = err ;
      PD_LOG( PDERROR, "failed to connect to remote, errno: %d( %s )",
              errno, ossGetLastErrorMsg( errno ) ) ;
      rc = SDB_NET_CANNOT_CONNECT ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB__OSSSK__COMPLETE, rc ) ;
   return rc ;
error:
   close() ;
   goto done ;
}

#else

INT32 _ossSocket::_complete( INT32 timeout )
{
   return SDB_OK ;
}

#endif // (_LINUX) || defined (_AIX)

// socket functions implement:

INT32 ossInitSocket()
{
#if defined (_WINDOWS)
   static BOOLEAN socketInitialized = FALSE ;

   if ( !socketInitialized )
   {
      WSADATA data = {0} ;
      INT32 rc = WSAStartup ( MAKEWORD ( 2,2 ), &data ) ;
      if ( 0 != rc )
      {
         // The WSAStartup function directly returns the extended error code in
         // the return value for this function
         // A call to the WSAGetLastError function is not needed and should not
         // be used.
         PD_LOG ( PDERROR, "Failed to startup socket, rc = %d", rc ) ;
         return SDB_NETWORK ;
      }
      else
      {
         socketInitialized = TRUE ;
      }
   }
#endif // _WINDOWS
   return SDB_OK ;
}

INT32 ossGetHostName( CHAR * pName, INT32 nameLen )
{
   INT32 rc = ossInitSocket() ;

   if ( SDB_OK == rc )
   {
      rc = gethostname ( pName, nameLen ) ;
   }
   return rc ;
}

INT32 ossGetPort( const CHAR * pServiceName, UINT16 & port )
{
   INT32 rc = ossInitSocket() ;

   if ( SDB_OK == rc )
   {
      struct servent *servinfo ;
      servinfo = getservbyname ( pServiceName, "tcp" ) ;
      if ( !servinfo )
      {
         INT32 tmpPort = atoi ( pServiceName ) ;
         if ( 0 >= tmpPort || 65535 < tmpPort )
         {
            rc = SDB_INVALIDARG ;
         }
         port = tmpPort ;
      }
      else
      {
         port = (UINT16)ntohs(servinfo->s_port) ;
      }
   }
   return rc ;
}

INT32 ossGetAddrInfo( sockaddr_in * addr, CHAR * pAddress, UINT32 length,
                      UINT16 * pPort )
{
   INT32 rc = ossInitSocket() ;

   if ( SDB_OK == rc )
   {
      length = length < OSS_MAX_HOSTNAME ? length : OSS_MAX_HOSTNAME ;
      rc = getnameinfo ( (struct sockaddr *)addr, sizeof(sockaddr), pAddress,
                         length, NULL, 0, NI_NUMERICHOST ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to getnameinfo, rc = %d",
                  SOCKET_GETLASTERROR ) ;
         rc = SDB_NETWORK ;
      }
      if ( pPort )
      {
         *pPort = ntohs ( addr->sin_port ) ;
      }
   }

   return rc ;
}

INT32 ossIP2Str( UINT32 ip, CHAR * pStr, INT32 nameLen )
{
   struct sockaddr_in sockAddr ;
   ossMemset( &sockAddr, 0, sizeof(sockAddr) ) ;
   sockAddr.sin_family = AF_INET ;
   sockAddr.sin_addr.s_addr = htonl ( ip ) ;
   sockAddr.sin_port = htons ( 0 ) ;
   return ossGetAddrInfo( &sockAddr, pStr, nameLen, NULL ) ;
}

UINT32 ossStr2IP( const CHAR * pStr )
{
   return (UINT32)inet_addr( pStr ) ;
}


