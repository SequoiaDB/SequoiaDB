/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "ossMem.h"
#include "ossUtil.h"
#include "core.hpp"
#include "ossSocket.hpp"


#define REST_ADAPTOR_RECV_BUFF_SIZE 2048

INT32 receive ( ossSocket &sock, CHAR *buffer,
                INT32 &len, INT32 timeout )
{
   INT32 rc = SDB_OK ;
   CHAR *readBuf = buffer ;
   INT32 tempLen = 0 ;
   INT32 bufferLen = len ;
   len = 0 ;

   while ( len < bufferLen )
   {
      tempLen = (len + REST_ADAPTOR_RECV_BUFF_SIZE) > bufferLen ?
                bufferLen - len : REST_ADAPTOR_RECV_BUFF_SIZE ;
      rc = sock.recv ( buffer + len, tempLen, timeout, 0, FALSE ) ;
      if ( rc )
      {
         pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
                 "Failed to call ossSocket recv" ) ;
         goto error ;
      }
      if ( ossStrstr ( readBuf + len - (len?6:0),
                       "\r\n0\r\n\r\n" ) )
      {
         goto done ;
      }
      len += tempLen ;
   }

done :
   return rc ;
error :
   goto done ;
}

BOOLEAN restTest ( )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;
   CHAR buffer[4096] = {0} ;
   CHAR hostName[128] = {0} ;
   UINT32 port = 50004 ;
   INT32 timeout = 0 ;
   INT32 sentLen = 0 ;
   ossSnprintf ( hostName, 128,
                 "192.168.20.105" ) ;
   ossSocket sock( hostName, port, timeout ) ;
   rc = sock.initSocket() ;
   if ( rc )
   {
      printf ( "error init socket\n" ) ;
      return FALSE ;
   }
   rc = sock.setSocketLi(1,0) ;
   if ( rc )
   {
      printf ( "error set sockopt" ) ;
      return FALSE ;
   }
   rc = sock.connect() ;
   if ( rc )
   {
      printf ( "error connect %d\n", rc ) ;
      return FALSE ;
   }
   len = ossSnprintf ( buffer, 4096,
                       "POST / HTTP/1.1\r\n\
Host: 192.168.20.105:5004\r\n\
\r\n" ) ;
   rc = sock.send ( buffer, len, sentLen ) ;
   if ( rc )
   {
      printf ( "error send\n" ) ;
      return FALSE ;
   }
   len = 4096 ;
   rc = receive ( sock, buffer, len, 10000 ) ;
   if ( rc )
   {
      printf ( "error receive\n" ) ;
      return FALSE ;
   }
   return TRUE ;
}

INT32 main ( INT32 args, CHAR *argv[] )
{
   INT32 k = 0 ;
   for ( INT32 i = 0; i < 100000; ++i )
   {
      if ( restTest() )
      {
         ++k ;
      }
      printf ( "loop %d  success %d\n", i, k ) ;
   }
   printf ( "success %d\n", k ) ;
   return 0 ;
}
