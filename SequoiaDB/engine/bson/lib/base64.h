/** \file base64.h
    \brief Encode binary data using printable characters.
*/

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

#pragma once

//#include <boost/scoped_array.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <string.h>
/** \namespace base64
    \brief Include files to encode binary data.
*/
namespace base64 {

    class Alphabet {
    public:
        Alphabet()
            : encode((unsigned char*)
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                     "abcdefghijklmnopqrstuvwxyz"
                     "0123456789"
                     "+/")
            //, decode(new unsigned char[257]) {
            {
            decode = new unsigned char [ 257 ] ;
            memset ( decode, 0, 256 ) ;
            //memset( decode.get() , 0 , 256 );
            for ( int i=0; i<64; i++ ) {
                decode[ encode[i] ] = i;
            }

            test();
        }
        void test() {
            //assert( strlen( (char*)encode ) == 64 );
            //for ( int i=0; i<26; i++ )
            //    assert( encode[i] == toupper( encode[i+26] ) );
        }

        char e( int x ) {
            return encode[x&0x3f];
        }
        ~Alphabet ()
        {
           if ( decode )
              delete [] decode ;
        }

    private:
        const unsigned char * encode;
    public:
        unsigned char *decode ;
        //boost::scoped_array<unsigned char> decode;
    };

    extern Alphabet alphabet;

/** \fn void encode( std::stringstream& ss , const char * data , int size )
    \brief C-style string convert to base64
    \param [in] data Input data
    \param [in] size The length of the input stringr
    \param [out] ss Output result
*/
    void encode( std::stringstream& ss , const char * data , int size );
    std::string encode( const char * data , int size );
    std::string encode( const std::string& s );
	
/** \fn void decode( std::stringstream& ss , const std::string& s ) 
    \brief Base64 convert to string
    \param [in] s The base64 string
    \param [out] ss Output result
*/
    void decode( std::stringstream& ss , const std::string& s );
    std::string decode( const std::string& s );

    void testAlphabet();
}
