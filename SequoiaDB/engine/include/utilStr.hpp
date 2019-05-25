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

   Source File Name = utilStr.hpp

   Descriptive Name =

   When/how to use: str util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTILSTR_HPP_
#define UTILSTR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

#define HEX_PRE          "0x"
#define HEX_PRE_SIZE     ( sizeof( HEX_PRE ) -1 )
#define OCT_PRE          "0"
#define OCT_PRE_SIZE     ( sizeof( OCT_PRE ) -1 )

using namespace std ;

namespace engine
{
   INT32 utilStrTrimBegin( const CHAR *src, const CHAR *&begin ) ;
   std::string &utilStrLtrim ( std::string &s ) ;

   INT32 utilStrTrimEnd( CHAR *src ) ;
   std::string &utilStrRtrim ( std::string &s ) ;

   INT32 utilStrTrim( CHAR *src, const CHAR *&begin ) ;
   std::string &utilStrTrim ( std::string &s ) ;

   OSS_INLINE BOOLEAN utilStrStartsWith( const string& str, const string& substr )
   {
      if ( str.empty() || substr.empty() )
      {
         return FALSE ;
      }

      return str.compare( 0, substr.size(), substr ) == 0 ? TRUE : FALSE ;
   }

   OSS_INLINE BOOLEAN utilStrEndsWith( const string& str, const string& substr )
   {
      if ( str.empty() || substr.empty() )
      {
         return FALSE ;
      }

      return str.compare( str.size() - substr.size(), substr.size(), substr ) == 0 ?
               TRUE : FALSE ;
   }

   INT32 utilStrToUpper( const CHAR *src, CHAR *&upper ) ;

   BOOLEAN utilStrIsDigit( const string& str ) ;

   BOOLEAN utilStrIsDigit( const char *str ) ;

   BOOLEAN utilStrIsODigit( const char *str ) ;

   BOOLEAN utilStrIsXDigit( const char *str ) ;

   vector<string> utilStrSplit( const string& str, const string& sep ) ;

   INT32 utilSplitStr( const string &input, vector<string> &listServices,
                       const string &seperators ) ;

   INT32 utilStr2Num( const CHAR *str, INT32 &num ) ;
   INT32 utilStr2TimeT( const CHAR *str,
                        time_t &tm,
                        UINT64 *usec = NULL ) ;

   INT32 utilStr2Date( const CHAR *str, UINT64 &millis ) ;

   INT32 utilBuildFullPath( const CHAR *path, const CHAR *name,
                            UINT32 fullSize, CHAR *full ) ;

   INT32 utilCatPath( CHAR *src, UINT32 srcSize, const CHAR *catStr ) ;

   const CHAR* utilAscTime( time_t tTime, CHAR *pBuff, UINT32 size ) ;

   BOOLEAN isValidIPV4( const CHAR *ip ) ;

   string utilTimeSpanStr( UINT64 seconds ) ;

   INT32 utilParseVersion( CHAR *pVersionStr,    // in
                           INT32 &version,       // out
                           INT32 &subVersion,    // out
                           INT32 &fixVersion,    // out
                           INT32 &release,       // out
                           string &buildInfo ) ;

   BOOLEAN utilIsValidOID( const CHAR *pStr ) ;

   class utilSplitIterator : public SDBObject
   {
   public:
      utilSplitIterator( CHAR *src, CHAR ch = '.' )
      {
         _src = NULL ;
         _ch = ch ;
         _last = NULL ;

         if ( src )
         {
            _src = _nextptr( src ) ;
         }
      }

      ~utilSplitIterator()
      {
         finish() ;
      }

      void finish()
      {
         if ( NULL != _last )
         {
            *_last = _ch ;
            _last = NULL ;
         }
         _src = NULL ;
      }

   protected:
      CHAR  *_nextptr( CHAR *str ) ;

   public:
      BOOLEAN more() const ;
      const CHAR *next() ;
   private:
      CHAR *_src ;
      CHAR _ch ;
      CHAR *_last ;
   } ;
}

#endif // UTILSTR_HPP_

