/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossUtil.cpp

   Descriptive Name = Operating System Services Utilities

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains wrappers for basic System Calls
   or C APIs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ossUtil.h"
#include "ossMem.h"
#include "oss.h"
#include "ryu/ryu.h"
#include <math.h>
#include <float.h>
#if defined (_LINUX) || defined ( _AIX )
#include <locale.h>
#endif

#define OSS_TIMESTAMP_MIN (-2147483648LL)
#define OSS_TIMESTAMP_MAX (2147483647LL)

// All strings represent "true"
const char *OSSTRUELIST[]={
   "YES",
   "yes",
   "Y",
   "y",
   "TRUE",
   "true",
   "T",
   "t",
   "1"};

// All strings represent "false"
const char *OSSFALSELIST[]={
   "NO",
   "no",
   "N",
   "n",
   "FALSE",
   "false",
   "F",
   "f",
   "0"};

CHAR *ossStrdup ( const CHAR * str )
{
   size_t siz ;
   CHAR *copy ;
   siz = ossStrlen ( str ) + 1 ;
   if ( ( copy = (CHAR*)SDB_OSS_MALLOC ( siz ) ) == NULL )
      return NULL ;
   ossMemcpy ( copy, str, siz ) ;
   return copy ;
}

INT32 ossStrToInt ( const CHAR *pBuffer, INT32 *num )
{
   INT32 rc = SDB_OK ;
   const CHAR *pTempBuf = pBuffer ;
   INT32 number = 0 ;
   if ( !num )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   while ( pBuffer &&
           *pBuffer )
   {
      if ( *pBuffer >= '0' &&
           *pBuffer <= '9' )
      {
         number *= 10 ;
         number += ( *pBuffer - '0' ) ;
         ++pBuffer ;
      }
      else if ( '.' == *pBuffer )
      {
         if ( pBuffer - pTempBuf <= 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         break ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   *num = number ;
done:
   return rc ;
error:
   goto done ;
}

size_t ossSnprintf(char* pBuffer, size_t iLength, const char* pFormat, ...)
{
   va_list ap;
   size_t n;
   va_start(ap, pFormat);
#if defined (_WINDOWS)
   n=_vsnprintf_s(pBuffer, iLength, _TRUNCATE, pFormat, ap);
#else
   n=vsnprintf(pBuffer, iLength, pFormat, ap);
#endif
   va_end(ap);
   // Set terminate if the length is greater than buffer size
   if((n<0) || (size_t)n>=iLength)
      n=iLength-1;
   pBuffer[n]='\0';
   return n;
}

// C-safe special-value checks (NaN compares unequal to itself; Inf is
// outside the finite double range and NaN fails both range comparisons)
static BOOLEAN _ossDoubleIsNaNOrInf( FLOAT64 val )
{
   return ( val != val ) || ( val > DBL_MAX ) || ( val < -DBL_MAX ) ;
}

/*
 * Convert Ryu d2s scientific text (e.g. "9.999E1", "1E-7") to %g-like
 * general format: fixed decimal when -4 <= X < P (P = significant digits,
 * X = scientific exponent); otherwise scientific with lowercase 'e',
 * explicit sign, and at least two exponent digits (glibc %g style).
 * On failure (return < 0), buf may hold an unterminated partial write and
 * must be treated as undefined by the caller.
 */
static INT32 _ossRyuSciToGeneral( const CHAR *ryuSci,
                                  CHAR *buf,
                                  INT32 bufSize )
{
   BOOLEAN neg = FALSE ;
   const CHAR *p = NULL ;
   CHAR digits[ 24 ] ;
   INT32 ndigits = 0 ;
   INT32 expX = 0 ;
   BOOLEAN expNeg = FALSE ;
   INT32 outLen = 0 ;
   INT32 i = 0 ;
   INT32 intLen = 0 ;
   INT32 leadZeros = 0 ;
   INT32 fracEnd = 0 ;
   INT32 expAbs = 0 ;
   INT32 expDigits = 0 ;
   INT32 tmp = 0 ;

   if ( NULL == ryuSci || NULL == buf || bufSize <= 0 )
   {
      return -1 ;
   }

   p = ryuSci ;
   if ( '-' == *p )
   {
      neg = TRUE ;
      ++p ;
   }

   if ( *p < '0' || *p > '9' )
   {
      return -1 ;
   }
   digits[ndigits++] = *p++ ;

   if ( '.' == *p )
   {
      ++p ;
      while ( *p >= '0' && *p <= '9' )
      {
         if ( ndigits >= (INT32)sizeof( digits ) )
         {
            return -1 ;
         }
         digits[ndigits++] = *p++ ;
      }
   }

   if ( 'E' != *p && 'e' != *p )
   {
      return -1 ;
   }
   ++p ;

   if ( '-' == *p )
   {
      expNeg = TRUE ;
      ++p ;
   }
   else if ( '+' == *p )
   {
      ++p ;
   }

   if ( *p < '0' || *p > '9' )
   {
      return -1 ;
   }
   while ( *p >= '0' && *p <= '9' )
   {
      /* Guard against signed overflow on hostile/non-Ryu input; IEEE
       * double exponents are far below this threshold. */
      if ( expX > 100000 )
      {
         return -1 ;
      }
      expX = expX * 10 + ( *p - '0' ) ;
      ++p ;
   }
   if ( '\0' != *p )
   {
      return -1 ;
   }
   if ( expNeg )
   {
      expX = -expX ;
   }

   /* ISO C %g: fixed iff -4 <= X < P */
   if ( expX >= -4 && expX < ndigits )
   {
      if ( neg )
      {
         if ( outLen + 1 >= bufSize )
         {
            return -1 ;
         }
         buf[outLen++] = '-' ;
      }

      if ( expX >= 0 )
      {
         intLen = expX + 1 ;
         for ( i = 0 ; i < intLen ; ++i )
         {
            if ( outLen + 1 >= bufSize )
            {
               return -1 ;
            }
            if ( i < ndigits )
            {
               buf[outLen++] = digits[i] ;
            }
            else
            {
               /* Defensive: unreachable for Ryu input because the fixed
                * branch requires expX < ndigits, so intLen <= ndigits. */
               buf[outLen++] = '0' ;
            }
         }
         if ( intLen < ndigits )
         {
            if ( outLen + 1 >= bufSize )
            {
               return -1 ;
            }
            buf[outLen++] = '.' ;
            for ( i = intLen ; i < ndigits ; ++i )
            {
               if ( outLen + 1 >= bufSize )
               {
                  return -1 ;
               }
               buf[outLen++] = digits[i] ;
            }
            /* Defensive trim: Ryu shortest mantissa has no trailing zeros;
             * kept in case reformatting ever introduced them. */
            fracEnd = outLen - 1 ;
            while ( fracEnd >= 0 && '0' == buf[fracEnd] )
            {
               --fracEnd ;
            }
            if ( fracEnd >= 0 && '.' == buf[fracEnd] )
            {
               --fracEnd ;
            }
            outLen = fracEnd + 1 ;
         }
      }
      else
      {
         /* 0.00...digits */
         leadZeros = -expX - 1 ;
         if ( outLen + 2 >= bufSize )
         {
            return -1 ;
         }
         buf[outLen++] = '0' ;
         buf[outLen++] = '.' ;
         for ( i = 0 ; i < leadZeros ; ++i )
         {
            if ( outLen + 1 >= bufSize )
            {
               return -1 ;
            }
            buf[outLen++] = '0' ;
         }
         for ( i = 0 ; i < ndigits ; ++i )
         {
            if ( outLen + 1 >= bufSize )
            {
               return -1 ;
            }
            buf[outLen++] = digits[i] ;
         }
         /* Defensive trim: see comment on the expX >= 0 fractional path. */
         fracEnd = outLen - 1 ;
         while ( fracEnd >= 0 && '0' == buf[fracEnd] )
         {
            --fracEnd ;
         }
         if ( fracEnd >= 0 && '.' == buf[fracEnd] )
         {
            --fracEnd ;
         }
         outLen = fracEnd + 1 ;
      }

      if ( outLen + 1 > bufSize )
      {
         return -1 ;
      }
      buf[outLen] = '\0' ;
      return outLen ;
   }

   /* Scientific notation (glibc %g style) */
   if ( neg )
   {
      if ( outLen + 1 >= bufSize )
      {
         return -1 ;
      }
      buf[outLen++] = '-' ;
   }
   if ( outLen + 1 >= bufSize )
   {
      return -1 ;
   }
   buf[outLen++] = digits[0] ;
   if ( ndigits > 1 )
   {
      if ( outLen + 1 >= bufSize )
      {
         return -1 ;
      }
      buf[outLen++] = '.' ;
      for ( i = 1 ; i < ndigits ; ++i )
      {
         if ( outLen + 1 >= bufSize )
         {
            return -1 ;
         }
         buf[outLen++] = digits[i] ;
      }
   }
   if ( outLen + 1 >= bufSize )
   {
      return -1 ;
   }
   buf[outLen++] = 'e' ;
   if ( outLen + 1 >= bufSize )
   {
      return -1 ;
   }
   buf[outLen++] = ( expX < 0 ) ? '-' : '+' ;
   expAbs = ( expX < 0 ) ? -expX : expX ;
   expDigits = 1 ;
   tmp = expAbs ;
   while ( tmp >= 10 )
   {
      tmp /= 10 ;
      ++expDigits ;
   }
   if ( expDigits < 2 )
   {
      expDigits = 2 ;
   }
   if ( outLen + expDigits >= bufSize )
   {
      return -1 ;
   }
   for ( i = expDigits - 1 ; i >= 0 ; --i )
   {
      buf[outLen + i] = (CHAR)( '0' + ( expAbs % 10 ) ) ;
      expAbs /= 10 ;
   }
   outLen += expDigits ;
   buf[outLen] = '\0' ;
   return outLen ;
}

/*
 * If buf holds an integer-looking decimal (no '.'/'e'/'E'), append ".0"
 * so the text matches sdbexprt float-literal style.
 */
static INT32 _ossEnsureFloatLiteral( CHAR *buf, INT32 outLen, INT32 bufSize )
{
   INT32 i = 0 ;

   if ( NULL == buf || outLen <= 0 || outLen >= bufSize )
   {
      return -1 ;
   }

   for ( i = 0 ; i < outLen ; ++i )
   {
      if ( '.' == buf[i] || 'e' == buf[i] || 'E' == buf[i] )
      {
         return outLen ;
      }
   }

   if ( outLen + 2 >= bufSize )
   {
      return -1 ;
   }
   buf[outLen++] = '.' ;
   buf[outLen++] = '0' ;
   buf[outLen] = '\0' ;
   return outLen ;
}

INT32 ossDoubleToFullPrecisionString( FLOAT64 val, CHAR *buf, INT32 bufSize )
{
   INT32 rc = 0 ;
   INT64 iVal = 0 ;
   CHAR ryuBuf[ 32 ] = { 0 } ;
   /* 2^53: every integer in [-intExactMax, intExactMax] is exactly
    * representable in IEEE-754 binary64. Prefer %lld fixed form here for
    * readability; beyond this range (or non-integers) use Ryu + %g-like
    * rules, so format style can jump across the boundary (e.g. 1e15 vs
    * 1e16). */
   const FLOAT64 intExactMax = 9007199254740992.0 ;

   if ( NULL == buf || bufSize <= 0 )
   {
      return -1 ;
   }

   if ( _ossDoubleIsNaNOrInf( val ) )
   {
      buf[0] = '\0' ;
      return 0 ;
   }

   /* +/-0.0 both compare equal to 0.0; drop signbit and emit "0.0". */
   if ( 0.0 == val )
   {
      if ( bufSize < 4 )
      {
         return -1 ;
      }
      buf[0] = '0' ;
      buf[1] = '.' ;
      buf[2] = '0' ;
      buf[3] = '\0' ;
      return 3 ;
   }

   if ( val >= -intExactMax && val <= intExactMax
        && val == floor( val ) )
   {
      iVal = (INT64)val ;
      if ( (FLOAT64)iVal == val )
      {
         rc = (INT32)ossSnprintf( buf, (size_t)bufSize,
                                  OSS_LL_PRINT_FORMAT, iVal ) ;
         if ( rc < 0 || rc >= bufSize )
         {
            return -1 ;
         }
         return _ossEnsureFloatLiteral( buf, rc, bufSize ) ;
      }
   }

   /* Ryu shortest scientific, then %g-like fixed/scientific selection */
   d2s_buffered( val, ryuBuf ) ;
   rc = _ossRyuSciToGeneral( ryuBuf, buf, bufSize ) ;
   if ( rc < 0 )
   {
      /* Fallback should be rare; keep a conservative printf path */
      rc = (INT32)ossSnprintf( buf, (size_t)bufSize, "%.17g", val ) ;
      if ( rc < 0 || rc >= bufSize )
      {
         return -1 ;
      }
   }
   return _ossEnsureFloatLiteral( buf, rc, bufSize ) ;
}

BOOLEAN ossIsInteger( const CHAR *pStr )
{
   UINT32 i = 0 ;
   while( pStr[i] )
   {
      if ( pStr[i] < '0' || pStr[i] > '9' )
      {
         if ( 0 != i || ( '-' != pStr[i] && '+' != pStr[i] ) )
         {
            return FALSE ;
         }
      }
      ++i ;
   }
   return TRUE ;
}

BOOLEAN ossIsUTF8 ( CHAR *pzInfo )
{
#if defined (_WINDOWS)
   INT32 error  = 0 ;
   INT32 nWSize = MultiByteToWideChar ( CP_UTF8,
                                        MB_ERR_INVALID_CHARS,
                                        pzInfo,
                                        -1, NULL, 0 ) ;
   error = GetLastError () ;
   if ( ERROR_NO_UNICODE_TRANSLATION == error )
      return FALSE ;
   else
      return TRUE ;
#else
   size_t size = 0 ;
   setlocale ( LC_ALL, "" ) ;
   size = mbstowcs ( NULL, pzInfo, 0 ) ;
   if ( (size_t)-1 == size )
      return FALSE ;
   else
      return TRUE ;
#endif
}

#if defined (_WINDOWS)
INT32 ossWC2ANSI ( LPCWSTR lpcszWCString,
                   LPSTR   *plppszString,
                   DWORD   *plpdwString )
{
   INT32 rc           = SDB_OK ;
   INT32 strSize      = 0 ;
   INT32 requiredSize = 0 ;
   requiredSize       = WideCharToMultiByte ( CP_ACP, 0, lpcszWCString,
                                              -1, NULL, 0, NULL, NULL ) ;
   // caller is responsible to free memory
   *plppszString = (LPSTR)SDB_OSS_MALLOC ( requiredSize ) ;
   if ( !(*plppszString) )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   strSize = WideCharToMultiByte ( CP_ACP, 0, lpcszWCString, -1,
                                   *plppszString, requiredSize,
                                   NULL, NULL ) ;
   if ( 0 == strSize )
   {
      SDB_OSS_FREE ( *plppszString ) ;
      rc = SDB_SYS ;
      *plppszString = NULL ;
      goto error ;
   }
   if ( plpdwString )
   {
      *plpdwString = (DWORD)strSize ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 ossANSI2WC ( LPCSTR lpcszString,
                   LPWSTR *plppszWCString,
                   DWORD  *plpdwWCString )
{
   INT32 rc           = SDB_OK ;
   INT32 strSize      = 0 ;
   INT32 requiredSize = 0 ;
   requiredSize       = MultiByteToWideChar ( CP_ACP,
                                              0, lpcszString, -1, NULL, 0 ) ;
   // caller is responsible to free memory
   *plppszWCString = (LPWSTR)SDB_OSS_MALLOC ( requiredSize * sizeof(WCHAR) ) ;
   if ( !(*plppszWCString) )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   strSize = MultiByteToWideChar ( CP_ACP, 0, lpcszString, -1,
                                   *plppszWCString, requiredSize ) ;
   if ( 0 == strSize )
   {
      SDB_OSS_FREE ( *plppszWCString ) ;
      rc = SDB_SYS ;
      *plppszWCString = NULL ;
      goto error ;
   }

   if ( plpdwWCString )
      *plpdwWCString = strSize ;
done :
   return rc ;
error :
   goto done ;
}
#elif defined (_LINUX)
void ossCloseAllOpenFileHandles ( BOOLEAN closeSTD )
{
   INT32 i = 3 ;
   INT32 max = OSS_FD_SETSIZE ;
   // if we want to close STDIN/STDOUT/STDERR, then we should start form 0
   if ( closeSTD )
   {
      i = 0 ;
   }
   while ( i < max )
   {
      close ( i ) ;
      ++i ;
   }
   if ( closeSTD )
   {
      INT32 fd = 0 ;
      // if we are told to close STD, we have to redirect STDIN/STDOUT/STDERR to
      // /dev/null
      close ( STDIN_FILENO ) ;
      // after close fd 0, next the fd should be using 0
      fd = open ( SDB_DEV_NULL, O_RDWR ) ;
      if ( -1 != fd )
      {
         dup2 ( fd, STDOUT_FILENO ) ;
         dup2 ( fd, STDERR_FILENO ) ;
      }
   }
}

void ossCloseStdFds()
{
   INT32 fd = 0 ;

   fd = open ( SDB_DEV_NULL, O_RDWR ) ;
   if ( -1 != fd )
   {
      dup2 ( fd, STDIN_FILENO ) ;
      dup2 ( fd, STDOUT_FILENO ) ;
      dup2 ( fd, STDERR_FILENO ) ;
      close( fd ) ;
   }
}

#endif
INT32 ossStrncasecmp ( const CHAR *pString1, const CHAR *pString2,
                       size_t iLength)
{
#if defined (_LINUX) || defined ( _AIX )
   return strncasecmp(pString1, pString2, iLength);
#else
   if (iLength==0)
      return 0;
   while (iLength--!=0 && tolower(*pString1)==tolower(*pString2))
   {
      if(iLength==0||*pString1=='\0'||*pString2=='\0')
         break;
      pString1++;
      pString2++;
   }
   return tolower(*(unsigned char*)pString1) - tolower(*(unsigned
                    char*)pString2);
#endif
}

CHAR *ossStrnchr(const CHAR *pString, UINT32 c, UINT32 n)
{
   const CHAR* p = pString;
   while(n--)
      if(*p==(CHAR)c) return (CHAR*)p; else ++p;
   return NULL;
}

INT32 ossStrToBoolean(const char* pString, BOOLEAN* pBoolean)
{
   INT32  rc           = SDB_OK ;
   UINT32 i            = 0 ;
   size_t len          = ossStrlen(pString) ;

   if (0 == len)
   {
      *pBoolean=FALSE ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   for(; i < sizeof(OSSTRUELIST)/sizeof(ossValuePtr); i++)
   {
      if(ossStrncasecmp(pString, OSSTRUELIST[i], len) == 0)
      {
         *pBoolean = TRUE ;
         goto done ;
      }
   }
   for(i = 0; i < sizeof(OSSFALSELIST)/sizeof(ossValuePtr); i++)
   {
      if(ossStrncasecmp(pString, OSSFALSELIST[i], len) == 0)
      {
         *pBoolean = FALSE ;
         goto done ;
      }
   }

   *pBoolean = FALSE ;
   rc = SDB_INVALIDARG ;
   goto error ;

done :
   return rc ;
error :
   goto done ;
}

size_t ossVsnprintf
(
   char * buf, size_t size, const char * fmt, va_list ap
)
{
   size_t n ;
   size_t terminator ;

#if defined (_WINDOWS)
   n = _vsnprintf_s( buf, size, _TRUNCATE, fmt, ap ) ;
#elif defined (_LINUX) || defined ( _AIX )
   n = vsnprintf( buf, size, fmt, ap ) ;
#endif

   // Add the NULL terminator after the string
   if ( ( n >= 0 ) && ( n < size ) )
   {
      terminator = n ;
   }
   else
   {
      terminator = size - 1 ;
   }
   buf[terminator] = '\0' ;

   return terminator ;
}

UINT32 ossHashFileName ( const CHAR *fileName )
{
   const CHAR *pathSep = OSS_FILE_SEP ;
   const CHAR *pFileName = ossStrrchr ( fileName, pathSep[0] ) ;
   if ( !pFileName )
      pFileName = fileName ;
   else
      pFileName++ ;
   return ossHash ( pFileName, (INT32)ossStrlen ( pFileName ) ) ;
}

/**
 * Wrapper of mktime. Supports automatic DST(daylight saving time) corrention.
 *
 * DST introduce:
 * 1. DST begin, the time skips an hour, and the skipped time is called the
 * time gap, such as 2:30.
 *    CST(+8h)  |    Time gap(+1h)   |   CDT(+9h)
 * -------------|----------_---------|-------------->
 *             1:59       2:30      3:00
 *
 * 2. DST end, the time needs to be rolled back 1h, and there are duplication
 * times at this time, such as two 1:30
 *    CDT(+9h)  |  Time repeat(-1h)  |   CST(+8)
 * -------------|----------_---------|-------_------>
 *             1:59       1:30      1:00    1:30
 */
time_t ossMkTime ( struct tm *timePtr )
{
   time_t result = 0 ;
   time_t retryResult = 0 ;
   struct tm fixTime = { 0 } ;

   if ( !timePtr )
   {
      return -1 ;
   }
   fixTime = *timePtr ;

   // Set isdst to 0 to handle repeated times as non-DST
   timePtr->tm_isdst = 0 ;
   result = mktime( timePtr ) ;
   if ( 1 == timePtr->tm_isdst )
   {
      // DST corrention
      fixTime.tm_isdst = 1 ;
      retryResult = mktime( &fixTime ) ;
      // 1. If isdst did not change, it means that the time is DST, and the
      // corrected time is correct.
      // 2. If isdst changed, it means that the time is time gap, and it
      // does not require corrention.
      if ( fixTime.tm_isdst == timePtr->tm_isdst )
      {
         result = retryResult ;
      }
   }
   return result ;
}

// blow hash function is coming from
// http://www.azillionmonkeys.com/qed/hash.html
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const UINT16 *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((UINT32)(((const UINT8 *)(d))[1])) << 8)\
                       +(UINT32)(((const UINT8 *)(d))[0]) )
#endif
UINT32 ossHash ( const CHAR *data, INT32 len )
{
   UINT32 hash = len, tmp ;
   INT32 rem ;
   if ( len <= 0 || data == NULL ) return 0 ;
   rem = len&3 ;
   len >>= 2 ;
   for (; len > 0 ; --len )
   {
      hash += get16bits (data) ;
      tmp   = (get16bits (data+2) << 11) ^ hash;
      hash  = (hash<<16)^tmp ;
      data += 2*sizeof(UINT16) ;
      hash += hash>>11 ;
   }
   switch ( rem )
   {
   case 3:
      hash += get16bits (data) ;
      hash ^= hash<<16 ;
      hash ^= ((SINT8)data[sizeof (UINT16)])<<18 ;
      hash += hash>>11 ;
      break ;
   case 2:
      hash += get16bits(data) ;
      hash ^= hash <<11 ;
      hash += hash >>17 ;
      break ;
   case 1:
      hash += (SINT8)*data ;
      hash ^= hash<<10 ;
      hash += hash>>1 ;
   }
   hash ^= hash<<3 ;
   hash += hash>>5 ;
   hash ^= hash<<4 ;
   hash += hash>>17 ;
   hash ^= hash<<25 ;
   hash += hash>>6 ;
   return hash ;
}
#undef get16bits

INT32 ossDup2( int oldFd, int newFd )
{
#if defined( _WINDOWS )
   if ( _dup2( oldFd, newFd ) != 0 )
   {
      return SDB_SYS ;
   }
#else
   if ( dup2( oldFd, newFd ) < 0 )
   {
      return SDB_SYS ;
   }
#endif // _WINDOWS
   return SDB_OK ;
}

INT32 ossResetTty()
{
   INT32 rc     = SDB_OK ;
   FILE *stream = NULL ;
#if defined(_WINDOWS)
   stream = freopen( "CON", "w", stdout ) ;
#else
   stream = freopen( "/dev/tty", "w", stdout ) ;
#endif
   if ( NULL == stream )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}


BOOLEAN ossIsTimestampValid( INT64 tm )
{
   if( tm > OSS_TIMESTAMP_MAX || tm < OSS_TIMESTAMP_MIN )
   {
      return FALSE ;
   }

   return TRUE ;
}


