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

   Source File Name = ossUtil.h

   Descriptive Name = Operating System Services Utility Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains wrappers for utilities like
   memcpy, strcmp, etc...

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSUTIL_H_
#define OSSUTIL_H_
#include "core.h"
#include <time.h>
#include <sys/types.h>
#if defined (_LINUX) || defined ( _AIX )
#include <sys/time.h>
#include <strings.h>
#include <pthread.h>
#include <signal.h>
#else
#include <io.h>
#endif
#include "oss.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define OSS_TEN_MILLION 10000000
#define OSS_ONE_MILLION 1000000
#define OSS_ONE_SEC     (1000)

#define OSS_EPSILON     (1e-6)

SDB_EXTERN_C_START
size_t ossSnprintf(CHAR* pBuffer, size_t iLength, const CHAR* pFormat, ...);
#define ossSscanf( x, y, ... ) sscanf( x, y, ##__VA_ARGS__ ) ;
CHAR *ossStrdup ( const CHAR *str ) ;
INT32 ossStrToInt ( const CHAR *pBuffer, INT32 *number ) ;
#define ossStrncmp(x,y,z) strncmp(x,y,z)
#define ossStrcmp(x,y) strcmp(x,y)
#define ossStrcpy(x,y) strcpy(x,y)
#if defined (_LINUX) || defined ( _AIX )
#define ossStrncpy(x,y,z) strncpy(x,y,z)
#define ossStrncat(x,y,z) strncat(x,y,z)
#define ossStrtok(x,y,z) strtok_r(x,y,z)
#define ossFdopen(x,y) fdopen(x,y)
#define OSS_LL_PRINT_FORMAT   "%lld"
#define ossStrcasecmp(x,y)  strcasecmp(x,y)
#elif defined (_WINDOWS)
#define ossStrncpy(x,y,z) strncpy(x,y,z)
#define ossStrncat(x,y,z) strncat(x,y,z)
#define ossStrtok(x,y,z) strtok_s(x,y,z)
#define ossFdopen(x,y) _fdopen(x,y)
#define OSS_LL_PRINT_FORMAT   "%I64d"
#define ossStrcasecmp(x,y)  _stricmp(x,y)
#endif
#define ossMemcpy(x,y,z) memcpy(x,y,z)
#define ossMemmove(x,y,z) memmove(x,y,z)
#define ossMemset(x,y,z) memset(x,y,z)
#define ossMemcmp(x,y,z) memcmp(x,y,z)
#define ossStrlen(x) strlen(x)
#define ossStrstr(x,y) strstr(x,y)
#define ossStrrchr(x,y) strrchr(x,y)
#define ossStrchr(x,y) strchr(x,y)
#define ossAtoi(x) atoi(x)
#define ossAtof(x) atof(x)
#define ossIsspace(c) isspace(c)

#define ossItoa(x,y,z) if (y) { ossSnprintf(y, z, "%d", (INT32)(x) );}
#define ossLltoa(x,y,z) if (y) { ossSnprintf(y, z, OSS_LL_PRINT_FORMAT, (INT64)(x) );}

BOOLEAN ossIsTimestampValid( INT64 tm ) ;

INT32 ossDup2( int oldFd, int newFd ) ;

INT32 ossResetTty() ;

#if defined (_WINDOWS)
   #define  ossDup(fd)                 _dup(fd)
   #define  ossCloseFd(fd)             _close(fd)
#else
   #define  ossDup(fd)                 dup(fd)
   #define  ossCloseFd(fd)             close(fd)
#endif // _WINDOWS

#if defined (_WINDOWS)
#define ossAtoll(x) _atoi64(x)
#elif defined (_LINUX) || defined ( _AIX )
#define ossAtoll(x) atoll(x)
#endif
#if defined (_NOSCREENOUT)
#define ossPrintf(x,...)
#else
#define ossPrintf(x,...) \
      do {\
         printf((x), ##__VA_ARGS__);\
         fflush(stdout) ;\
      } while (0)
#endif
// String compare without case
BOOLEAN ossIsInteger( const CHAR *pStr ) ;
BOOLEAN ossIsUTF8 ( CHAR *pzInfo ) ;
INT32 ossStrncasecmp ( const CHAR *pString1, const CHAR *pString2,
                       size_t iLength) ;
CHAR *ossStrnchr(const CHAR *pString, UINT32 c, UINT32 n) ;
size_t ossVsnprintf (CHAR * buf, size_t size, const CHAR * fmt, va_list ap);
INT32 ossStrToBoolean(const CHAR* pString, BOOLEAN* pBoolean);
UINT32 ossHash ( const CHAR *data, INT32 len ) ;
UINT32 ossHashFileName ( const CHAR *fileName ) ;

/*
 * Full-precision double to string via Ryu shortest round-trip, then
 * reformatted with %g-like rules: use fixed decimal when
 * -4 <= exp < significant_digits; otherwise keep scientific notation
 * (lowercase 'e', signed exponent, at least 2 exponent digits).
 *
 * Integers in [-2^53, 2^53] use %lld fixed form (not pure %g) so values
 * such as 1e15 become "1000000000000000.0" while 1e16 becomes "1e+16";
 * that style jump at 2^53 is intentional for readability.
 * Integer-looking results (no '.'/'e'/'E') append ".0" to match sdbexprt
 * float-literal style (e.g. 100 -> "100.0"). Callers that already append
 * ".0" when missing a marker remain correct (no double suffix).
 *
 * NaN/Inf yield an empty string with return 0; callers that need literals
 * or DB2 empty/default semantics should filter specials before calling.
 * +/-0.0 both yield "0.0" (signbit of -0.0 is dropped by design).
 * On failure (return < 0), buf contents are undefined.
 */
INT32 ossDoubleToFullPrecisionString( FLOAT64 val, CHAR *buf, INT32 bufSize ) ;

// Wrapper of mktime. Supports automatic DST(daylight saving time) corrention.
time_t ossMkTime ( struct tm *timePtr ) ;

#if defined (_WINDOWS)
INT32 ossWC2ANSI ( LPCWSTR lpcszWCString,
                   LPSTR   *lppszString,
                   DWORD   *lpdwString ) ;

INT32 ossANSI2WC ( LPCSTR lpcszString,
                   LPWSTR *lppszWCString,
                   DWORD  *lpdwWCString ) ;
#elif defined (_LINUX) || defined ( _AIX )
void ossCloseAllOpenFileHandles ( BOOLEAN closeSTD ) ;
void ossCloseStdFds() ;
#endif
#define OSS_BIT_SET(x,y)     ((x) |= (y))
#define OSS_BIT_CLEAR(x,y)   ((x) &= ~(y))
#define OSS_BIT_TEST(x,y)    ((x) & (y))
#if defined (_LINUX) || defined ( _AIX )
#define ossKill(x,y) kill((x),(y))
#define ossPThreadKill(x,y) pthread_kill((x),(y))
#define ossPThreadSelf() pthread_self()
#endif
SDB_EXTERN_C_END
#endif  //OSSUTIL_H_
