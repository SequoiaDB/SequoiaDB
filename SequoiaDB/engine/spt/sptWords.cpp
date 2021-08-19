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

   Source File Name = sptWords.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#if defined (_WINDOWS )
#include <windows.h>
#endif

#include "sptWords.hpp"
#include "ossMem.h"
#include "ossUtil.h"
#include "string.h"
#include <boost/algorithm/string.hpp>

#define PERIOD_UTF8_CN     "\u3002"
#define COMMA_UTF8_CN      "\uFF0C"
#define PERIOD_ASCII_EN    "."
#define COMMA_ASCII_EN     ","
#define BACKSLASH          "\\"
#define TABLE_LINE_LEN     24

namespace engine {

#if defined ( _WINDOWS )

   // both GBK and UTF8 are multi-byte coding schemes,
   // we can use the function to convert one to another.
   INT32 _sptConvertMultiByte( UINT32 sourceCodePage, 
                               UINT32 targetCodePage,
                               const std::string& sourceStr, 
                               std::string &targetStr )
   {
      INT32 rc = SDB_OK ;
      INT32 requiredSize = 0 ;
      INT32 strSize = 0 ;
      CHAR *chars = NULL ;
      WCHAR *wchars = NULL ;

      // multi byte(GBK/UTF-8) to wide chars(Unicode)
      requiredSize = MultiByteToWideChar( sourceCodePage, 0, sourceStr.c_str(), 
                                          -1, NULL, 0 ) ;
      wchars = (WCHAR *)SDB_OSS_MALLOC( requiredSize * sizeof(WCHAR) ) ;
      if ( !wchars )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( wchars, 0, requiredSize * sizeof(WCHAR) ) ;
      MultiByteToWideChar( sourceCodePage, 0, sourceStr.c_str(), 
                           -1, wchars, requiredSize ) ;
      // wide chars(Unicode) to multi byte(UTF-8/GBK)
      requiredSize = WideCharToMultiByte( targetCodePage, 0, wchars, 
                                          -1, NULL, 0, NULL, NULL) ;
      chars = (CHAR *)SDB_OSS_MALLOC( requiredSize ) ;
      if ( !chars )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( chars, 0, requiredSize ) ;
      WideCharToMultiByte( targetCodePage, 0, wchars, -1, 
                           chars, requiredSize, NULL, NULL ) ;
      // return
      targetStr = chars ;
      
   done:
      SAFE_OSS_FREE( chars ) ;
      SAFE_OSS_FREE( wchars ) ;
      return rc ;
   error:
      goto done ;
   }

   // transform GBK to UTF-8
   INT32 sptGBKToUTF8( const std::string& strGBK, std::string &strUTF8 )
   {
      return _sptConvertMultiByte( CP_ACP, CP_UTF8, strGBK, strUTF8 ) ;
   }

   // transform UTF-8 to GBK
   INT32 sptUTF8ToGBK( const std::string &strUTF8, std::string &strGBK )
   {
      return _sptConvertMultiByte( CP_UTF8, CP_ACP, strUTF8, strGBK ) ;
   }
#endif

   void _sptUTF8ToCharSet(const string &input, vector<string> &output)
   {
      string c ;
      INT32 i = 0, len = 0 ;
      for (; i != (INT32)input.length(); i += len )
      {
         UINT8 byte = (UINT8)input[i] ;
         if (byte >= 0xFC) // lenght 6
            len = 6 ;
         else if (byte >= 0xF8)
            len = 5 ;
         else if (byte >= 0xF0)
            len = 4 ;
         else if (byte >= 0xE0)
            len = 3 ;
         else if (byte >= 0xC0)
            len = 2 ;
         else
            len = 1 ;
         c = input.substr( i, len ) ;
         output.push_back( c ) ;
      }
   }

