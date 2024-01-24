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

   Source File Name = urldecode.c

   Descriptive Name = url decode

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component. This file contains declare of json2rawbson. Note
   this function should NEVER be directly called other than fromjson.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/20/2014  JWH Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.h"
#include "oss.h"
#include "ossUtil.h"
#include "ossMem.h"
#include "msg.h"
#include "url.h"

INT16 hexChar2dec( CHAR c )
{
   if( '0' <= c && c <= '9' )
   {
      return (INT16)( c - '0' ) ;
   }
   else if ( 'a' <= c && c <= 'f' )
   {
      return (INT16)( ( c - 'a' ) + 10 ) ;
   }
   else if( 'A' <= c && c <= 'F' )
   {
      return (INT16)( ( c - 'A' ) + 10 ) ;
   }
   return -1 ;
}

INT32 urlDecodeSize( const CHAR *pBuffer, INT32 bufferSize )
{
   INT32 decodeSize = 0 ;
   CHAR result = 0 ;
   INT32 i = 0 ;
   for( ; i < bufferSize; ++i )
   {
      result = pBuffer[i] ;
      if( '%' == result )
      {
         if ( i + 2 >= bufferSize )
         {
            break ;
         }
         i += 2 ;
         ++decodeSize ;
      }
      else
      {
         ++decodeSize ;
      }
   }
   return decodeSize ;
}

void urlDecode( const CHAR *pBuffer, INT32 bufferSize,
                CHAR **pOut, INT32 outSize )
{
   CHAR *pTemp = *pOut ;
   CHAR result = 0 ;
   CHAR c1 = 0 ;
   CHAR c2 = 0 ;
   INT32 c3 = 0 ;
   INT32 i = 0 ;
   for( ; i < bufferSize && outSize > 0; ++i )
   {
      result = pBuffer[i] ;
      if( '+' == result )
      {
         *pTemp = ' ' ;
         --outSize ;
      }
      else if( '%' == result )
      {
         if ( i + 2 >= bufferSize )
         {
            break ;
         }
         ++i ;
         c1 = pBuffer[i] ;
         ++i ;
         c2 = pBuffer[i] ;
         if( c1 == -1 || c2 == -1 )
         {
            continue ;
         }
         c3 = hexChar2dec( c1 ) * 16 + hexChar2dec( c2 ) ;
         result = (CHAR)c3 ;
         *pTemp = result ;
         --outSize ;
      }
      else
      {
         *pTemp = result ;
         --outSize ;
      }
      ++pTemp ;
   }
}
