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

   Source File Name = utilStr.cpp

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
#include "utilStr.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "timestamp.h"
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <algorithm>
#include <functional>
#include <cctype>
#include <iostream>

using namespace boost::xpressive ;
namespace engine
{
   INT32 utilStrTrimBegin( const CHAR *src, const CHAR *&begin )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == src )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
      UINT32 len = ossStrlen( src ) ;
      UINT32 sub = 0 ;
      while ( sub < len )
      {
         if ( ' ' == src[sub]
              || '\t' == src[sub] )
         {
            ++sub ;
         }
         else
         {
            break ;
         }
      }

      begin = &(src[sub]) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilStrTrimEnd( CHAR *src )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == src )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
      UINT32 len = ossStrlen( src ) ;
      INT32 sub = len - 1 ;
      while ( -1 < sub )
      {
         if ( ' ' == src[sub]
              || '\t' == src[sub] )
         {
            src[sub--] = '\0' ;
         }
         else
         {
            break ;
         }
      }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilStrTrim( CHAR *src, const CHAR *&begin )
   {
      INT32 rc = SDB_OK ;
      const CHAR *tmpBegin = NULL ;
      rc = utilStrTrimBegin( src, tmpBegin ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = utilStrTrimEnd( (CHAR *)tmpBegin ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      begin = tmpBegin ;
   done:
      return rc ;
   error:
      goto done ;
   }

   std::string &utilStrLtrim ( std::string &s )
   {
      s.erase ( s.begin(), std::find_if ( s.begin(), s.end(),
                std::not1 ( std::ptr_fun<int, int>(std::isspace)))) ;
      return s ;
   }

   std::string &utilStrRtrim ( std::string &s )
   {
      s.erase ( std::find_if ( s.rbegin(), s.rend(),
                std::not1 ( std::ptr_fun<int, int>(std::isspace))).base(),
                s.end() ) ;
      return s ;
   }

   std::string &utilStrTrim ( std::string &s )
   {
      return utilStrLtrim ( utilStrRtrim ( s ) ) ;
   }

   INT32 utilStrToUpper( const CHAR *src, CHAR *&upper )
   {
      INT32 rc = SDB_OK ;
      CHAR *tmp = NULL ;
      UINT32 size = 0 ;
      if ( NULL == src )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      size = ossStrlen( src) + 1 ;
      tmp = (CHAR *)SDB_OSS_MALLOC(size) ;
      if ( NULL == tmp )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         goto error ;
      }

      /// '\0' is contained.
      for ( UINT32 i = 0; i < size ; i++ )
      {
         tmp[i] = ( src[i] >= 'a' && src[i] <= 'z' ) ?
                    src[i] - 32 : src[i] ;
      }

      upper = tmp ;
   done:
      return rc ;
   error:
      if ( NULL != tmp )
      {
         SDB_OSS_FREE( tmp ) ;
      }
      goto done ;
   }

   BOOLEAN utilStrStartsWithIgnoreCase( const string& str, const string& substr )
   {
      if ( str.empty() || substr.empty() || str.size() < substr.size() )
      {
         return FALSE ;
      }

      return ossStrncasecmp( str.c_str(), substr.c_str(), substr.size() ) == 0 ?
               TRUE : FALSE ;
   }

   BOOLEAN utilStrIsDigit( const string& str )
   {
      if ( 0 == str.size() )
      {
         return FALSE ;
      }
      for ( UINT32 i = 0 ; i < str.size() ; i++ )
      {
         if ( ( '-' == str.at( i ) || '+' == str.at( i ) ) && 0 == i )
         {
            continue ;
         }
         if ( !isdigit( str.at( i ) ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   BOOLEAN utilStrIsDigit( const char *str )
   {
      if ( NULL == str )
      {
         return FALSE ;
      }
      UINT32 len = ossStrlen( str ) ;
      for ( UINT32 i = 0 ; i < len ; i++ )
      {
         if ( ( '-' == str[ i ] || '+' == str[ i ] ) && 0 == i )
         {
            continue ;
         }
         if ( !isdigit( str[ i ] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   BOOLEAN utilStrIsODigit( const char *str )
   {
      UINT32 len = ossStrlen( str ) ;
      for ( UINT32 i = 0 ; i < len ; i++ )
      {
         if ( str[ i ] < '0' || str[ i ] > '7' )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   BOOLEAN utilStrIsXDigit( const char *str )
   {

      UINT32 len = ossStrlen( str ) ;
      for ( UINT32 i = 0 ; i < len ; i++ )
      {
         if ( !isxdigit( str[ i ] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   vector<string> utilStrSplit( const string& str, const string& sep )
   {
      vector<string> elems ;
      size_t pos = 0 ;
      size_t len = str.length() ;
      size_t sepLen = sep.length() ;

      if ( sepLen == 0 )
      {
        return elems ;
      }

      while ( pos < len )
      {
        int findPos = str.find( sep, pos ) ;
        if ( findPos < 0 )
        {
            elems.push_back( str.substr( pos, len - pos ) ) ;
            break ;
        }

        elems.push_back( str.substr( pos, findPos - pos ) ) ;
        pos = findPos + sepLen ;
      }

      return elems ;
   }

   INT32 utilSplitStr( const string & input,
                       vector < string > & listServices,
                       const string & seperators )
   {
      INT32 rc = SDB_OK ;
      CHAR *cstr = NULL ;
      CHAR *p = NULL ;
      CHAR *pContext = NULL ;
      INT32 bufSize = input.size() ;

      cstr = (CHAR*)SDB_OSS_MALLOC ( bufSize + 1 ) ;
      if ( !cstr )
      {
         std::cout << "Alloc memory(" << bufSize + 1 << ") failed"
                   << std::endl ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset ( cstr, 0, bufSize + 1 ) ;
      ossStrncpy ( cstr, input.c_str(), bufSize ) ;

      p = ossStrtok ( cstr, seperators.c_str(), &pContext ) ;
      while ( p )
      {
         string ts ( p ) ;
         listServices.push_back ( ts ) ;
         p = ossStrtok ( NULL, seperators.c_str(), &pContext ) ;
      }

   done :
      if ( cstr )
      {
         SDB_OSS_FREE ( cstr ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 utilStr2Num( const CHAR *str, INT32 &num, INT32 typeMask )
   {
      INT32 rc = SDB_OK ;
      INT32 scanRc = SDB_OK ;
      INT32 tmpNum = 0 ;

      if ( ( typeMask & UTIL_STR2NUM_HEX ) &&
           0 == ossStrncmp( str, HEX_PRE, HEX_PRE_SIZE ) )
      {
         str = str + 2 ;
         if ( '\0' == *str )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( ! utilStrIsXDigit( str ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         scanRc = ossSscanf( str, "%x", &tmpNum ) ;
         if ( -1 == scanRc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( ( typeMask & UTIL_STR2NUM_OCT ) &&
                0 == ossStrncmp( str, OCT_PRE, OCT_PRE_SIZE ) )
      {
         if ( ! utilStrIsODigit( str ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         scanRc = ossSscanf( str, "%o", &tmpNum ) ;
         if ( -1 == scanRc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( typeMask & UTIL_STR2NUM_DEC )
      {
         if ( ! utilStrIsDigit( str ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         scanRc = ossSscanf( str, "%d", &tmpNum ) ;
         if ( -1 == scanRc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      num = tmpNum ;

   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 utilStr2TimeT( const CHAR *str,
                        time_t &tm,
                        UINT64 *usec )
   {
      INT32 rc = SDB_OK ;
      struct tm t ;
      memset ( &t, 0, sizeof(t) ) ;
      INT32 year   = 0 ;
      INT32 month  = 0 ;
      INT32 day    = 0 ;
      INT32 hour   = 0 ;
      INT32 minute = 0 ;
      INT32 second = 0 ;
      INT32 micros = 0 ;
      BOOLEAN hasColon = FALSE ;

      if( ossStrchr( str, 'T' ) || ossStrchr( str, 't' ) )
      {
         // the format "2000-01-01T(t)01:30:24:999999Z(z)" or
         // "2000-01-01T(t)01:30:24:000000+0800"
         /* for mongo date type, iso8601 */
         sdbTimestamp sdbTime ;
         if( timestampParse( str, ossStrlen( str ), &sdbTime ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         tm = (time_t)sdbTime.sec ;
         micros = sdbTime.nsec / 1000 ;
      }
      else
      {
         // the format is "2000-01-01-13.14.26.123456"
         // the bound is
         // timestamp min 1901-12-13-20.45.52.000000 +/- TZ
         // timestamp max 2038-01-19-03.14.07.999999 +/- TZ
         static cregex reg = cregex::compile("^((((19|[2-9]\\d)\\d{2})-(0?[13578]|1[02])-(0?[1-9]|[12]\\d|3[01]))|(((19|[2-9]\\d)\\d{2})-(0?[13456789]|1[012])-(0?[1-9]|[12]\\d|30))|(((19|[2-9]\\d)\\d{2})-0?2-(0?[1-9]|1\\d|2[0-8]))|(((19|[2-9]\\d)(0[48]|[2468][048]|[13579][26])|(([2468][048]|[3579][26])00))-0?2-29))-(20|21|22|23|[0-1]?\\d).[0-5]?\\d.[0-5]?\\d(.[0-9]{6})?$") ;
         if ( !( regex_match( str, reg ) ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if( ossStrchr( str, ':' ) )
         {
            hasColon = TRUE ;
         }

         if ( !sscanf ( str,
                        hasColon ? "%d-%d-%d-%d:%d:%d.%d" : "%d-%d-%d-%d.%d.%d.%d",
                        &year   ,
                        &month  ,
                        &day    ,
                        &hour   ,
                        &minute ,
                        &second ,
                        &micros ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         t.tm_year  = year - 1900  ;
         t.tm_mon   = month - 1 ;
         t.tm_mday  = day    ;
         t.tm_hour  = hour   ;
         t.tm_min   = minute ;
         t.tm_sec   = second ;

         tm = mktime( &t ) ;
      }
      if ( NULL != usec )
      {
         *usec = micros ;
      }

      if ( !ossIsTimestampValid( ( INT64 ) tm ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilStr2Date( const CHAR *str, UINT64 &millis )
   {
      INT32 rc = SDB_OK ;
      struct tm t ;
      time_t timep ;
      memset ( &t, 0, sizeof(t) ) ;
      INT32 year   = 0 ;
      INT32 month  = 0 ;
      INT32 day    = 0 ;

      if( ossStrchr( str, 'T' ) || ossStrchr( str, 't' ) )
      {
         // the format "2000-01-01T(t)01:30:24:999999Z(z)" or
         // "2000-01-01T(t)01:30:24:000000+0800"
         /* for mongo date type, iso8601 */
         INT32 micros = 0 ;
         sdbTimestamp sdbTime ;
         if( timestampParse( str, ossStrlen( str ), &sdbTime ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         timep = (time_t)sdbTime.sec ;
         micros = sdbTime.nsec / 1000 ;
         millis = timep * 1000 + micros / 1000 ;
      }
      else
      {
         // the format is "2000-01-01"
         // the bound is "0000-01-01" - "9999-12-31"
         static cregex reg = cregex::compile("^(((\\d{4})-(0?[13578]|1[02])-(0?[1-9]|[12]\\d|3[01]))|((\\d{4})-(0?[13456789]|1[012])-(0?[1-9]|[12]\\d|30))|((\\d{4})-0?2-(0?[1-9]|1\\d|2[0-8]))|(((\\d{2})(0[48]|[2468][048]|[13579][26])|((0[048]|1[26]|[2468][048]|[3579][26])00))-0?2-29))$") ;
         if ( !( regex_match( str, reg ) ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( !sscanf ( str,
                        "%d-%d-%d",
                        &year   ,
                        &month  ,
                        &day ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         t.tm_year  = year - 1900  ;
         t.tm_mon   = month - 1 ;
         t.tm_mday  = day    ;

         timep = mktime( &t ) ;
         millis = timep * 1000 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilBuildFullPath( const CHAR *path, const CHAR *name,
                            UINT32 fullSize, CHAR *full )
   {
      INT32 rc = SDB_OK ;
      if ( ossStrlen( path ) + ossStrlen( name )
           + 2 > fullSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossMemset( full, 0, fullSize );
      ossStrcpy( full, path ) ;
      if ( '\0' != path[0] &&
           0 != ossStrcmp(&path[ossStrlen(path)-1], OSS_FILE_SEP ) )
      {
         ossStrncat( full, OSS_FILE_SEP, 1 ) ;
      }
      ossStrncat( full, name, ossStrlen( name ) ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilCatPath( CHAR * src, UINT32 srcSize, const CHAR * catStr )
   {
      INT32 rc = SDB_OK ;
      UINT32 srcLen = ossStrlen( src ) ;
      UINT32 catStrLen = ossStrlen( catStr ) ;

      if ( srcLen + catStrLen + 2 > srcSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( srcLen > 0 && src[srcLen-1] != OSS_FILE_SEP_CHAR )
      {
         ossStrncat( src, OSS_FILE_SEP, 1 ) ;
      }
      ossStrncat( src, catStr, catStrLen ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* utilAscTime( time_t tTime, CHAR * pBuff, UINT32 size )
   {
      struct tm localTm ;
      ossLocalTime( tTime, localTm ) ;

      ossMemset( pBuff, 0, size ) ;
      ossSnprintf( pBuff, size-1,
                   "%04d-%02d-%02d-%02d:%02d:%02d",
                   localTm.tm_year+1900,            // 1) Year (UINT32)
                   localTm.tm_mon+1,                // 2) Month (UINT32)
                   localTm.tm_mday,                 // 3) Day (UINT32)
                   localTm.tm_hour,                 // 4) Hour (UINT32)
                   localTm.tm_min,                  // 5) Minute (UINT32)
                   localTm.tm_sec                   // 6) Second (UINT32)
                  ) ;
      pBuff[ size - 1 ] = 0 ;
      return pBuff ;
   }

   BOOLEAN isValidIPV4( const CHAR *ip )
   {
      //ipv4 range: 0.0.0.0 - 255.255.255.255
      static cregex rangeReg = cregex::compile(
            "(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[.]" \
            "(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[.]" \
            "(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[.]" \
            "(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])" ) ;
      //invalid address: 0.0.0.0, 127.0.0.1, 255.255.255.255
      static cregex invalidReg = cregex::compile( "0[.]0[.]0[.]0|" \
                                                  "127[.]0[.]0[.]1|" \
                                                  "255[.]255[.]255[.]255" ) ;

      return regex_match( ip, rangeReg ) && !regex_match( ip, invalidReg ) ;
   }

   BOOLEAN utilIsValidOID( const CHAR * pStr )
   {
      if ( NULL == pStr || 24 > ossStrlen( pStr ) )
      {
         return FALSE ;
      }
      for ( UINT32 i = 0; i < 24; ++i )
      {
         if ( ! ( ( pStr[i] >= '0' && pStr[i] <= '9' ) ||
                  ( pStr[i] >= 'a' && pStr[i] <= 'f' ) ||
                  ( pStr[i] >= 'A' && pStr[i] <= 'F' ) ) )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   string utilTimeSpanStr( UINT64 seconds )
   {
      stringstream ss ;
      BOOLEAN beginCalc = FALSE ;

      if ( seconds > 86400 ) // days
      {
         ss << seconds / 86400 << " days " ;
         seconds %= 86400 ;
         beginCalc = TRUE ;
      }
      if ( beginCalc || seconds > 3600 ) // hours
      {
         ss << seconds / 3600 << " hours " ;
         seconds %= 3600 ;
         beginCalc = TRUE ;
      }
      if ( beginCalc || seconds > 60 ) // mins
      {
         ss << seconds / 60 << " mins " ;
         seconds %= 60 ;
         beginCalc = TRUE ;
      }
      ss << seconds << " secs" ;

      return ss.str() ;
   }

   INT32 utilParseVersion( CHAR * pVersionStr,
                           INT32 &version,
                           INT32 &subVersion,
                           INT32 &fixVersion,
                           INT32 &release,
                           string &buildInfo )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pDelim = " \r\n" ;
      CHAR *pToken = NULL ;
      CHAR *pLast = NULL ;

      if ( !pVersionStr )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pToken = ossStrtok( pVersionStr, pDelim, &pLast ) ;
      // XXX version: 1.8[.x]
      if ( !pToken )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         CHAR *pTokenTmp = NULL ;
         CHAR *pLastTmp = NULL ;
         // 1.8[.x]
         CHAR *pVerPtr = ossStrstr( pToken, ":" ) ;
         if ( !pVerPtr )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // 1
         pTokenTmp = ossStrtok( pVerPtr, " .", &pLastTmp ) ;
         if ( !pTokenTmp )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         version = ossAtoi( pTokenTmp ) ;

         // 8
         pTokenTmp = ossStrtok( NULL, " .", &pLastTmp ) ;
         if ( !pTokenTmp )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         subVersion = ossAtoi( pTokenTmp ) ;

         fixVersion = 0 ;
         //[.x]
         pTokenTmp = ossStrtok( NULL, " .", &pLastTmp ) ;
         if ( pTokenTmp )
         {
            fixVersion = ossAtoi( pTokenTmp ) ;
         }
      }

      // Release: 14702
      pToken = ossStrtok( NULL, pDelim, &pLast ) ;
      if ( !pToken )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         string releaseStr ;
         CHAR *pReleasePtr = ossStrstr( pToken, ":" ) ;
         if ( !pReleasePtr )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         releaseStr = pReleasePtr ;
         utilStrLtrim( releaseStr ) ;
         release = ossAtoi( releaseStr.c_str() ) ;
      }

      // 2014-08-31-23.18.18(Debug)
      pToken = ossStrtok( NULL, pDelim, &pLast ) ;
      if ( !pToken )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      buildInfo = pToken ;

   done:
      return rc ;
   error:
      goto done ;
   }

   CHAR* utilSplitIterator::_nextptr( CHAR *str )
   {
      while( *str )
      {
         if ( _ch == *str )
         {
            ++str ;
            continue ;
         }
         return str ;
      }
      return NULL ;
   }

   BOOLEAN utilSplitIterator::more() const
   {
      return _src ? TRUE : FALSE ;
   }

   const CHAR *utilSplitIterator::next()
   {
      CHAR *ch = NULL ;
      const CHAR *r = "" ;

      /// restore
      if ( NULL != _last )
      {
         *_last = _ch ;
         _last = NULL ;
      }

      if ( !_src || !(*_src) )
      {
         goto done ;
      }

      ch = ossStrchr( _src, _ch ) ;

      /// "abc"
      if ( NULL == ch )
      {
         r = _src ;
         _src = NULL ;
         goto done ;
      }

      _last = ch ;
      r = _src ;
      _src = _nextptr( ch ) ;
      *_last = '\0' ;

   done:
      return r ;
   }
}

