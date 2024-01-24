/******************************************************************************/
/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "base64.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

namespace engine
{
   namespace base64
   {
      Alphabet alphabet ;

      // PD_TRACE_DECLARE_FUNCTION ( SDB_BASE64_ENCODE, "encode" )
      void encode( stringstream& ss, const char* data, int size )
      {
         // PD_TRACE_ENTRY ( SDB_BASE64_ENCODE )
         for ( int i = 0 ; i < size ; i += 3 )
         {
            int left = size - i ;
            const unsigned char * start = (const unsigned char*)data + i ;

            // byte 0
            ss << alphabet.e(start[0]>>2) ;

            // byte 1
            unsigned char temp = ( start[0] << 4 ) ;
            if ( left == 1 )
            {
               ss << alphabet.e(temp) ;
               break ;
            }
            temp |= ( ( start[1] >> 4 ) & 0xF ) ;
            ss << alphabet.e(temp) ;

            // byte 2
            temp = ( start[1] & 0xF ) << 2 ;
            if ( left == 2 )
            {
               ss << alphabet.e(temp) ;
               break ;
            }
            temp |= ( ( start[2] >> 6 ) & 0x3 ) ;
            ss << alphabet.e(temp) ;

            // byte 3
            ss << alphabet.e(start[2] & 0x3f) ;
         }

         int mod = size % 3 ;
         if ( mod == 1 )
         {
            ss << "==" ;
         }
         else if ( mod == 2 )
         {
            ss << "=" ;
         }
         // PD_TRACE_EXIT ( SDB_BASE64_ENCODE )
      }

      string encode( const char* data, int size )
      {
         stringstream ss ;
         encode( ss, data, size ) ;
         return ss.str() ;
      }

      string encode( const string& s )
      {
         return encode( s.c_str(), s.size() ) ;
      }

      // PD_TRACE_DECLARE_FUNCTION ( SDB_BASE64_DECODE, "decode" )
      void decode( stringstream& ss, const string& s )
      {
         SDB_ASSERT( s.size() % 4 == 0, "invalid base64" ) ;
         // PD_TRACE_ENTRY ( SDB_BASE64_DECODE )
         const unsigned char * data = (const unsigned char*)s.c_str() ;
         int size = s.size() ;

         unsigned char buf[3] ;
         for ( int i = 0 ; i < size ; i += 4 )
         {
            const unsigned char * start = data + i ;
            buf[0] = ( ( alphabet.decode[start[0]] << 2 ) & 0xFC ) |
                     ( ( alphabet.decode[start[1]] >> 4 ) & 0x3 ) ;
            buf[1] = ( ( alphabet.decode[start[1]] << 4 ) & 0xF0 ) |
                     ( ( alphabet.decode[start[2]] >> 2 ) & 0xF ) ;
            buf[2] = ( ( alphabet.decode[start[2]] << 6 ) & 0xC0 ) |
                     ( ( alphabet.decode[start[3]] & 0x3F ) ) ;

            int len = 3 ;
            if ( start[3] == '=' )
            {
               len = 2 ;
               if ( start[2] == '=' )
               {
                  len = 1 ;
               }
            }
            ss.write( (const char*)buf , len ) ;
         }
         // PD_TRACE_EXIT ( SDB_BASE64_DECODE )
      }

      string decode( const string& s )
      {
         stringstream ss ;
         decode( ss , s ) ;
         return ss.str() ;
      }

      // PD_TRACE_DECLARE_FUNCTION ( SDB_BASE64_ENCODEEX, "encodeEx" )
      void encodeEx( ossPoolStringStream& ss, const char* data, int size,
                     const Alphabet* pAlphabet )
      {
         // PD_TRACE_ENTRY ( SDB_BASE64_ENCODEEX )
         for ( int i = 0 ; i < size ; i += 3 )
         {
            int left = size - i ;
            const unsigned char * start = (const unsigned char*)data + i ;

            // byte 0
            ss << pAlphabet->e(start[0]>>2) ;

            // byte 1
            unsigned char temp = ( start[0] << 4 ) ;
            if ( left == 1 )
            {
               ss << pAlphabet->e(temp) ;
               break ;
            }
            temp |= ( ( start[1] >> 4 ) & 0xF ) ;
            ss << pAlphabet->e(temp) ;

            // byte 2
            temp = ( start[1] & 0xF ) << 2 ;
            if ( left == 2 )
            {
               ss << pAlphabet->e(temp) ;
               break ;
            }
            temp |= ( ( start[2] >> 6 ) & 0x3 ) ;
            ss << pAlphabet->e(temp) ;

            // byte 3
            ss << pAlphabet->e(start[2] & 0x3f) ;
         }

         int mod = size % 3 ;
         if ( mod == 1 )
         {
            ss << "==" ;
         }
         else if ( mod == 2 )
         {
            ss << "=" ;
         }
         // PD_TRACE_EXIT ( SDB_BASE64_ENCODEEX )
      }

      ossPoolString encodeEx( const char* data, int size,
                              const Alphabet* pAlphabet )
      {
         ossPoolStringStream ss ;
         encodeEx( ss, data, size, pAlphabet ) ;
         return ss.str() ;
      }

      ossPoolString encodeEx( const ossPoolString& s,
                              const Alphabet* pAlphabet )
      {
         return encodeEx( s.c_str(), s.size(), pAlphabet ) ;
      }

      // PD_TRACE_DECLARE_FUNCTION ( SDB_BASE64_DECODEEX, "decodeEx" )
      void decodeEx( ossPoolStringStream& ss, const char* data, int size,
                     const Alphabet* pAlphabet )
      {
         // PD_TRACE_ENTRY ( SDB_BASE64_DECODEEX )
         if ( size % 4 != 0 )
         {
            size -= ( size % 4 ) ;
         }

         unsigned char buf[3] ;
         for ( int i = 0 ; i < size ; i += 4 )
         {
            const unsigned char * start = (const unsigned char*)data + i ;
            buf[0] = ( ( pAlphabet->decode[start[0]] << 2 ) & 0xFC ) |
                     ( ( pAlphabet->decode[start[1]] >> 4 ) & 0x3 ) ;
            buf[1] = ( ( pAlphabet->decode[start[1]] << 4 ) & 0xF0 ) |
                     ( ( pAlphabet->decode[start[2]] >> 2 ) & 0xF ) ;
            buf[2] = ( ( pAlphabet->decode[start[2]] << 6 ) & 0xC0 ) |
                     ( ( pAlphabet->decode[start[3]] & 0x3F ) ) ;

            int len = 3 ;
            if ( start[3] == '=' )
            {
               len = 2 ;
               if ( start[2] == '=' )
               {
                  len = 1 ;
               }
            }
            ss.write( (const char*)buf , len ) ;
         }
         // PD_TRACE_EXIT ( SDB_BASE64_DECODEEX )
      }

      ossPoolString decodeEx( const char* data, int size,
                              const Alphabet* pAlphabet )
      {
         ossPoolStringStream ss ;
         decodeEx( ss, data, size, pAlphabet ) ;
         return ss.str() ;
      }

      ossPoolString decodeEx( const ossPoolString& s,
                              const Alphabet* pAlphabet )
      {
         ossPoolStringStream ss ;
         decodeEx( ss , s.c_str(), s.size(), pAlphabet ) ;
         return ss.str() ;
      }

   }
}

