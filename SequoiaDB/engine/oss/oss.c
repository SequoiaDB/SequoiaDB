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

   Source File Name = oss.c

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include <time.h>
#include "oss.h"

#if defined (_LINUX)
#include <unistd.h>
#include <syscall.h>
#elif defined (_AIX)
#include <pthread.h>
#else
#include <tlhelp32.h>
#endif

#if defined (_WINDOWS)
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   FILETIME ft;
   UINT64 tmpres = 0;
   static int tzflag;

   if (NULL != tv)
   {
      GetSystemTimeAsFileTime(&ft);

      tmpres |= ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*convert into microseconds*/
      tmpres /= 10;

      /*converting file time to unix epoch*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;

      tv->tv_sec = (long)(tmpres / 1000000UL);
      tv->tv_usec = (long)(tmpres % 1000000UL);
   }

   if (NULL != tz)
   {
   	  long ttz;
      int tdl;
      if (!tzflag)
      {
         _tzset();
         tzflag++;
      }
      _get_timezone(&ttz);
      _get_daylight(&tdl);
      tz->tz_minuteswest = ttz / 60;
      tz->tz_dsttime = tdl;
   }

   return 0;
}

#endif
OSSPID ossGetCurrentProcessID()
{
#if defined (_WINDOWS)
   return GetCurrentProcessId();
#else
   return getpid();
#endif
}
OSSTID ossGetCurrentThreadID()
{
#if defined (_WINDOWS)
   return GetCurrentThreadId();
#elif defined (_AIX)
   return pthread_self();
#else
   return syscall(SYS_gettid);
#endif
}

UINT32 ossGetLastError()
{
#if defined (_WINDOWS)
   return GetLastError();
#else
   return errno;
#endif
}

const CHAR* ossGetLastErrorMsg( UINT32 sysErrno )
{
   const static CHAR* errMsgs[] =
   {
#if defined (_WINDOWS)
      "No error",
      "Operation not permitted",
      "No such file or directory",
      "No such process",
      "Interrupted function call",
      "Input/output error",
      "No such device or address",
      "Arg list too long",
      "Exec format error",
      "Bad file descriptor",
      "No child processes",
      "Resource temporarily unavailable",
      "Not enough space",
      "Permission denied",
      "Bad address",
      "Unknown error",
      "Resource device",
      "File exists",
      "Improper link",
      "No such device",
      "Not a directory",
      "Is a directory",
      "Invalid argument",
      "Too many open files in system",
      "Too many open files",
      "Inappropriate I/O control operation",
      "Unknown error",
      "File too large",
      "No space left on device",
      "Invalid seek",
      "Read-only file system",
      "Too many links",
      "Broken pipe",
      "Domain error",
      "Result too large",
      "Unknown error",
      "Resource deadlock avoided",
      "Unknown error",
      "Filename too long",
      "No locks available",
      "Function not implemented",
      "Directory not empty",
      "Illegal byte sequence"
#else
      "Success",
      "Operation not permitted",
      "No such file or directory",
      "No such process",
      "Interrupted system call",
      "Input/output error",
      "No such device or address",
      "Argument list too long",
      "Exec format error",
      "Bad file descriptor",
      "No child processes",
      "Resource temporarily unavailable",
      "Cannot allocate memory",
      "Permission denied",
      "Bad address",
      "Block device required",
      "Device or resource busy",
      "File exists",
      "Invalid cross-device link",
      "No such device",
      "Not a directory",
      "Is a directory",
      "Invalid argument",
      "Too many open files in system",
      "Too many open files",
      "Inappropriate ioctl for device",
      "Text file busy",
      "File too large",
      "No space left on device",
      "Illegal seek",
      "Read-only file system",
      "Too many links",
      "Broken pipe",
      "Numerical argument out of domain",
      "Numerical result out of range",
      "Resource deadlock avoided",
      "File name too long",
      "No locks available",
      "Function not implemented",
      "Directory not empty",
      "Too many levels of symbolic links",
      "Unknown error",
      "No message of desired type",
      "Identifier removed",
      "Channel number out of range",
      "Level 2 not synchronized",
      "Level 3 halted",
      "Level 3 reset",
      "Link number out of range",
      "Protocol driver not attached",
      "No CSI structure available",
      "Level 2 halted",
      "Invalid exchange",
      "Invalid request descriptor",
      "Exchange full",
      "No anode",
      "Invalid request code",
      "Invalid slot",
      "Unknown error",
      "Bad font file format",
      "Device not a stream",
      "No data available",
      "Timer expired",
      "Out of streams resources",
      "Machine is not on the network",
      "Package not installed",
      "Object is remote",
      "Link has been severed",
      "Advertise error",
      "Srmount error",
      "Communication error on send",
      "Protocol error",
      "Multihop attempted",
      "RFS specific error",
      "Bad message",
      "Value too large for defined data type",
      "Name not unique on network",
      "File descriptor in bad state",
      "Remote address changed",
      "Can not access a needed shared library",
      "Accessing a corrupted shared library",
      ".lib section in a.out corrupted",
      "Attempting to link in too many shared libraries",
      "Cannot exec a shared library directly",
      "Invalid or incomplete multibyte or wide character",
      "Interrupted system call should be restarted",
      "Streams pipe error",
      "Too many users",
      "Socket operation on non-socket",
      "Destination address required",
      "Message too long",
      "Protocol wrong type for socket",
      "Protocol not available",
      "Protocol not supported",
      "Socket type not supported",
      "Operation not supported",
      "Protocol family not supported",
      "Address family not supported by protocol",
      "Address already in use",
      "Cannot assign requested address",
      "Network is down",
      "Network is unreachable",
      "Network dropped connection on reset",
      "Software caused connection abort",
      "Connection reset by peer",
      "No buffer space available",
      "Transport endpoint is already connected",
      "Transport endpoint is not connected",
      "Cannot send after transport endpoint shutdown",
      "Too many references: cannot splice",
      "Connection timed out",
      "Connection refused",
      "Host is down",
      "No route to host",
      "Operation already in progress",
      "Operation now in progress",
      "Stale file handle",
      "Structure needs cleaning",
      "Not a XENIX named type file",
      "No XENIX semaphores available",
      "Is a named type file",
      "Remote I/O error",
      "Disk quota exceeded",
      "No medium found",
      "Wrong medium type",
      "Operation canceled",
      "Required key not available",
      "Key has expired",
      "Key has been revoked",
      "Key was rejected by service",
      "Owner died",
      "State not recoverable",
      "Operation not possible due to RF-kill",
      "Memory page has hardware error"
#endif
   } ;

   if ( sysErrno < 0 || sysErrno >= ( sizeof ( errMsgs ) / sizeof ( CHAR* ) ) )
   {
      return "unknown error" ;
   }

   return errMsgs[sysErrno] ;
}


void ossSleep(UINT32 milliseconds)
{
#if defined (_WINDOWS)
   Sleep(milliseconds);
#else
   usleep(milliseconds*1000);
#endif
}

void ossPanic()
{
   INT32 *p = NULL ;
   *p = 10 ;
}

/// in most cases, just need to use pmdGetSysPageSize()
INT64  ossGetPageSize()
{
   INT64 pagesize = 0;
#if defined (_WINDOWS)
   SYSTEM_INFO si;
   GetSystemInfo(&si);
   pagesize = si.dwPageSize;
#else /* Posix */
#if defined (_SC_PAGESIZE)
   pagesize = sysconf(_SC_PAGESIZE);
#elif defined (_SC_PAGE_SIZE)
   pagesize = sysconf(_SC_PAGE_SIZE);
#else
   pagesize = (INT64)getpagesize();
#endif
#endif // _WINDOWS
   return pagesize;
}

