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
#define ossIsspace(c) isspace(c)

#define ossItoa(x,y,z) if (y) { ossSnprintf(y, z, "%d", (INT32)(x) );}
#define ossLltoa(x,y,z) if (y) { ossSnprintf(y, z, OSS_LL_PRINT_FORMAT, (INT64)(x) );}

INT32 ossDup2( int oldFd, int newFd ) ;

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
BOOLEAN ossIsUTF8 ( CHAR *pzInfo ) ;
INT32 ossStrncasecmp ( const CHAR *pString1, const CHAR *pString2,
                       size_t iLength) ;
CHAR *ossStrnchr(const CHAR *pString, UINT32 c, UINT32 n) ;
size_t ossVsnprintf (CHAR * buf, size_t size, const CHAR * fmt, va_list ap);
void ossStrToBoolean(const CHAR* pString, BOOLEAN* pBoolean);
UINT32 ossHash ( const CHAR *data, INT32 len ) ;
UINT32 ossHashFileName ( const CHAR *fileName ) ;
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
