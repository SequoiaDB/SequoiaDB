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

#include "network.h"
#include "ossUtil.h"
#include "ossMem.h"
#include "oss.h"
#if defined (_LINUX) || defined (_AIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#else
#include <mstcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#ifdef SDB_SSL
#include "ossSSLWrapper.h"
#endif
#if defined (_WINDOWS)
#define SOCKET_GETLASTERROR WSAGetLastError()
#else
#define SOCKET_GETLASTERROR errno
#endif

struct Socket
{
   SOCKET      rawSocket ;
#ifdef SDB_SSL
   SSLHandle*  sslHandle ;
#endif
   socketInterruptFunc isInterruptFunc ;
} ;

#if defined (_WINDOWS)
static BOOLEAN sockInitialized = FALSE ;
static ossMutex winSockInitMutex ;
static void _winSockInit( void )
{
   ossMutexInit( &winSockInitMutex ) ;
}
#endif

static INT32 _disableNagle( SOCKET sock ) ;
static void _clientDisconnect ( SOCKET sock ) ;
#ifdef SDB_SSL
static INT32 _clientSecure( Socket* sock ) ;
#endif
#define MAX_RECV_RETRIES 5
#define MAX_SEND_RETRIES 5


INT32 clientConnect ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      BOOLEAN useSSL,
                      Socket** sock )
{
   INT32 rc = SDB_OK ;
   struct hostent *hp = NULL ;
   struct servent *servinfo ;
   struct sockaddr_in sockAddress ;
   Socket* s = NULL ;
   SOCKET rawSocket = -1 ;
   UINT16 port ;
#if defined (_WINDOWS)
   static ossOnce initOnce = OSS_ONCE_INIT;
#endif


   if ( !sock )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

#if defined (_WINDOWS)
   ossOnceRun( &initOnce, _winSockInit );
   if ( FALSE == sockInitialized )
   {
      ossMutexLock( &winSockInitMutex ) ;
      if ( FALSE == sockInitialized )
      {
         WSADATA data = {0} ;
         rc = WSAStartup ( MAKEWORD ( 2,2 ), &data ) ;
         if ( INVALID_SOCKET == rc )
         {
            rc = SDB_NETWORK ;
            goto error ;
         }
         sockInitialized = TRUE ;
      }
      ossMutexUnlock( &winSockInitMutex ) ;
   }
#endif

   ossMemset ( &sockAddress, 0, sizeof(sockAddress) ) ;
   sockAddress.sin_family = AF_INET ;
#if defined (_WINDOWS)
   if ( (hp = gethostbyname ( pHostName ) ) )
#elif defined (_LINUX)
   struct hostent hent ;
   struct hostent *retval = NULL ;
   INT32 error             = 0 ;
   CHAR hbuf[8192]         = { 0 } ;
   hp                      = &hent ;
   if ( (0 == gethostbyname_r ( pHostName, &hent, hbuf, sizeof(hbuf), 
                                &retval, &error )) && NULL != retval )
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
         sockAddress.sin_addr.s_addr = *( pAddr ) ;  
      }
      else 
      {
         rc = SDB_SYS ;
         goto error ;
      }
   }
   else
   {
      sockAddress.sin_addr.s_addr = inet_addr ( pHostName ) ;
   }
   servinfo = getservbyname ( pServiceName, "tcp" ) ;
   if ( !servinfo )
   {
      port = atoi ( pServiceName ) ;
   }
   else
   {
      port = (UINT16)ntohs(servinfo->s_port) ;
   }
   sockAddress.sin_port = htons ( port ) ;

   rawSocket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ;
   if ( -1 == rawSocket )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   rc = connect ( rawSocket, (struct sockaddr *) &sockAddress,
                    sizeof( sockAddress ) ) ;
   if ( rc )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   setKeepAlive( rawSocket, 1, OSS_SOCKET_KEEP_IDLE,
                 OSS_SOCKET_KEEP_INTERVAL,
                 OSS_SOCKET_KEEP_CONTER ) ;
   _disableNagle( rawSocket ) ;

   s = (Socket*) SDB_OSS_MALLOC ( sizeof( Socket ) ) ;
   if ( NULL == s )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   s->rawSocket = rawSocket ;