   void _sptChar2Word( const string &text, vector<string> &output )
   {
       INT32 word_len = 0 ;
       vector<string> vec_chars ;
       vector<string>::iterator it ;
       string word ;
       string digit ;
       
       _sptUTF8ToCharSet( text, vec_chars ) ;
       for( it = vec_chars.begin(); it != vec_chars.end(); it++ )
       {
           word_len = it->length() ;
           if ( word_len == 1 )
           {
               if ( (*it >= "a" && *it <= "z") ||
                    (*it >= "A" && *it <= "Z") )
               {
                   if ( !digit.empty() )
                   {
                       output.push_back(digit) ;
                       digit.clear() ;
                   }
                   word += *it ;
               }
               else if ( *it == "+" || *it == "-" || ( *it >= "0" && *it <= "9" ) )
               {
                   if ( !word.empty() )
                   {
                       output.push_back( word ) ;
                       word.clear() ;
                   }
                   digit += *it ;
               }
               else
               {
                   // save the word and digits which had been cached first
                   if ( !word.empty() )
                   {
                       output.push_back( word ) ;
                       word.clear() ;
                   }
                   if ( !digit.empty() ) 
                   {
                       output.push_back( digit ) ;
                       digit.clear() ;
                   }
                   // and then save the current charactor
                   output.push_back( *it ) ;
               }
           }
           else if ( word_len > 1 )
           {
               // keep the word and digits which had been cached first
               if ( !word.empty() )
               {
                   output.push_back( word ) ;
                   word.clear() ;
               }
               if ( !digit.empty() ) 
               {
                   output.push_back( digit ) ;
                   digit.clear() ;
               }
               // and then keep the current charactor
               output.push_back( *it ) ;
           }
       } // for
       // keep the word and digits we had cached
       if ( !word.empty() )
       {
           output.push_back( word ) ;
           word.clear() ;
       }
       if ( !digit.empty() )
       {
           output.push_back( digit ) ;
           digit.clear() ;
       }
   }

   void sdbSplitWords( const string &text, INT32 lineLen, vector<string> &output )
   {
       string one_line ;
       string pre_char ;
       vector<string> vec_result ;
       vector<string> vec_words ;
       vector<string>::iterator it ;
       INT32 left = lineLen ;

       // get words, the output is: "abc", " ", ",", "1", "1024", "集合",...
       _sptChar2Word( text, vec_words ) ;

       // build line
       for( it = vec_words.begin(); it != vec_words.end(); it++ )
       {
           left -= it->length() ;
           if (left > 0)
           {
               one_line += *it ;
               pre_char = *it ;
           }
           else
           {
               BOOLEAN has_handle = FALSE ;
               // if it's "," and ".", we still let 
               // it put to the end of the line
               if ( *it == COMMA_ASCII_EN ||
                    *it == PERIOD_ASCII_EN ||
                    pre_char == BACKSLASH ||
                    *it == COMMA_UTF8_CN ||
                    *it == PERIOD_UTF8_CN )
               {
                   one_line += *it ;
                   has_handle = TRUE ;
               }
               // put the current line into vector
               if ( !one_line.empty() )
               {
                   boost::trim_left( one_line ) ;
                   vec_result.push_back( one_line ) ;
               }
               // clean up
               one_line.clear() ;
               pre_char = "" ;
               // try to start a new line
               if ( !has_handle )
               {
                   // start a new line
                   one_line += *it ;
                   pre_char = *it ;
                   left = lineLen - it->length() ;
               }
               else
               {
                   left = lineLen ;
               }
           }
       }
       // put the last line to the result vector
       if ( !one_line.empty() )
       {
           boost::trim_left( one_line ) ;
           vec_result.push_back( one_line ) ;
       }
       // output
#if defined ( _WINDOWS )
       for ( it = vec_result.begin(); it != vec_result.end(); it++ )
       {
           string strGBK ;
           sptUTF8ToGBK( *it, strGBK ) ;
           output.push_back( strGBK ) ;
       }
#else
       vec_result.swap( output ) ;
#endif
   }

} // namespace

