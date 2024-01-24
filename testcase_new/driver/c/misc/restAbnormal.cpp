/*****************************************************************************
 * @Description : test case for rest
 *
 * @Modify List : Ting YU
 *                2016-09-06
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <string.h>
#include <client.h>
#include "testcommon.hpp"
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "arguments.hpp"

#define BUFLEN    1024
#define RESTPORT  11814 

INT32 checkServer()
{
   INT32 rc = SDB_OK ; 
   sdbConnectionHandle db ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   CHECK_RC( SDB_OK, rc, "fail to connect sdb" ) ;
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 initClient()
{
   INT32 rc ;
   INT32 sockfd ;

   // create socket  
   sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ;    

   // get addr
   CHAR *hostip = "127.0.0.1" ;
   struct sockaddr_in addr ;
   memset( &addr, 0, sizeof(addr) ) ;
   addr.sin_family = AF_INET ;
   addr.sin_port = htons( RESTPORT ) ;
   addr.sin_addr.s_addr = inet_addr( hostip ) ;

   // connect     
   rc = connect( sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr) ) ;   
   if( rc == -1 )
   {
      perror( "connect error" ) ;
      return rc ;
   }

   return sockfd ;
}

TEST( restAbnormal, multiSend )
{   
   INT32 rc ;
   INT32 sockfd = initClient() ;
   if( sockfd == -1 ) return ; 

   // send message
   CHAR restStr[8] ;
   sprintf( restStr, "%d", RESTPORT ) ;
   const CHAR* hostip = "127.0.0.1" ;

   CHAR sendbuf[ BUFLEN ] ;
   memset( sendbuf, 0, sizeof(sendbuf) ) ;
   strcpy( sendbuf, "POST / HTTP/1.1\r\n" ) ; 
   strcat( sendbuf, "Content-Type: application/x-www-form-urlencoded;CHARset=UTF-8\r\n" ) ;
   strcat( sendbuf, "Content-Length: 22\r\n" ) ;
   strcat( sendbuf, "Host: " ) ;
   strcat( sendbuf, hostip ) ;
   strcat( sendbuf, ":" ) ;
   strcat( sendbuf, restStr ) ;
   strcat( sendbuf, "\r\n" ) ;
   strcat( sendbuf, "\r\ncmd=list" ) ;

   rc = send( sockfd, sendbuf, strlen(sendbuf), 0 ) ;
   ASSERT_GE( rc, 0 ) << "first send error" ;

   CHAR sendbuf2[ BUFLEN ] ; 
   memset( sendbuf2, 0, sizeof(sendbuf) ) ;
   strcat( sendbuf2, "%20collections" ) ;

   rc = send( sockfd, sendbuf2, strlen(sendbuf2), 0 ) ;
   ASSERT_GE( rc, 0 ) << "second send error" ;

   // recv and check returned errno 
   CHAR recvbuf[ BUFLEN ] ;
   CHAR* totalrecvbuf = (CHAR*)malloc( sizeof(CHAR) * BUFLEN );
   if( !totalrecvbuf )
   {
      printf( "Error malloc space for totalrecvbuf.\n" ) ;
      return ;
   }
   INT32 totalSize = BUFLEN ;
   INT32 availSize = BUFLEN ;
   INT32 pos = 0 ;
   while( ( rc = recv( sockfd, recvbuf, BUFLEN, 0 ) ) > 0 )
   {
      if( rc > availSize )
      {
         CHAR* tmp = (CHAR*)realloc( totalrecvbuf, sizeof(CHAR) * (totalSize + BUFLEN) ) ;
         if( !tmp )
         {
            printf( "Error realloc space for totalrecvbuf.\n" ) ;
            break ;
         }
         totalrecvbuf = tmp ;
         totalSize += BUFLEN ;
         availSize += BUFLEN ;
      }
      memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
      availSize -= rc ;
      pos += rc ;
   }
   printf( "%s\n", totalrecvbuf ) ;
   CHAR* p = strstr( totalrecvbuf, "\r\n\r\n{ \"errno\": 0 }" ) ;
   if ( p == NULL )
   {
      p = strstr( totalrecvbuf, "\r\n{ \"errno\": 0 }" ) ;
   }
   ASSERT_STRNE( NULL, p ) << "check recieve message error";
   free( totalrecvbuf ) ;
   close( sockfd ) ; 

   rc = checkServer() ;     
   ASSERT_EQ( SDB_OK, rc ) << "fail to checkServer" ; 
}

TEST( restAbnormal, postLackTerminator )
{   
   INT32 rc ;
   INT32 sockfd = initClient() ;
   if( sockfd == -1 ) return ; 

   // send message
   CHAR restStr[8];
   sprintf( restStr, "%d", RESTPORT ) ;
   const CHAR* hostip = "127.0.0.1" ;
   CHAR sendbuf[ BUFLEN ] ;   
   memset( sendbuf, 0, sizeof(sendbuf) ) ;
   strcpy( sendbuf, "POST / HTTP/1.1\r\n" ) ; 
   strcat( sendbuf, "Content-Type: application/x-www-form-urlencoded;CHARset=UTF-8\r\n" ) ;
   strcat( sendbuf, "Content-Length: 28\r\n" ) ; //length is 22 
   strcat( sendbuf, "Host: " ) ;
   strcat( sendbuf, hostip ) ;
   strcat( sendbuf, ":" ) ;
   strcat( sendbuf, restStr ) ;
   strcat( sendbuf, "\r\n" ) ;
   strcat( sendbuf, "\r\ncmd=list%20collections" ) ;
   rc = send( sockfd, sendbuf, strlen(sendbuf), 0 ) ;
   ASSERT_GE( rc, 0 ) << "send error" ;

   // recv 
   CHAR recvbuf[ BUFLEN ] ;
   CHAR* totalrecvbuf = (CHAR*)malloc( sizeof(CHAR) * BUFLEN );
   if( !totalrecvbuf )
   {
      printf( "Error malloc space for totalrecvbuf.\n" ) ;
      return ;
   }
   INT32 totalSize = BUFLEN ;
   INT32 availSize = BUFLEN ;
   INT32 pos = 0;
   while( ( rc = recv( sockfd, recvbuf, BUFLEN, 0 ) ) > 0 )
   {
      if( rc > availSize )
      {
         CHAR* tmp = (CHAR*)realloc( totalrecvbuf, sizeof(CHAR) * (totalSize + BUFLEN) ) ;
         if( !tmp )
         {
            printf( "Error realloc space for totalrecvbuf.\n" ) ;
            break ;
         }
         totalrecvbuf = tmp ;
         totalSize += BUFLEN ;
         availSize += BUFLEN ;
      }
      memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
      availSize -= rc ;
      pos += rc ;
   }
   printf( "%s\n", totalrecvbuf ) ;
   free( totalrecvbuf ) ;
   close( sockfd ) ;    

   rc = checkServer() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to checkServer" ;
}

TEST( restAbnormal, getLackTerminator )
{   
   INT32 rc ;
   INT32 sockfd = initClient() ;
   if( sockfd == -1 ) return ; 

   // send message
   CHAR restStr[8];
   sprintf( restStr, "%d", RESTPORT ) ;
   const CHAR* hostip = "127.0.0.1" ;
   CHAR sendbuf[ BUFLEN ] ;
   memset( sendbuf, 0, sizeof(sendbuf) ) ;
   strcpy( sendbuf, "GET /?cmd=list%20collections HTTP/1.1\r\n" ) ;
   strcat( sendbuf, "Host: " ) ;
   strcat( sendbuf, hostip ) ;
   strcat( sendbuf, ":" ) ;
   strcat( sendbuf, restStr ) ;
   strcat( sendbuf, "\r\n" ) ; // lack of end "\r\n\r\n"
   rc = send( sockfd, sendbuf, strlen(sendbuf), 0 ) ;
   ASSERT_GE( rc, 0 ) << "send error" ;

   // recv 
   CHAR recvbuf[ BUFLEN ] ;
   CHAR* totalrecvbuf = (CHAR*)malloc( sizeof(CHAR) * BUFLEN ) ;
   if( !totalrecvbuf )
   {
      printf( "Error malloc space for totalrecvbuf.\n" ) ;
      return ;
   }
   INT32 totalSize = BUFLEN ;
   INT32 availSize = BUFLEN ;
   INT32 pos = 0;
   while( ( rc = recv( sockfd, recvbuf, BUFLEN, 0 ) ) > 0 )
   {
      if( rc > availSize )
      {
         CHAR* tmp = (CHAR*)realloc( totalrecvbuf, sizeof(CHAR) * (totalSize + BUFLEN) ) ;
         if( !tmp )
         {
            printf( "Error realloc space for totalrecvbuf.\n" ) ;
            break ;
         }
         totalrecvbuf = tmp ;
         totalSize += BUFLEN ;
         availSize += BUFLEN ;
      }
      memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
      availSize -= rc ;
      pos += rc ;
   }
   printf( "%s\n", totalrecvbuf ) ;
   free( totalrecvbuf ) ;
   close( sockfd ) ;    

   rc = checkServer() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to checkServer" ;      
}

TEST( restAbnormal, formatNotMatchProtocol1 )
{   
   INT32 rc ;
   INT32 sockfd = initClient() ;
   if( sockfd == -1 ) return ;

   // send message   
   CHAR sendbuf[ BUFLEN ] ;
   memset( sendbuf, 0, sizeof(sendbuf) ) ;
   strcpy( sendbuf, "\r\n\r\n" ) ;  
   rc = send( sockfd, sendbuf, strlen(sendbuf), 0 ) ;
   ASSERT_GE( rc, 0 ) << "send error";

   // recv  
   CHAR recvbuf[ BUFLEN ] ;
   CHAR* totalrecvbuf = (CHAR*)malloc( sizeof(CHAR) * BUFLEN ) ;
   if( !totalrecvbuf )
   {
      printf( "Error malloc space for totalrecvbuf.\n" ) ;
      return ;
   }
   INT32 totalSize = BUFLEN ;
   INT32 availSize = BUFLEN ;
   INT32 pos = 0 ;
   while( ( rc = recv( sockfd, recvbuf, BUFLEN, 0 ) ) > 0 )
   {
      if( rc > availSize )
      {
         CHAR* tmp = (CHAR*)realloc( totalrecvbuf, sizeof(CHAR) * (totalSize + BUFLEN) ) ;
         if( !tmp )
         {
            printf( "Error realloc space for totalrecvbuf.\n" ) ;
            break ;
         }
         totalrecvbuf = tmp ;
         totalSize += BUFLEN ;
         availSize += BUFLEN ;
      }
      memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
      availSize -= rc ;
      pos += rc ;
   }
   printf( "%s\n", totalrecvbuf ) ;
   free( totalrecvbuf ) ;
   close( sockfd ) ;    

   rc = checkServer() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to checkServer" ;
}

TEST( restAbnormal, formatNotMatchProtocol2 )
{   
   INT32 rc ;
   INT32 sockfd = initClient() ; 
   if( sockfd == -1 ) return ;

   // send message   
   CHAR sendbuf[ BUFLEN ] ;
   memset( sendbuf, 0, sizeof(sendbuf) ) ;
   strcpy( sendbuf, "GE123gaetyt4a4tyawy49" ) ;  
   rc = send( sockfd, sendbuf, strlen(sendbuf), 0 ) ;
   ASSERT_GE( rc, 0 ) << "send error" ;

   // recv
   CHAR recvbuf[ BUFLEN ] ;
   CHAR* totalrecvbuf = (CHAR*)malloc( sizeof(CHAR) * BUFLEN ) ;
   if( !totalrecvbuf )
   {
      printf( "Error malloc space for totalrecvbuf.\n" ) ;
      return ;
   }
   INT32 totalSize = BUFLEN ;
   INT32 availSize = BUFLEN ;
   INT32 pos = 0;
   while( ( rc = recv( sockfd, recvbuf, BUFLEN, 0 ) ) > 0 )
   {
      if( rc > availSize )
      {
         CHAR* tmp = (CHAR*)realloc( totalrecvbuf, sizeof(CHAR) * (totalSize + BUFLEN) ) ;
         if( !tmp )
         {
            printf( "Error realloc space for totalrecvbuf.\n" ) ;
            break ;
         }
         totalrecvbuf = tmp ;
         totalSize += BUFLEN ;
         availSize += BUFLEN ;
      }
      memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
      availSize -= rc ;
      pos += rc ;
   }
   printf( "%s\n", totalrecvbuf ) ;
   free( totalrecvbuf ) ;
   close( sockfd ) ;

   rc = checkServer() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to checkServer" ;      
}
