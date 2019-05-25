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
      targetStr = chars ;
      
   done:
      SAFE_OSS_FREE( chars ) ;
      SAFE_OSS_FREE( wchars ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 sptGBKToUTF8( const std::string& strGBK, std::string &strUTF8 )
   {
      return _sptConvertMultiByte( CP_ACP, CP_UTF8, strGBK, strUTF8 ) ;
   }

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
                   output.push_back( *it ) ;
               }
           }
           else if ( word_len > 1 )
           {
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
               output.push_back( *it ) ;
           }
       } // for
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

       _sptChar2Word( text, vec_words ) ;

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
               if ( *it == COMMA_ASCII_EN ||
                    *it == PERIOD_ASCII_EN ||
                    pre_char == BACKSLASH ||
                    *it == COMMA_UTF8_CN ||
                    *it == PERIOD_UTF8_CN )
               {
                   one_line += *it ;
                   has_handle = TRUE ;
               }
               if ( !one_line.empty() )
               {
                   boost::trim_left( one_line ) ;
                   vec_result.push_back( one_line ) ;
               }
               one_line.clear() ;
               pre_char = "" ;
               if ( !has_handle )
               {
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
       if ( !one_line.empty() )
       {
           boost::trim_left( one_line ) ;
           vec_result.push_back( one_line ) ;
       }
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