#ifdef SDB_SSL
   s->sslHandle = NULL ;
#endif
   s->isInterruptFunc = NULL ;

   if ( useSSL )
   {
#ifdef SDB_SSL
      rc = _clientSecure ( s ) ;
      if ( SDB_OK != rc )
      {
         goto error;
      }
      *sock = s ;
      goto done ;
#endif
      /* SSL feature not available in this build */
      rc = SDB_INVALIDARG ;
      goto error;
   }

   *sock = s ;
   rc = SDB_OK ;

done :
   return rc ;
error :
   if ( -1 != rawSocket )
   {
      _clientDisconnect ( rawSocket ) ;
      rawSocket = -1 ;
   }
   if ( NULL != s )
   {
      SDB_OSS_FREE ( s ) ;
   }
   goto done ;
}

#ifdef SDB_SSL

static INT32 _clientSecure( Socket* sock )
{
   SSLContext* ctx = NULL ;
   SSLHandle* sslHandle = NULL ;
   INT32 ret = SDB_OK ;

   if ( NULL == sock || -1 == sock->rawSocket )
   {
      ret = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL != sock->sslHandle )
   {
      /* sslHandle already exists */
      ret = SDB_NETWORK ;
      goto error ;
   }

   ctx = ossSSLGetContext () ;
   if ( NULL == ctx )
   {
      /* failed to get SSL context */
      ret = SDB_NETWORK ;
      goto error;
   }

   ret = ossSSLNewHandle ( &sslHandle, ctx, sock->rawSocket, NULL, 0 ) ;
   if ( SSL_OK != ret )
   {
      /* failed to create SSL handle */
      ret = SDB_NETWORK ;
      goto error ;
   }

   ret = ossSSLConnect ( sslHandle ) ;
   if ( SSL_OK != ret )
   {
      /* SSL failed to connect */
      ret = SDB_NETWORK ;
      goto error ;
   }

   /* create a SSL connection */
   sock->sslHandle = sslHandle ;
   ret = SDB_OK ;

done:
   return ret;
error:
   if ( NULL != sslHandle )
   {
      ossSSLFreeHandle ( &sslHandle ) ;
   }
   goto done;
}

#endif /* SDB_SSL */

SOCKET clientGetRawSocket( Socket* sock )
{
   SOCKET s = -1;

   if ( NULL == sock )
   {
      goto error ;
   }

   s = sock->rawSocket ;

done:
   return s ;
error:
   goto done ;
}

