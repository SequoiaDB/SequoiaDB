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

   Source File Name = oss.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_H_
#define OSS_H_

#include "core.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

#if defined (_LINUX) || defined ( _AIX )
#include <errno.h>
#include <unistd.h>
#endif

#define ISEMPTYSTRING(x) (strlen(x)==0)
#define OSS_MAX_GROUPNAME_SIZE      127

#if defined (_LINUX) || defined ( _AIX )
#define OSS_FILE_SEP       "/"
#define OSS_FILE_SEP_CHAR  '/'
#define OSS_MAX_PATHSIZE   PATH_MAX
#elif defined (_WINDOWS)
#define OSS_FILE_SEP       "\\"
#define OSS_FILE_SEP_CHAR  '\\'
#define OSS_MAX_PATHSIZE   _MAX_PATH
#endif

#define OSS_DFT_SVCPORT    (11810)
#define OSS_PROCESS_NAME_LEN          255
#define OSS_RENAME_PROCESS_BUFFER_LEN OSS_PROCESS_NAME_LEN

#if defined (_LINUX) || defined (_AIX)
#define OSS_FILE_DIRECT_IO_ALIGNMENT 512
#elif defined (_WINDOWS)
#define OSS_FILE_DIRECT_IO_ALIGNMENT 512
#else
#define OSS_FILE_DIRECT_IO_ALIGNMENT 512
#endif

enum OSS_MATCH_TYPE
{
   OSS_MATCH_LEFT,
   OSS_MATCH_MID,
   OSS_MATCH_RIGHT,
   OSS_MATCH_ALL,
   OSS_MATCH_NULL
} ;

SDB_EXTERN_C_START
#if defined (_WINDOWS)
   #if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000
   #else
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000LL
   #endif //(_MSC_VER) || defined(_MSC_EXTENSIONS)

struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

OSSPID ossGetParentProcessID();
OSSPID ossGetCurrentProcessID();
OSSTID ossGetCurrentThreadID();
UINT32 ossGetLastError();
void ossSleep(UINT32 milliseconds);
void ossPanic () ;

#if defined (_WINDOWS)
#include <windows.h>
#define ossMutex                  CRITICAL_SECTION
#define ossMutexInit              InitializeCriticalSection
#define ossMutexLock              EnterCriticalSection
#define ossMutexUnlock            LeaveCriticalSection
#define ossMutexDestroy           DeleteCriticalSection

typedef long ossOnce ;
#define OSS_ONCE_INIT 0

#else /* Posix */
#include <pthread.h>
#define ossMutex                  pthread_mutex_t
#define ossMutexInit(_n)          pthread_mutex_init((_n), NULL)
#define ossMutexLock              pthread_mutex_lock
#define ossMutexUnlock            pthread_mutex_unlock
#define ossMutexDestroy           pthread_mutex_destroy

typedef pthread_once_t ossOnce;

#define OSS_ONCE_INIT PTHREAD_ONCE_INIT

#endif

INT32 ossOnceRun(ossOnce* control, void (*func)(void));

SDB_EXTERN_C_END
#endif // OSS_H_

