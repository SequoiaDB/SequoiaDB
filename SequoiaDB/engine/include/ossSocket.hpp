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

   Source File Name = ossSocket.hpp

   Descriptive Name = Operating System Services Socket Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure for socket object.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SOCKET_HPP__
#define OSS_SOCKET_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#if defined (_LINUX) || defined (_AIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#else
//#include <winsock2.h>
//#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <string.h>
#if defined (_WINDOWS)
#define SOCKET_GETLASTERROR   WSAGetLastError()
#define SOCKET_INVALIDSOCKET  INVALID_SOCKET
#define SOCKET_EINTR          WSAEINTR
#define SOCKET_EMFILE         WSAEMFILE
#else
#define SOCKET_GETLASTERROR   errno
#define SOCKET_INVALIDSOCKET  -1
#define SOCKET_EINTR          EINTR
#define SOCKET_EMFILE         EMFILE
#endif
#include "pd.hpp"

// by default 500ms timeout
// note this is default for engine communication
// this value shouldn't be set too big since it may break
// the heartbeat detection
// for client, we should set client socket timeout value instead of this one
#define OSS_SOCKET_DFT_TIMEOUT      500

#define OSS_MAX_HOSTNAME            NI_MAXHOST
#define OSS_MAX_SERVICENAME         NI_MAXSERV

#ifdef SDB_SSL
SDB_EXTERN_C_START
struct SSLHandle ;
SDB_EXTERN_C_END
#endif

// todo: support AF_UNIX later
/*
   _ossSocket define
*/
class _ossSocket : public SDBObject
{
   private :
      SOCKET               _fd ;
      socklen_t            _addressLen ;
      socklen_t            _peerAddressLen ;
      struct sockaddr_in   _sockAddress ;
      struct sockaddr_in   _peerAddress ;

      BOOLEAN              _init ;
      BOOLEAN              _closeWhenDestruct ;
      INT32                _timeout ;

#ifdef SDB_SSL
      SSLHandle*           _sslHandle;
#endif

   protected:
      UINT32   _getPort ( sockaddr_in *addr ) ;
      UINT32   _getIP( sockaddr_in *addr ) ;
      INT32    _getAddress ( sockaddr_in *addr, CHAR *pAddress,
                             UINT32 length ) ;
      INT32    _complete( INT32 timeout ) ;

   public :
      INT32 setSocketLi ( INT32 lOnOff, INT32 linger ) ;
      INT32 setKeepAlive( INT32 keepAlive = 1,
                          INT32 keepIdle = OSS_SOCKET_KEEP_IDLE,
                          INT32 keepInterval = OSS_SOCKET_KEEP_INTERVAL,
                          INT32 keepCount = OSS_SOCKET_KEEP_CONTER ) ;

      // Create a listening socket, timeout in millisecond
      _ossSocket ( UINT32 port, INT32 timeoutMilli = 0 ) ;
      // Create a connecting socket, timeout in millisecond
      _ossSocket ( const CHAR *pHostname, UINT32 port, INT32 timeoutMilli = 0 ) ;
      // Create from a existing socket, timeout in millisecond
      _ossSocket ( SOCKET *sock, INT32 timeoutMilli = 0 ) ;

      ~_ossSocket () ;

      OSS_INLINE SOCKET native()const{ return _fd ; }
      OSS_INLINE void closeWhenDestruct( BOOLEAN closeWhenDestruct )
      {
         _closeWhenDestruct = closeWhenDestruct ;
      }

      INT32 initSocket () ;
      INT32 bind_listen () ;
      BOOLEAN isConnected () ;
      BOOLEAN isClosed() const { return !_init ; }

      INT32 send ( const CHAR *pMsg, INT32 len,
                   INT32 &sentLen,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                   INT32 flags = 0,
                   BOOLEAN block = TRUE ) ;
      INT32 recv ( CHAR *pMsg, INT32 len,
                   INT32 &receivedLen,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                   INT32 flags = 0,
                   BOOLEAN block = TRUE,
                   BOOLEAN recvRawData = FALSE ) ;

      INT32 connect ( INT32 timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
      void  close () ;
      INT32 accept ( SOCKET *sock, struct sockaddr *addr, socklen_t *addrlen,
                     INT32 timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
      INT32 disableNagle () ;
      void  quickAck () ;
#ifdef SDB_SSL
      INT32 secure () ;
      INT32 doSSLHandshake ( const CHAR* initialBytes, INT32 len ) ;
#endif
      UINT32 getPeerPort () ;
      INT32  getPeerAddress ( CHAR *pAddress, UINT32 length ) ;
      UINT32 getPeerIP () ;

      UINT32 getLocalPort () ;
      INT32  getLocalAddress ( CHAR *pAddress, UINT32 length ) ;
      UINT32 getLocalIP () ;

      INT32 setTimeout ( INT32 milliSeconds ) ;

      static INT32 getHostName ( CHAR *pName, INT32 nameLen ) ;
      static INT32 getPort ( const CHAR *pServiceName, UINT16 &port ) ;
} ;

typedef class _ossSocket ossSocket ;

// define socket functions

INT32    ossInitSocket() ;
INT32    ossGetHostName( CHAR *pName, INT32 nameLen ) ;
INT32    ossGetPort( const CHAR *pServiceName, UINT16 &port ) ;

INT32    ossGetAddrInfo( sockaddr_in *addr, CHAR *pAddress, UINT32 length,
                         UINT16 *pPort = NULL ) ;

INT32    ossIP2Str( UINT32 ip, CHAR *pStr, INT32 nameLen ) ;
UINT32   ossStr2IP( const CHAR *pStr ) ;

#endif // OSS_SOCKET_HPP__