INT32 setKeepAlive( SOCKET sock, INT32 keepAlive, INT32 keepIdle,
                    INT32 keepInterval, INT32 keepCount )
{
   INT32 rc = SDB_OK ;
#if defined (_WINDOWS)
   struct tcp_keepalive alive_in ;
   DWORD ulBytesReturn       = 0 ;
#endif

   if ( 0 == sock )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   
#if defined (_WINDOWS)
   alive_in.onoff             = keepAlive ;
   alive_in.keepalivetime     = keepIdle * 1000 ; // ms
   alive_in.keepaliveinterval = keepInterval * 1000 ; // ms
   rc = setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE,
                    ( CHAR *)&keepAlive, sizeof(keepAlive) ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = WSAIoctl( sock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
                  NULL, 0, &ulBytesReturn, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
#else
   rc = setsockopt( sock, SOL_SOCKET, SO_KEEPALIVE,
                    ( void *)&keepAlive, sizeof(keepAlive) ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   #if defined (_AIX)
      #define SOL_TCP IPPROTO_TCP
   #endif
   rc = setsockopt( sock, SOL_TCP, TCP_KEEPIDLE,
                    ( void *)&keepIdle, sizeof(keepIdle) ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = setsockopt( sock, SOL_TCP, TCP_KEEPINTVL,
                    ( void *)&keepInterval, sizeof(keepInterval) ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = setsockopt( sock, SOL_TCP, TCP_KEEPCNT,
                    ( void *)&keepCount, sizeof(keepCount) ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
#endif

done:
   return rc ;
error:
   goto done ;
}

static INT32 _disableNagle( SOCKET sock )
{
   INT32 rc = SDB_OK ;
   INT32 temp = 1 ;
   rc = setsockopt ( sock, IPPROTO_TCP, TCP_NODELAY, (CHAR *) &temp,
                     sizeof ( INT32 ) ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 disableNagle( Socket* sock )
{
   INT32 rc = SDB_OK ;

   if ( !sock )
   {
      rc = SDB_INVALIDARG ;
   }
   else
   {
      rc = _disableNagle ( sock->rawSocket ) ;
   }

   return rc ;
}

static void _clientDisconnect ( SOCKET sock )
{
#if defined (_WINDOWS)
      closesocket ( sock ) ;
#else
      close ( sock ) ;
#endif
}

void clientDisconnect ( Socket** sock )
{
   if ( !sock )
   {
      goto done ;
   }

   if ( NULL != *sock )
   {
      _clientDisconnect ( (*sock)->rawSocket ) ;
#ifdef SDB_SSL
      if ( NULL != (*sock)->sslHandle )
      {
         ossSSLShutdown ( (*sock)->sslHandle ) ;
         ossSSLFreeHandle ( &( (*sock)->sslHandle ) ) ;
      }
#endif
      SDB_OSS_FREE ( *sock ) ;
      *sock = NULL ;
   }

done:
   return ;
}

INT32 clientSend ( Socket* sock, const CHAR *pMsg, INT32 len, 
                   INT32 *pSentLen, INT32 timeout )
{
   INT32 rc = SDB_OK ;
   UINT32 retries = 0 ;
   SOCKET rawSocket ;
   struct timeval maxSelectTime ;
   fd_set fds ;

   if ( !sock || !pSentLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !pMsg || !len )
   {
      goto done ;
   }
   *pSentLen = 0 ;
   rawSocket = sock->rawSocket ;
   if ( timeout >= 0 )
   {
      maxSelectTime.tv_sec = timeout / 1000000 ;
      maxSelectTime.tv_usec = timeout % 1000000 ;
   }
   else
   {
      maxSelectTime.tv_sec = 1000000 ;
      maxSelectTime.tv_usec = 0 ;
   }

   while ( TRUE )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( rawSocket, &fds ) ;
      rc = select ( rawSocket + 1, NULL, &fds, NULL,
                    timeout>=0?&maxSelectTime:NULL ) ;
      if ( 0 == rc )
      {
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      if ( 0 > rc )
      {
         rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
         if ( WSAEINTR == rc )
#else
         if ( EINTR == rc )
#endif
         {
            if ( NULL == sock->isInterruptFunc || !sock->isInterruptFunc() )
            {
               continue ;
            }
            else
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
         }
         rc = SDB_NETWORK ;
         goto error ;
      }

      if ( FD_ISSET ( rawSocket, &fds ) )
      {
         break ;
      }
   }
   while ( len > 0 )
   {
#ifdef SDB_SSL
      if ( NULL != sock->sslHandle )
      {
         rc = ossSSLWrite ( sock->sslHandle, pMsg, len ) ;
         if ( rc <= 0 )
         {
            if ( SSL_AGAIN == rc )
            {
               rc = SDB_TIMEOUT ;
            }
            else
            {
               /* SSL failed to send */
               rc = SDB_NETWORK ;
            }
            goto error;
         }
      }
      else
#endif /* SDB_SSL */
      {
#if defined (_WINDOWS)
         rc = send ( rawSocket, pMsg, len, 0 ) ;
         if ( SOCKET_ERROR == rc )
#else
         rc = send ( rawSocket, pMsg, len, MSG_NOSIGNAL ) ;
         if ( -1 == rc )
#endif
         {
            rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
            if ( WSAETIMEDOUT == rc)
#else
            if ( EAGAIN == rc || EWOULDBLOCK == rc || ETIMEDOUT == rc)
#endif
            {
               rc = SDB_TIMEOUT ;
               goto error ;
            }
#if defined ( _WINDOWS )
            if ( ( WSAEINTR == rc ) && ( retries < MAX_SEND_RETRIES ) )
#else
            if ( ( EINTR == rc ) && ( retries < MAX_SEND_RETRIES ) )
#endif
            {
               ++retries ;
               continue ;
            }
            rc = SDB_NETWORK ;
            goto error ;
         }
      }
      *pSentLen += rc ;
      len -= rc ;
      pMsg += rc ;
   }
   rc = SDB_OK ;
done :
   return rc ;
error :
   goto done ;
}

INT32 clientRecv ( Socket* sock, CHAR *pMsg, INT32 len, 
                   INT32 *pReceivedLen, INT32 timeout )
{
   INT32 rc = SDB_OK ;
   UINT32 retries = 0 ;
   SOCKET rawSocket ;
   struct timeval maxSelectTime ;
   fd_set fds ;

   if ( !sock || !pReceivedLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !pMsg || !len )
   {
      goto done ;
   }

   *pReceivedLen = 0 ;

#ifdef SDB_SSL
   if ( NULL != sock->sslHandle )
   {
      while ( len > 0 )
      {
         rc = ossSSLRead ( sock->sslHandle, pMsg, len ) ;
         if ( rc <= 0 )
         {
            if ( SSL_AGAIN == rc || SSL_TIMEOUT == rc)
            {
               rc = SDB_TIMEOUT ;
            }
            else
            {
               /* SSL failed to recv */
               rc = SDB_NETWORK ;
            }
            goto error;
         }
         *pReceivedLen += rc ;
         len -= rc ;
         pMsg += rc ;
         rc = SDB_OK;
      }

      rc = SDB_OK;
      goto done;
   }
#endif /* SDB_SSL */
   rawSocket = sock->rawSocket ;
   if ( timeout >= 0 )
   {
      maxSelectTime.tv_sec = timeout / 1000000 ;
      maxSelectTime.tv_usec = timeout % 1000000 ;
   }
   else
   {
      maxSelectTime.tv_sec = 1000000 ;
      maxSelectTime.tv_usec = 0 ;
   }
   while ( TRUE )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( rawSocket, &fds ) ;
      rc = select ( rawSocket + 1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL ) ;
      if ( 0 == rc )
      {
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      if ( 0 > rc )
      {
         rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
         if ( WSAEINTR == rc )
#else
         if ( EINTR == rc )
#endif 
         {
            if ( NULL == sock->isInterruptFunc || !sock->isInterruptFunc() )
            {
               continue ;
            }
            else
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
         }
         rc = SDB_NETWORK ;
         goto error ;
      }
      if ( FD_ISSET ( rawSocket, &fds ) )
      {
         break ;
      }
   }
   while ( len > 0 )
   {
#if defined (_WINDOWS)
      rc = recv ( rawSocket, pMsg, len, 0 ) ;
#else
      rc = recv ( rawSocket, pMsg, len, MSG_NOSIGNAL ) ;
#endif
      if ( rc > 0 )
      {
         *pReceivedLen += rc ;
         len -= rc ;
         pMsg += rc ;
      }
      else if ( rc == 0 )
      {
         rc = SDB_NETWORK_CLOSE ;
         goto error ;
      }
      else
      {
         rc = SOCKET_GETLASTERROR ;
#if defined (_WINDOWS)
         if ( WSAETIMEDOUT == rc )
#else
         if ( (EAGAIN == rc || EWOULDBLOCK == rc ) )
#endif
         {
            rc = SDB_TIMEOUT ;
            goto error ;
         }
#if defined ( _WINDOWS )
         if ( ( WSAEINTR == rc ) && ( retries < MAX_RECV_RETRIES ) )
#else
         if ( ( EINTR == rc ) && ( retries < MAX_RECV_RETRIES ) )
#endif
         {
            ++retries ;
            continue ;
         }
         rc = SDB_NETWORK ;
         goto error ;
      }
   }
   rc = SDB_OK ;
done :
   return rc ;
error :
   goto done ;
}

void clientSetInterruptFunc( Socket* sock, socketInterruptFunc func )
{
   if ( !sock )
   {
      return ;
   }

   sock->isInterruptFunc = func ;
}


