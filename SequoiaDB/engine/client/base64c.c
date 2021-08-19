/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include "base64c.h"

static char B64[64] =
{
   'A','B','C','D','E','F','G','H',
   'I','J','K','L','M','N','O','P',
   'Q','R','S','T','U','V','W','X',
   'Y','Z',
   'a','b','c','d','e','f','g','h',
   'i','j','k','l','m','n','o','p',
   'q','r','s','t','u','v','w','x',
   'y','z',
   '0','1','2','3','4','5','6','7',
   '8','9','+','/'
} ;

int getEnBase64Size ( int size )
{
   int len = size ;
   int zeroize = len % 3 ;
   if( size == 0 )
   {
      return 0 ;
   }
   len = ( len + ( zeroize ? 3 - zeroize : 0 ) ) / 3 * 4 + 1 ;
   return len ;
}

//return string length + 1
int getDeBase64Size ( const char *s )
{

   int len = strlen ( s ) ;
   int zeroize = 0 ;
   if( !s )
   {
      return -1 ;
   }
   if( len == 0 )
   {
      return 0 ;
   }
   else if( len % 4 > 0 )
   {
      return -1 ;
   }
   if ( '=' == s [ len - 2 ] )
      zeroize = 2 ;
   else if ( '=' == s [ len - 1 ] )
      zeroize = 1 ;
   len = len / 4 * 3 - zeroize + 1 ;
   return len ;
}

int base64Encode ( const char *s, int in_size, char *_ret, int out_size )
{
   char c = 0x00 ;
   char t = 0x00 ;
   int vLen = 0 ;
   int len = in_size ;
   if( !s || !_ret )
   {
      return -1 ;
   }
   /* empty string */
   if( len == 0 )
   {
      return 0 ;
   }
   if ( out_size < getEnBase64Size ( in_size ) )
   {
      return -1 ;
   }
   while ( len > 0 )
   {
      c = ( s [ 0 ] >> 2 ) ;
      c = c & 0x3F;
      *_ret++ = B64[ (int)c ] ;
      if ( len > 2 )
      {
         c = ( (unsigned char)s[0] & 3 ) << 4 ;
         t = ( (unsigned char)s[1] >> 4 ) & 0x0F ;
         *_ret++ = B64 [ ( c ) | ( t ) ] ;
         c = ( (unsigned char)s[1] & 0xF ) << 2 ;
         t = ( (unsigned char)s[2] >> 6 ) & 0x3 ;
         *_ret++ = B64 [ ( c ) | ( t ) ] ;
         c = ( (unsigned char)s[2] & 0x3F ) ;
         *_ret++ = B64 [ (int)c ] ;
      }
      else
      {
         switch ( len )
         {
         case 1:
            *_ret++ = B64[ ( (unsigned char)s[0] & 3 ) << 4 ] ;
            *_ret++ = '=' ;
            *_ret++ = '=' ;
            break ;
         case 2:
            *_ret++ = B64[ ( ( (unsigned char)s[0] & 3 ) << 4 ) |
                             ( (unsigned char)s[1] >> 4 ) ] ;
            *_ret++ = B64[ ( ( (unsigned char)s[1] & 0x0F ) << 2 ) ] ;
            *_ret++ = '=' ;
            break ;
         }
      }
      s += 3 ;
      len -= 3 ;
      vLen += 4 ;
   }
   *_ret = 0 ;
   return vLen ;
}

char getCharIndex ( char c )
{
   if ( ( c >= 'A' ) && ( c <= 'Z' ) )
   {
      return c - 'A' ;
   }
   else if ( ( c >= 'a' ) && ( c <= 'z' ) )
   {
      return c - 'a' + 26 ;
   }
   else if ( ( c >= '0' ) && ( c <= '9' ) )
   {
      return c - '0' + 52 ;
   }
   else if ( c == '+' )
   {
      return 62 ;
   }
   else if ( c == '/' )
   {
      return 63 ;
   }
   else if ( c == '=' )
   {
      return 0 ;
   }
   return 0 ;
}

int base64Decode ( const char *s, char *_ret, int out_size )
{
   static char lpCode [ 4 ] ;
   int vLen = 0 ;
   int len = strlen ( s ) ;
   if( !s || !_ret )
   {
      return -1 ;
   }
   /* empty base64 */
   if( len == 0 )
   {
      return 0 ;
   }
   /* base64 must be 4 bytes aligned */
   if( ( len % 4 ) ||
       ( out_size < getDeBase64Size ( s ) ) )
   {
      return -1 ;
   }
   while( len >= 4 )
   {
      lpCode [ 0 ] = getCharIndex ( s [ 0 ] ) ;
      lpCode [ 1 ] = getCharIndex ( s [ 1 ] ) ;
      lpCode [ 2 ] = getCharIndex ( s [ 2 ] ) ;
      lpCode [ 3 ] = getCharIndex ( s [ 3 ] ) ;

      if( out_size <= 1 )
         return vLen ;
      *_ret++ = ( lpCode [ 0 ] << 2 ) | ( lpCode [ 1 ] >> 4 ) ;
      --out_size ;
      if( out_size <= 1 )
         return vLen ;
      *_ret++ = ( lpCode [ 1 ] << 4 ) | ( lpCode [ 2 ] >> 2 ) ;
      --out_size ;
      if( out_size <= 1 )
         return vLen ;
      *_ret++ = ( lpCode [ 2 ] << 6 ) | ( lpCode [ 3 ] ) ;
      --out_size ;

      s += 4 ;
      len -= 4 ;
      vLen += 3 ;
   }
   return vLen ;
}
