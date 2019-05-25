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
