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
   if ( !plppszString )
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
   if ( !plppszWCString )
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