OSSPID ossGetParentProcessID()
{
#if defined (_WINDOWS)
   OSSPID         pid         = OSS_INVALID_PID ;
   OSSPID         ppid        = OSS_INVALID_PID ;
   HANDLE         hSnapshot   = INVALID_HANDLE_VALUE ;
   PROCESSENTRY32 pe          = { 0 } ;
   pe.dwSize                  = sizeof (PROCESSENTRY32) ;

   hSnapshot = CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS , 0 ) ;
   if ( hSnapshot == INVALID_HANDLE_VALUE )
      goto error ;

   pid = GetCurrentProcessId() ;

   if ( Process32First ( hSnapshot , &pe ) )
   {
      do
      {
         if ( pe.th32ProcessID == pid )
         {
            ppid = pe.th32ParentProcessID ;
            goto done ;
         }
      } while ( Process32Next ( hSnapshot , &pe ) ) ;
   }
done :
   return ppid ;
error :
   goto done ;

#else
   return getppid() ;
#endif
}

INT32 ossOnceRun(ossOnce* control, void (*func)(void))
{
#ifdef _WINDOWS
   if ( !InterlockedExchange ( control, 1 ) )
   {
      func() ;
   }

   return 0 ;

#else /* Posix */
   return pthread_once ( control, func ) ;
#endif
}

