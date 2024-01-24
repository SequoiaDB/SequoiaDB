/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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
#include "ossUtil.c"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined (_LINUX) || defined (_AIX)
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mntent.h>
#elif defined (_WINDOWS)
#include "Psapi.h"
#endif
#include <sstream>

// Wrapper of localtime, convert a time value and correct for the local time
// zone. The input pTime represents the seconds elapsed since the Epoch,
// midnight (00:00:00), January 1, 1970, UTC
void ossLocalTime ( time_t &Time, struct tm &TM )
{
#if defined (_LINUX ) || defined (_AIX)
   localtime_r( &Time, &TM ) ;
#elif defined (_WINDOWS)
   // The Time represents the seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC. This value is usually obtained from the time
   // function.
   localtime_s( &TM, &Time ) ;
#endif
}

// When a double value is out of the range of type unsigned long, the result of
// converting it into a unsigned long value is different on x86 from arm. So when
// it is out of range we handle the result as x86 did.
UINT64 ossDoubleToUINT64( FLOAT64 num )
{
   // When double is out of the range of unsigned long,it has four cases.
   // case1: when double is special value ,in NaN and -INF case we give it an
   //        "indefinite integer value",in +INF case we give it 0
   // case2: when double is left overflow than the min value long type can
   //        represent,we give it an "indefinite integer value"
   // case3: when double is smaller than zero we mod it with 2^64, which is
   //        equivalent to ( num + 2^64 ), but the max value UINT64 can represent
   //        is (2^64-1),so here we handle it as (OSS_UINT64_MAX - abs(num) + 1 )
   // case4: when double if right overflow than the max value unsigned long type
   //        can represent,we give it an 0
   // otherwise: we do nothing
   UINT64 ans = 0 ;
   INT32 sign = 0 ;

   if ( ossIsNaN( num ) )
   {
      ans = OSS_INDEF_VAL_64 ;
   }
   else if ( ossIsInf( num, &sign ) )
   {
      if ( sign == -1 )
      {
         ans = OSS_INDEF_VAL_64 ;
      }
      else
      {
         ans = 0 ;
      }
   }
   else if ( num <= OSS_SINT64_MIN_D )
   {
      ans = OSS_INDEF_VAL_64 ;
   }
   else if ( num < 0 )
   {
      ans = UINT64( OSS_UINT64_MAX - UINT64( -num ) + 1 ) ;
   }
   else if ( num >= OSS_UINT64_MAX )
   {
      ans = 0 ;
   }
   else
   {
      ans = num ;
   }

   return ans ;
}

UINT32 ossDoubleToUINT32( FLOAT64 num )
{
   // When double is out of the range of unsigned int,it has five cases.
   // case1: when double is special value  NaN and -INF and +INF we give it 0
   // case2: when double is left overflow than the min value long type can
   //        represent,we give it an 0
   // case3: when double is smaller than zero we mod it with 2^32
   // case4: when double is right overflow than the max value long type can
   //        represent,we give it an 0
   // case5: when double if right overflow than the max value unsigned int type
   //        can represent,we mod it with 2^32
   // otherwise: we do nothing
   UINT32 ans = 0 ;
   if ( ossIsNaN( num ) || ossIsInf( num ) )
   {
      ans = 0 ;
   }
   else if ( num <= OSS_SINT64_MIN_D )
   {
      ans = 0 ;
   }
   else if ( num < 0 )
   {
      ans = INT64( num ) % OSS_SINT64_2_32 ;
   }
   else if ( num >= OSS_SINT64_MAX_D )
   {
      ans = 0 ;
   }
   else if ( num > OSS_UINT32_MAX )
   {
      ans = INT64( num ) % OSS_SINT64_2_32 ;
   }
   else
   {
      ans = num ;
   }

   return ans ;
}

BOOLEAN ossIsPowerOf2( UINT32 num, UINT32 * pSquare )
{
   BOOLEAN bPowered = ( ( 0 != num ) && ( 0 == ( num & ( num -1 ) ) ) ) ;
   if ( bPowered && pSquare )
   {
      *pSquare = 0 ;
      while ( 1 != num  )
      {
         num = num >> 1 ;
         ++(*pSquare) ;
      }
   }
   return bPowered ;
}

UINT64 ossNextPowerOf2( UINT32 num, UINT32 *pSquare )
{
   UINT32 square = 0 ;
   UINT64 result = 1 ;

   while ( num > result )
   {
      square++ ;
      result <<= 1 ;
   }
   if ( pSquare )
   {
      *pSquare = square ;
   }
   return result ;
}

//  Wrapper of gmtime, converts a time value to a broken-down time structure.
//  The Time is represented as seconds elapsed since the Epoch.
void ossGmtime ( time_t &Time, struct tm &TM )
{
#if defined (_LINUX ) || defined (_AIX)
   gmtime_r( &Time, &TM ) ;
#elif defined (_WINDOWS)
   // Time is represented as seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC.
   // The TM of the returned structure hold the evaluated value of the
   // timer argument in UTC rather than in local time.
   gmtime_s( &TM, &Time ) ;
#endif
}

// convert ossTimestamp to string
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSTS2STR, "ossTimestampToString" )
void ossTimestampToString( ossTimestamp &Tm, CHAR * pStr )
{
   PD_TRACE_ENTRY ( SDB_OSSTS2STR );
   // The following format is used for timestamp:
   //   yyyy-mm-dd-hh.mm.ss.uuuuuu
   CHAR szFormat[] = "%04d-%02d-%02d-%02d.%02d.%02d.%06d" ;
   CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
   struct tm tmpTm ;

   if ( pStr )
   {
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    Tm.microtm ) ;
      ossStrncpy( pStr, szTimestmpStr, ossStrlen( szTimestmpStr ) + 1 ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSTS2STR );
}

void ossTimestampToUTCString( ossTimestamp &Tm, CHAR * pStr )
{
   CHAR szFormat[] = "%04d-%02d-%02d-%02d.%02d.%02d.%06d" ;
   CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
   struct tm tmpTm ;

   if ( pStr )
   {
      ossGmtime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    Tm.microtm ) ;
      ossStrncpy( pStr, szTimestmpStr, ossStrlen( szTimestmpStr ) + 1 ) ;
   }
}

UINT64 ossTimestampToMilliseconds( const ossTimestamp &timestamp )
{
   return ( (UINT64)timestamp.time ) * 1000L + timestamp.microtm / 1000L ;
}

void ossMillisecondsToString( UINT64 milliseconds, CHAR *pStr )
{
   ossTimestamp tm( milliseconds ) ;
   ossTimestampToString( tm, pStr ) ;
}

UINT64 ossStringToMilliseconds( const CHAR *pStr )
{
   ossTimestamp tm ;
   ossStringToTimestamp( pStr, tm ) ;
   return ossTimestampToMilliseconds( tm ) ;
}

// convert time_t from local to UTC in the same DateString
// for example:
//   [in] local timezone:CST, date:"2019-08-06 20:13:54", local:1565093634
//   [out] utc  timezone:UTC, date:"2019-08-06 20:13:54", utc:  1565122434
// quick check:
// 1. echo "Asia/Shanghai" > /etc/timezone && cat /usr/share/zoneinfo/Asia/Shanghai > /etc/localtime
// 2. date -d@1565093634  ==> Tue Aug  6 20:13:54 CST 2019
// 3. echo "UTC" > /etc/timezone && cat /usr/share/zoneinfo/UTC > /etc/localtime
// 4. date -d@1565122434  ==> Tue Aug  6 20:13:54 UTC 2019
void ossTimeLocalToUTCInSameDate( const time_t &local, time_t &utc )
{
   utc = local + ossTimeDiffWithUTC() ;
}

time_t ossTimeDiffWithUTC()
{
   static time_t time = 1565084755 ;
   struct tm local ;
   struct tm utc ;
   ossLocalTime( time, local ) ;
   ossGmtime( time, utc ) ;
   return ossMkTime( &local ) - ossMkTime( &utc ) ;
}

INT32 ossTimeGetMaxDay( INT32 year, INT32 month )
{
   static INT32 day[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} ;
   if ( 2 == month )
   {
      if ( (0 == year%4 && 0 != year%100) || 0 == year%400 )
      {
         return 29 ;
      }
      else
      {
         return 28 ;
      }
   }

   if ( month > 12 )
   {
      month = 12 ;
   }

   return day[month -1] ;
}

void ossStructTMToString( struct tm &tm, CHAR *pStr )
{
   if ( pStr )
   {
      CHAR szFormat[] = "%04d-%02d-%02d-%02d.%02d.%02d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec ) ;
      ossStrncpy( pStr, szTimestmpStr, ossStrlen( szTimestmpStr ) + 1 ) ;
   }
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_STR2OSSTS, "ossStringToTimestamp" )
void ossStringToTimestamp( const CHAR * pStr, ossTimestamp &Tm )
{
   PD_TRACE_ENTRY ( SDB_STR2OSSTS );
   struct tm tmp = { 0 } ;
   CHAR format[] = "%04d-%02d-%02d-%02d.%02d.%02d.%06d" ;

   Tm.microtm = 0 ;
   ossSscanf( pStr, format, &tmp.tm_year, &tmp.tm_mon, &tmp.tm_mday,
              &tmp.tm_hour, &tmp.tm_min, &tmp.tm_sec, &Tm.microtm ) ;
   tmp.tm_year -= 1900 ;
   tmp.tm_mon  -= 1 ;

   Tm.time = ossMkTime( &tmp ) ;

   PD_TRACE_EXIT ( SDB_STR2OSSTS );
}

void ossStringToTimestamp( const CHAR *pStr, ossTimestamp &Tm,
                           INT32 &parseNum )
{
   struct tm tmp = { 0 } ;
   CHAR format[] = "%04d-%02d-%02d-%02d.%02d.%02d.%06d" ;

   parseNum = 0 ;
   Tm.microtm = 0 ;
   parseNum = ossSscanf( pStr, format, &tmp.tm_year, &tmp.tm_mon, &tmp.tm_mday,
                         &tmp.tm_hour, &tmp.tm_min, &tmp.tm_sec, &Tm.microtm ) ;
   tmp.tm_year -= 1900 ;
   tmp.tm_mon  -= 1 ;

   Tm.time = ossMkTime( &tmp ) ;
}


void ossGetCurrentTime( ossTimestamp &TM )
{
#if defined (_LINUX) || defined (_AIX)
   struct timeval tv ;

   // obtain the current time, expressed as seconds and microseconds since
   // the Epoch
   if ( -1 == gettimeofday( &tv, NULL ) )
   {
       TM.time    = 0 ;
       TM.microtm = 0 ;
   }
   else
   {
       TM.time    = tv.tv_sec ;
       TM.microtm = tv.tv_usec ;
   }
#elif defined (_WINDOWS)
   FILETIME       fileTime ;
   ULARGE_INTEGER uLargeIntegerTime ;

   GetSystemTimeAsFileTime( &fileTime ) ;

   //convert FILETIME into ULARGER_INTEGER
   uLargeIntegerTime.LowPart  = fileTime.dwLowDateTime ;
   uLargeIntegerTime.HighPart = fileTime.dwHighDateTime ;

   // FILETIME contains a 64-bit value representing the number of
   // 100-nanosecond intervals since January 1, 1601 (UTC).
   uLargeIntegerTime.QuadPart -= ( DELTA_EPOCH_IN_MICROSECS * 10 ) ;

   // 1 FILETIME = 100 ns
   TM.time    = ( uLargeIntegerTime.QuadPart / OSS_TEN_MILLION ) ;
   TM.microtm = ( uLargeIntegerTime.QuadPart % OSS_TEN_MILLION ) / 10 ;
#endif
}

UINT64 ossGetCurrentMicroseconds()
{
   ossTimestamp current ;
   ossGetCurrentTime( current ) ;

   return ( (UINT64)current.time ) * 1000000L + current.microtm ;
}

UINT64 ossTimestampToMicroseconds( const ossTimestamp &timestamp )
{
   return ( (UINT64)timestamp.time ) * 1000000L + timestamp.microtm ;
}

ossTimestamp ossMicrosecondsToTimestamp( const UINT64 &microseconds )
{
   ossTimestamp timestamp ;
   timestamp.time = microseconds / 1000000L ;
   timestamp.microtm = microseconds % 1000000L ;

   return timestamp ;
}

UINT64 ossGetCurrentMilliseconds()
{
   return ossGetCurrentMicroseconds() / 1000L ;
}

// Get CPU usage for current process
#define OSS_PROC_FIELD_TO_SKIP_FOR_UTIME 13
#define OSS_PROC_PATH_LEN_MAX 255
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETCPUUSG, "ossGetCPUUsage" )
SINT32 ossGetCPUUsage
(
   ossTime &usrTime,
   ossTime &sysTime
)
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETCPUUSG );
#if defined (_WINDOWS)
   FILETIME creationTime, exitTime, kernelTime, userTime ;
   ULARGE_INTEGER uTemp, sTemp ;
   DWORD ossErr = 0 ;

   if ( GetProcessTimes( GetCurrentProcess(),
                         &creationTime, &exitTime, &kernelTime, &userTime ) )
   {
      uTemp.LowPart  = userTime.dwLowDateTime ;
      uTemp.HighPart = userTime.dwHighDateTime ;
      sTemp.LowPart  = kernelTime.dwLowDateTime ;
      sTemp.HighPart = kernelTime.dwHighDateTime ;

      // 1 FILETIME = 100 ns
      usrTime.seconds  = (UINT32)( uTemp.QuadPart / OSS_TEN_MILLION ) ;
      usrTime.microsec = (UINT32)((uTemp.QuadPart % OSS_TEN_MILLION ) / 10) ;
      sysTime.seconds  = (UINT32)( sTemp.QuadPart / OSS_TEN_MILLION ) ;
      sysTime.microsec = (UINT32)((sTemp.QuadPart % OSS_TEN_MILLION ) / 10) ;
   }
   else
   {
      ossErr = GetLastError() ;
      rc = SDB_SYS ;
   }
#elif defined (_LINUX) || defined (_AIX)

   #if ( defined __GLIBC__ && ( __GLIBC__ >= 2 ) && ( __GLIBC_MINOR__ >= 2 ) )
      #define OSS_CLK_TCK CLOCKS_PER_SEC
   #else
      #define OSS_CLK_TCK CLK_TCK
   #endif

   SINT32 ossErr = 0 ;
   CHAR   pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = { 0 } ;
   UINT32 cntr = 0 ;
   INT32 tmpChr = 0 ;
   UINT32 uTime = 0 ;
   UINT32 sTime = 0 ;
   FILE  *fp = NULL ;
   SINT32 numScanned = 0 ;

   static int clkTck = 0 ;
   static int numMicrosecPerClkTck  = 0 ;

   // On Linux, utime and stime use "jiffies" as unit of measurement.
   // Normally there are 100 jiffies per seconds.
   if ( 0 == clkTck )
   {
      clkTck = sysconf( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }

      // in time.h, CLOCKS_PER_SEC is required to be 1 million on all
      // XSI-conformant systems
      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }

   // read /proc/pid//stat can get both user and system times
   // however it is relatively expensive, it calls fopen, fgets, fscanf.
   // Thus, this function should not be called in a signal handler,
   // since these functions are not aync-signal-safe.
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/stat",
                getpid() ) ;

   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
      // skip first 13 fields, since the utime and stime are the 14th and
      // 15th fields in /proc/pid/task/tid/stat
      while (  ( cntr < OSS_PROC_FIELD_TO_SKIP_FOR_UTIME ) &&
               ( EOF != ( tmpChr = fgetc(fp) ) ) )
      {
         if ( ' ' == tmpChr )
         {
            cntr++ ;
         }
      }
      if ( OSS_PROC_FIELD_TO_SKIP_FOR_UTIME == cntr )
      {
         numScanned = fscanf (fp, "%u%u", &uTime, &sTime) ;
         if ( 2 == numScanned )
         {
            usrTime.seconds = uTime / clkTck ;
            usrTime.microsec = ( uTime % clkTck ) * numMicrosecPerClkTck ;
            sysTime.seconds = sTime / clkTck ;
            sysTime.microsec = (sTime % clkTck) * numMicrosecPerClkTck ;
         }
         else
         {
            rc = SDB_SYS ;
            ossErr = errno ;
         }
      }
         fclose( fp ) ;
   }
   else
   {
      ossErr = errno ;
      rc = SDB_SYS ;
   }
#endif
   if ( ossErr )
      PD_LOG ( PDERROR, "ossErr = %d", ossErr ) ;
   PD_TRACE_EXITRC ( SDB_OSSGETCPUUSG, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETCPUUSG2, "ossGetCPUUsage" )
SINT32 ossGetCPUUsage
(
#if defined (_WINDOWS)
   HANDLE tHandle,  // thread handle, e.g., GetCurrthenThread()
#elif defined (_LINUX) || defined (_AIX)
   OSSTID tid,      // lwp / kernel thread id, ossGetCurrentThreadID()
#endif
   ossTime &usrTime,
   ossTime &sysTime
)
{
   SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_OSSGETCPUUSG2 );
#if defined (_WINDOWS)
   FILETIME creationTime, exitTime, kernelTime, userTime ;
   ULARGE_INTEGER uTemp, sTemp ;
   DWORD ossErr = 0 ;

   if ( GetThreadTimes( tHandle,
                        &creationTime, &exitTime, &kernelTime, &userTime ) )
   {
      uTemp.LowPart  = userTime.dwLowDateTime ;
      uTemp.HighPart = userTime.dwHighDateTime ;
      sTemp.LowPart  = kernelTime.dwLowDateTime ;
      sTemp.HighPart = kernelTime.dwHighDateTime ;

      // 1 FILETIME = 100 ns
      usrTime.seconds  = (UINT32)( uTemp.QuadPart / OSS_TEN_MILLION ) ;
      usrTime.microsec = (UINT32)((uTemp.QuadPart % OSS_TEN_MILLION ) / 10) ;
      sysTime.seconds  = (UINT32)( sTemp.QuadPart / OSS_TEN_MILLION ) ;
      sysTime.microsec = (UINT32)((sTemp.QuadPart % OSS_TEN_MILLION ) / 10) ;
   }
   else
   {
      ossErr = GetLastError() ;
      rc = SDB_SYS ;
   }
#elif defined (_LINUX) || defined (_AIX)

   #if ( defined __GLIBC__ && ( __GLIBC__ >= 2 ) && ( __GLIBC_MINOR__ >= 2 ) )
      #define OSS_CLK_TCK CLOCKS_PER_SEC
   #else
      #define OSS_CLK_TCK CLK_TCK
   #endif

   SINT32 ossErr = 0 ;
   CHAR   pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = { 0 } ;
   UINT32 cntr = 0 ;
   INT32 tmpChr = 0 ;
   UINT32 uTime = 0 ;
   UINT32 sTime = 0 ;
   FILE  *fp = NULL ;
   SINT32 numScanned = 0 ;

   static int clkTck = 0 ;
   static int numMicrosecPerClkTck  = 0 ;

   // On Linux, utime and stime use "jiffies" as unit of measurement.
   // Normally there are 100 jiffies per seconds.
   if ( 0 == clkTck )
   {
      clkTck = sysconf( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }

      // in time.h, CLOCKS_PER_SEC is required to be 1 million on all
      // XSI-conformant systems
      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }

   // read /proc/pid/task/tid/stat can get both user and system times
   // however it is relatively expensive, it calls fopen, fgets, fscanf.
   // Thus, this function should not be called in a signal handler,
   // since these functions are not aync-signal-safe.
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/task/%lu/stat",
                getpid(),
               (unsigned long)tid ) ;

   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
      // skip first 13 fields, since the utime and stime are the 14th and
      // 15th fields in /proc/pid/task/tid/stat
      while (  ( cntr < OSS_PROC_FIELD_TO_SKIP_FOR_UTIME ) &&
               ( EOF != ( tmpChr = fgetc(fp) ) ) )
      {
         if ( ' ' == tmpChr )
         {
            cntr++ ;
         }
      }
      if ( OSS_PROC_FIELD_TO_SKIP_FOR_UTIME == cntr )
      {
         numScanned = fscanf (fp, "%u%u", &uTime, &sTime) ;
         if ( 2 == numScanned )
         {
            usrTime.seconds = uTime / clkTck ;
            usrTime.microsec = ( uTime % clkTck ) * numMicrosecPerClkTck ;
            sysTime.seconds = sTime / clkTck ;
            sysTime.microsec = (sTime % clkTck) * numMicrosecPerClkTck ;
         }
         else
         {
            rc = SDB_SYS ;
            ossErr = errno ;
         }
      }
         fclose( fp ) ;
   }
   else
   {
      ossErr = errno ;
      rc = SDB_SYS ;
   }
#endif
   if ( ossErr )
   {
#if defined (_WINDOWS)
      PD_LOG ( PDWARNING, "ossErr = %d, thread id = %u", ossErr, tHandle ) ;
#elif defined (_LINUX) || defined (_AIX)
      PD_LOG ( PDWARNING, "ossErr = %d, thread id = %u", ossErr, tid ) ;
#endif
   }
   PD_TRACE_EXITRC ( SDB_OSSGETCPUUSG2, rc );
   return rc ;
}

INT32 ossGetOSInfo( ossOSInfo &info )
{
   info._desp[ 0 ] = 0 ;
   info._distributor[ 0 ] = 0 ;
   info._release[ 0 ] = 0 ;
   CHAR arch[ 31 ] = { 0 } ;

#if defined( _WINDOWS )
   SYSTEM_INFO sysInfo = { 0 } ;
   OSVERSIONINFOEX OSVerInfo={ 0 } ;

   OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   GetVersionEx ( (OSVERSIONINFO*) &OSVerInfo ) ;
   if ( OSVerInfo.dwMajorVersion == 6 )
   {
      if ( OSVerInfo.dwMinorVersion == 0 )
      {
         if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
         {
            ossStrcpy( info._distributor, "Windows Vista" ) ;
         }
         else
         {
            ossStrcpy( info._distributor, "Windows Server 2008" ) ;
         }
      }
      if ( OSVerInfo.dwMinorVersion == 1 )
      {
         if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
         {
            ossStrcpy( info._distributor, "Windows 7" ) ;
         }
         else
         {
            ossStrcpy( info._distributor, "Windows Server 2008" ) ;
         }
      }
      if ( OSVerInfo.dwMinorVersion == 2 )
      {
         if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
         {
            ossStrcpy( info._distributor, "Windows 8 or later" ) ;
         }
         else
         {
            ossStrcpy( info._distributor, "Windows Server 2012 or later" ) ;
         }
      }
   }
   if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 2 )
   {
      if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
      {
         ossStrcpy( info._distributor, "Windows XP" ) ;
      }
      else
      {
         ossStrcpy( info._distributor, "Windows Server 2003" ) ;
      }
   }
   if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 1 )
   {
      ossStrcpy( info._distributor, "Windows XP" ) ;
   }
   if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 0 )
   {
      ossStrcpy( info._distributor, "Windows 2000" ) ;
   }

   ossSnprintf( info._release, sizeof( info._release ) - 1,
                "%s%d.%d Build:%d",
                OSVerInfo.szCSDVersion, OSVerInfo.dwMajorVersion,
                OSVerInfo.dwMinorVersion, OSVerInfo.dwBuildNumber ) ;

   GetSystemInfo( &sysInfo ) ;
   switch( sysInfo.wProcessorArchitecture )
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
           ossStrncpy( arch, "Intel x86", sizeof( arch ) - 1 ) ;
           info._bit = 32 ;
           break ;
      case PROCESSOR_ARCHITECTURE_IA64:
           ossStrncpy( arch, "Intel IA64", sizeof( arch ) - 1 ) ;
           info._bit = 64 ;
           break ;
      case PROCESSOR_ARCHITECTURE_AMD64:
           ossStrncpy( arch, "AMD 64", sizeof( arch ) - 1 ) ;
           info._bit = 64 ;
           break ;
      default:
           ossStrncpy( arch, "Unknown", sizeof( arch ) - 1 ) ;
           break ;
   }
#else
   struct utsname name ;

   if ( -1 == uname( &name ) )
   {
      memset( &name, 0, sizeof( name ) ) ;
   }

   ossSnprintf( info._distributor, sizeof( info._distributor ) - 1,
                "%s", name.sysname ) ;
   ossSnprintf( info._release, sizeof( info._release ) - 1,
                "%s", name.release ) ;
   ossSnprintf( arch, sizeof( arch ) - 1, "%s", name.machine ) ;
#if defined (_PPCLIN64) || defined (_ARMLIN64) || defined (_ALPHALIN64)
   info._bit = 64 ;
#else
   if ( 0 == ossStrcmp( arch, "x86_64" ) )
   {
      info._bit = 64 ;
   }
   else
   {
      info._bit = 32 ;
   }
#endif // _PPCLIN64
#endif // _WINDOWS
   ossSnprintf( info._desp, sizeof( info._desp ) - 1, "%s %s(%s)",
                info._distributor, info._release, arch ) ;
   return SDB_OK ;
}

/// Tick initialize
static UINT64 g_tickFactor = 1 ;
static void ossInitTickFactor()
{
   #if defined (_WINDOWS)
      LARGE_INTEGER countsPerSecond ;
      if ( ! QueryPerformanceFrequency(&countsPerSecond) )
      {
         g_tickFactor = 1 ;
      }
      g_tickFactor = countsPerSecond.QuadPart ;
   #elif defined (_LINUX) || defined (_AIX)
      g_tickFactor = 1 ;
   #endif
}

ossTickConversionFactor::ossTickConversionFactor()
{
   static ossOnce s_control = OSS_ONCE_INIT ;
   ossOnceRun( &s_control, ossInitTickFactor ) ;
   factor = g_tickFactor ;
}

/// Rand initialize
#if defined (_LINUX) || defined (_AIX)
static UINT32 g_randSeed = 0 ;
static ossMutex g_randMutex ;
#endif

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSRAND, "ossSrand" )
static void ossSrand()
{
   PD_TRACE_ENTRY ( SDB_OSSSRAND ) ;
#if defined (_WINDOWS)
      srand ( (UINT32) time ( NULL ) ) ;
#elif defined (_LINUX) || defined (_AIX)
      g_randSeed = time ( NULL ) ;
#endif
   PD_TRACE_EXIT ( SDB_OSSSRAND );
}

class ossRandAssit
{
   public:
      ossRandAssit()
      {
         ossSrand() ;
#if defined (_LINUX) || defined (_AIX)
         ossMutexInit( &g_randMutex ) ;
#endif
      }
      ~ossRandAssit()
      {
#if defined (_LINUX) || defined (_AIX)
         ossMutexDestroy( &g_randMutex ) ;
#endif
      }
      void done() {}
} ;

static ossRandAssit* _ossGetRandAssit()
{
   static ossRandAssit s_randAssit ;
   return &s_randAssit ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSRAND, "ossRand" )
UINT32 ossRand ()
{
   PD_TRACE_ENTRY ( SDB_OSSRAND );
   UINT32 randVal = 0 ;

   _ossGetRandAssit()->done() ;

#if defined (_WINDOWS)
   rand_s ( &randVal ) ;
#elif defined (_LINUX) || defined (_AIX)
   ossMutexLock( &g_randMutex ) ;
   randVal = rand_r ( &g_randSeed ) ;
   ossMutexUnlock( &g_randMutex ) ;
#endif
   PD_TRACE_EXIT ( SDB_OSSRAND );
   return randVal ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSHEXDL, "ossHexDumpLine" )
UINT32 ossHexDumpLine
(
   const void *   inPtr,
   UINT32         len,
   CHAR *         szOutBuf,
   UINT32         flags
)
{
   PD_TRACE_ENTRY ( SDB_OSSHEXDL );
   const char * cPtr = ( const char *)inPtr ;
   UINT32 curOff = 0 ;
   UINT32 bytesWritten = 0 ;
   UINT32 offInBuf = 0 ;
   UINT32 bytesRemain = OSS_HEXDUMP_LINEBUFFER_SIZE ;


   if ( inPtr && szOutBuf && ( len <= OSS_HEXDUMP_BYTES_PER_LINE ) )
   {
      bool padding = false ;

      szOutBuf[OSS_HEXDUMP_LINEBUFFER_SIZE - 1] = '\0' ;

      /* OSS_HEXDUMP_INCLUDE_ADDRESS */
      if ( flags & OSS_HEXDUMP_INCLUDE_ADDR )
      {
         offInBuf = ossSnprintf( szOutBuf, bytesRemain,
                                 "0x" OSS_PRIXPTR " : ", (UintPtr)cPtr) ;
         bytesRemain -= offInBuf ;
      }

      for ( UINT32 i = 0 ; i < len ; ++i )
      {
         bytesWritten = ossSnprintf( &szOutBuf[ offInBuf ], bytesRemain,
                                     "%02X", (unsigned char)cPtr[ i ] ) ;
         offInBuf += bytesWritten ;
         bytesRemain -= bytesWritten ;
         if ( bytesRemain )
         {
            szOutBuf[ offInBuf ] = ' ' ;
         }
         if ( padding && bytesRemain )
         {
            ++offInBuf ;
            bytesRemain -- ;
         }
         padding = ! padding ;
      }

      curOff = OSS_HEXDUMP_START_OF_DATA_DISP ;
      if ( flags & OSS_HEXDUMP_INCLUDE_ADDR )
      {
         curOff += OSS_HEXDUMP_ADDRESS_SIZE ;
      }

      if ( offInBuf < curOff )
      {
         ossMemset( &szOutBuf[ offInBuf ], ' ', ( curOff - offInBuf ) ) ;
      }

      if ( ! ( flags & OSS_HEXDUMP_RAW_HEX_ONLY ) )
      {
         for ( UINT32 i = 0 ; i < len ; i++, curOff++ )
         {
            /* Print character as is only if it is printable */
            if ( cPtr[i] >= ' ' && cPtr[i] <= '~' )
            {
               if ( curOff < OSS_HEXDUMP_LINEBUFFER_SIZE )
               {
                  szOutBuf[ curOff ] = cPtr[ i ] ;
               }
            }
            else
            {
               if ( curOff < OSS_HEXDUMP_LINEBUFFER_SIZE )
               {
                  szOutBuf[ curOff ] = '.' ;
               }
            }
         }
      }

      if ( curOff + sizeof(OSS_NEWLINE) <= OSS_HEXDUMP_LINEBUFFER_SIZE )
      {
         ossStrncpy( &szOutBuf[curOff], OSS_NEWLINE, sizeof( OSS_NEWLINE ) ) ;
         curOff += sizeof( OSS_NEWLINE ) - sizeof( '\0' ) ;
      }
      else
      {
         szOutBuf[OSS_HEXDUMP_LINEBUFFER_SIZE - 1] = '\0' ;
         curOff = OSS_HEXDUMP_LINEBUFFER_SIZE - 1 ;
      }
   }
   PD_TRACE1 ( SDB_OSSHEXDL, PD_PACK_UINT(curOff) );
   PD_TRACE_EXIT ( SDB_OSSHEXDL );
   return curOff ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSHEXDUMPBUF, "ossHexDumpBuffer" )
UINT32 ossHexDumpBuffer
(
   const void * inPtr,
   UINT32       len,
   CHAR *       szOutBuf,
   UINT32       outBufSz,
   const void * szPrefix,
   UINT32       flags,
   UINT32 *     pBytesProcessed
)
{
   SDB_ASSERT( szOutBuf != NULL, "szOutBuf can't be null" ) ;
   PD_TRACE_ENTRY ( SDB_OSSHEXDUMPBUF );
   UINT32 bytesProcessed = 0 ;
   CHAR szLineBuf[OSS_HEXDUMP_LINEBUFFER_SIZE] = { 0 } ;
   unsigned char preLine[ OSS_HEXDUMP_BYTES_PER_LINE ] = { 0 } ;
   bool bIsDupLine = false ;
   bool bPrinted = false ;
   CHAR * curPos = szOutBuf ;
   UINT32 prefixLength = 0 ;
   UINT32 totalLines = ossAlignX(len, OSS_HEXDUMP_BYTES_PER_LINE) /
                          OSS_HEXDUMP_BYTES_PER_LINE ;
   const char * cPtr = (const char *)inPtr ;
   const char * addrPtr = (const char *)szPrefix ;
   char szAddrStr[ OSS_HEXDUMP_ADDRESS_SIZE + 1 ] = { 0 } ;

   /* sanity check */
   if ( !( inPtr && szOutBuf && outBufSz ) )
   {
      goto exit ;
   }

   if ( flags & OSS_HEXDUMP_PREFIX_AS_ADDR )
   {
      flags = ( ~ OSS_HEXDUMP_INCLUDE_ADDR ) & flags ;
      prefixLength = OSS_HEXDUMP_ADDRESS_SIZE ;
   }
   else if (szPrefix)
   {
      prefixLength = ossStrlen((const CHAR*)szPrefix) ;
   }

   for ( UINT32 i = 0 ;
         i < totalLines ;
         i++, cPtr += OSS_HEXDUMP_BYTES_PER_LINE,
              addrPtr += OSS_HEXDUMP_BYTES_PER_LINE  )
   {
      UINT32 curLen, curOff ;
      if ( i + 1 == totalLines )
      {
         curLen = len - i * OSS_HEXDUMP_BYTES_PER_LINE ;
      }
      else
      {
         curLen = OSS_HEXDUMP_BYTES_PER_LINE ;
      }

      if ( OSS_HEXDUMP_BYTES_PER_LINE == curLen )
      {
         if ( i > 0 )
         {
            bIsDupLine = ( 0 == ossMemcmp( preLine,
                                           cPtr,
                                           OSS_HEXDUMP_BYTES_PER_LINE ) ) ;
         }
         ossMemcpy( preLine, cPtr, OSS_HEXDUMP_BYTES_PER_LINE ) ;
      }

      if ( ! bIsDupLine )
      {
         curOff = ossHexDumpLine( cPtr, curLen, szLineBuf, flags ) ;
      }
      else
      {
         curOff = 0 ;
         szLineBuf[0]= '\0' ;
      }

      if ( outBufSz >= curOff + prefixLength + 1 )
      {
         bytesProcessed += curLen ;
         if ( ! bIsDupLine )
         {
            if ( flags & OSS_HEXDUMP_PREFIX_AS_ADDR )
            {
               ossSnprintf( szAddrStr, sizeof( szAddrStr ),
                            "0x" OSS_PRIXPTR OSS_HEXDUMP_SPLITER,
                            (UintPtr)addrPtr ) ;
               ossStrncpy(curPos, szAddrStr, prefixLength + 1) ;
               curPos += prefixLength ;
               outBufSz -= prefixLength ;
            }
            else
            {
               if ( prefixLength )
               {
                  /* copy prefix first */
                  ossStrncpy(curPos, (const CHAR*)szPrefix, prefixLength + 1) ;
                  curPos += prefixLength ;
                  outBufSz -= prefixLength ;
               }
            }
            bPrinted = false ;
         }
         else
         {
            if ( ! bPrinted )
            {
               ossStrncpy(curPos, "*" OSS_NEWLINE, sizeof( "*" OSS_NEWLINE )) ;
               curPos += sizeof( "*" OSS_NEWLINE ) - sizeof( '\0' ) ;
               outBufSz -= sizeof( "*" OSS_NEWLINE ) - sizeof( '\0' ) ;
               bPrinted = true ;
            }
         }
         ossStrncpy(curPos, szLineBuf, curOff + 1) ;
         outBufSz -= curOff ;
         curPos += curOff ;
      }
      else
      {
         break ;
      }
      if ( ( curPos ) && ( (int)( curPos - szOutBuf ) >= 0 ) )
      {
         *curPos = '\0' ;
      }
   }

exit :
   if ( pBytesProcessed )
   {
      * pBytesProcessed = bytesProcessed ;
   }
   PD_TRACE_EXIT ( SDB_OSSHEXDUMPBUF );
   return  ( (UINT32)( curPos - szOutBuf ) ) ;
}

#if defined (_LINUX) || defined (_AIX)
#define OSS_GET_MEM_INFO_FILE             "/proc/meminfo"
#define OSS_GET_MEM_INFO_OVERCOMMIT_FILE  "/proc/sys/vm/overcommit_memory"
#define OSS_GET_MEM_INFO_MEMTOTAL         "MemTotal"
#define OSS_GET_MEM_INFO_MEMFREE          "MemFree"
#define OSS_GET_MEM_INFO_MEMAVAILABLE     "MemAvailable"
#define OSS_GET_MEM_INFO_SWAPTOTAL        "SwapTotal"
#define OSS_GET_MEM_INFO_SWAPFREE         "SwapFree"
#define OSS_GET_MEM_INFO_COMMITLIM        "CommitLimit"
#define OSS_GET_MEM_INFO_COMMITTED        "Committed_AS"
#define OSS_GET_MEM_INFO_AMPLIFIER        1024ll
#elif defined (_WINDOWS)
#define OSS_GET_MEM_INFO_AMPLIFIER        1024LL
#endif
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETMEMINFO, "ossGetMemoryInfo" )
INT32 ossGetMemoryInfo ( INT32 &loadPercent,  INT64 &totalPhys,
                         INT64 &freePhys,     INT64 &availPhys,
                         INT64 &totalPF,      INT64 &availPF,
                         INT64 &totalVirtual, INT64 &availVirtual,
                         INT32 &overCommitMode,
                         INT64 &commitLimit,  INT64 &committedAS )
{
   INT32 rc     = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETMEMINFO );
   INT32 ossErr = 0 ;
   totalPhys    = -1 ;
   availPhys    = -1 ;
   totalPF      = -1 ;
   availPF      = -1 ;
   totalVirtual = -1 ;
   availVirtual = -1 ;
   loadPercent  = -1 ;
   commitLimit  = -1 ;
   committedAS  = -1 ;
   overCommitMode = -1 ;
#if defined (_WINDOWS)
   MEMORYSTATUSEX statex ;
   statex.dwLength = sizeof(statex) ;
   if ( GlobalMemoryStatusEx ( &statex ) )
   {
      loadPercent  = statex.dwMemoryLoad ;
      totalPhys    = statex.ullTotalPhys ;
      availPhys    = statex.ullAvailPhys ;
      freePhys     = availPhys ;
      totalPF      = statex.ullTotalPageFile ;
      availPF      = statex.ullAvailPageFile ;
      totalVirtual = statex.ullTotalVirtual ;
      availVirtual = statex.ullAvailVirtual ;
   }
   else
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
   // There is not over commit in windows, only in linux
   overCommitMode = 0 ;
#elif defined (_LINUX) || defined (_AIX)
   CHAR pathName[OSS_PROC_PATH_LEN_MAX + 1] = {0} ;
   CHAR lineBuffer [OSS_PROC_PATH_LEN_MAX+1] = {0} ;
   INT32 inputNum = 0 ;
   FILE *fp = NULL ;
   ossSnprintf ( pathName, sizeof(pathName), OSS_GET_MEM_INFO_FILE ) ;
   fp = fopen ( pathName, "r" ) ;
   if ( !fp )
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
   // loop until hitting end of the file or 4 variables are read
   while ( fgets ( lineBuffer, OSS_PROC_PATH_LEN_MAX, fp ) &&
           ( totalPhys   == -1 ||
             availPhys   == -1 ||
             freePhys    == -1 ||
             totalPF     == -1 ||
             availPF     == -1 ||
             commitLimit == -1 ||
             committedAS == -1 )
          )
   {
      if ( ossStrncmp ( lineBuffer,
                        OSS_GET_MEM_INFO_MEMTOTAL,
                        ossStrlen ( OSS_GET_MEM_INFO_MEMTOTAL ) ) == 0 )
      {
         // +1 to skip ":"
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_MEMTOTAL )+1],
                  "%d", &inputNum ) ;
         totalPhys = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if ( ossStrncmp ( lineBuffer,
                             OSS_GET_MEM_INFO_MEMAVAILABLE,
                             ossStrlen ( OSS_GET_MEM_INFO_MEMAVAILABLE ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_MEMAVAILABLE )+1],
                  "%d", &inputNum ) ;
         availPhys = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if ( ossStrncmp ( lineBuffer,
                             OSS_GET_MEM_INFO_MEMFREE,
                             ossStrlen ( OSS_GET_MEM_INFO_MEMFREE ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_MEMFREE )+1],
                  "%d", &inputNum ) ;
         freePhys = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if (  ossStrncmp ( lineBuffer,
                              OSS_GET_MEM_INFO_SWAPTOTAL,
                              ossStrlen ( OSS_GET_MEM_INFO_SWAPTOTAL ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_SWAPTOTAL )+1],
                  "%d", &inputNum ) ;
         totalPF = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if (  ossStrncmp ( lineBuffer,
                              OSS_GET_MEM_INFO_SWAPFREE,
                              ossStrlen ( OSS_GET_MEM_INFO_SWAPFREE ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_SWAPFREE )+1],
                  "%d", &inputNum ) ;
         availPF = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if (  ossStrncmp ( lineBuffer,
                              OSS_GET_MEM_INFO_COMMITLIM,
                              ossStrlen ( OSS_GET_MEM_INFO_COMMITLIM ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_COMMITLIM )+1],
                  "%d", &inputNum ) ;
         commitLimit = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if (  ossStrncmp ( lineBuffer,
                              OSS_GET_MEM_INFO_COMMITTED,
                              ossStrlen ( OSS_GET_MEM_INFO_COMMITTED ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_COMMITTED )+1],
                  "%d", &inputNum ) ;
         committedAS = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      ossMemset ( lineBuffer, 0, sizeof(lineBuffer) ) ;
   }
   fclose ( fp ) ;

   totalVirtual = totalPhys + totalPF ;
   availVirtual = availPhys + availPF ;
   if ( totalPhys != 0 )
   {
      loadPercent = 100 * ( totalPhys - availPhys ) / totalPhys ;
      loadPercent = loadPercent > 100? 100:loadPercent ;
      loadPercent = loadPercent < 0? 0:loadPercent ;
   }
   else
   {
      loadPercent = 0 ;
   }

   fp = NULL ;
   fp = fopen ( OSS_GET_MEM_INFO_OVERCOMMIT_FILE, "r" ) ;
   if ( !fp )
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
   // read first line
   if ( !fgets ( lineBuffer, OSS_PROC_PATH_LEN_MAX, fp ) )
   {
      ossErr = ossGetLastError () ;
      fclose ( fp ) ;
      rc = SDB_SYS ;
      goto error ;
   }
   sscanf ( &lineBuffer[0], "%d", &inputNum ) ;
   if( 0 == inputNum || 1 == inputNum || 2 == inputNum )
   {
      overCommitMode = inputNum ;
   }
   else
   {
      overCommitMode = 0 ; // defalut mode
   }
   fclose ( fp ) ;
#endif
done :
   PD_TRACE_EXITRC ( SDB_OSSGETMEMINFO, rc );
   return rc ;
error :
   PD_LOG ( PDERROR, "Failed to get memory info, error = %d",
            ossErr ) ;
   goto done ;
}

INT32 ossGetMemoryInfo ( INT32 &loadPercent,  INT64 &totalPhys,
                         INT64 &freePhys,     INT64 &availPhys,
                         INT64 &totalPF,      INT64 &availPF,
                         INT64 &totalVirtual, INT64 &availVirtual )
{
   INT32 overCommitMode = 0 ;
   INT64 commitLimit = 0 ;
   INT64 committedAS = 0 ;

   return ossGetMemoryInfo ( loadPercent, totalPhys,
                             freePhys, availPhys,
                             totalPF, availPF,
                             totalVirtual, availVirtual,
                             overCommitMode,
                             commitLimit, committedAS ) ;
}

#if defined (_LINUX) || defined (_AIX)
// the mounted filesystem description file
#define OSS_GET_DISK_INFO_FILE      "/etc/mtab"
#endif

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETDISKINFO, "ossGetDiskInfo" )
INT32 ossGetDiskInfo ( const CHAR *pPath, INT64 &totalBytes, INT64 &freeBytes,
                       INT64 &availBytes, INT32 &loadPercent,
                       CHAR* fsName, INT32 fsNameSize )
{
   INT32 rc                = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETDISKINFO );
#if defined (_WINDOWS)
   LPWSTR pszWString       = NULL ;
   LPSTR lpszVolumePath    = NULL ;
   DWORD dwString          = 0 ;
   BOOL success            = FALSE ;
   WCHAR volumePath[ OSS_MAX_PATHSIZE ] = L"" ;

   rc = ossANSI2WC ( pPath, &pszWString, &dwString ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to convert ansi to wc, rc = %d", rc );

   // get total space and free space
   success = GetDiskFreeSpaceEx ( pszWString, (PULARGE_INTEGER) &availBytes,
                                 (PULARGE_INTEGER) &totalBytes,
                                 (PULARGE_INTEGER) &freeBytes ) ;

   PD_CHECK( success, SDB_SYS, error, PDERROR, "Failed to get disk space"
             ", errno: %d, rc = %d", ossGetLastError(), rc );

   // get disk name
   if ( NULL == fsName )
   {
      goto done ;
   }

   // get percentage of disk load in space
   if ( 0 != totalBytes )
   {
      loadPercent = 100 * ( totalBytes - freeBytes ) /
                    totalBytes ;
      loadPercent = loadPercent > 100 ? 100 : loadPercent ;
      loadPercent = loadPercent < 0 ? 0 : loadPercent ;
   }
   else
   {
      loadPercent = 0 ;
   }

   success = GetVolumePathName ( pszWString, volumePath, OSS_MAX_PATHSIZE + 1 ) ;
   PD_CHECK( success, SDB_SYS, error, PDERROR, "Failed to get disk name"
             ", errno: %d, rc = %d", ossGetLastError(), rc );

   rc = ossWC2ANSI ( volumePath, &lpszVolumePath, NULL ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to convert ansi to wc, rc = %d", rc );

   ossStrncpy ( fsName, lpszVolumePath, fsNameSize - 1 ) ;
   fsName[ fsNameSize - 1 ] = '\0' ;

#elif defined (_LINUX) || defined (_AIX)
   INT32 retcode = 0 ;
   struct statvfs vfs ;
   FILE *fp = NULL ;
   struct mntent *me = NULL ;
   struct mntent dummy ;
   CHAR tmpBuff[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   struct stat pathStat ;
   BOOLEAN findOut = FALSE ;
   dev_t pathDevID ;
   INT64 totalAvailale = 0 ;

   /// 1. get total and free space
   if ( statvfs ( pPath, &vfs ) )
   {
      retcode = ossGetLastError() ;
      goto error ;
   }

   totalBytes = vfs.f_frsize * vfs.f_blocks ;
   freeBytes = vfs.f_bsize * vfs.f_bfree ;
   availBytes = vfs.f_bsize * vfs.f_bavail ;
   

   /// 2. get disk name ( device name )
   if ( NULL == fsName )
   {
      goto done ;
   }

   // 2.1 get device id of the specified path
   if ( stat ( pPath, &pathStat ) )
   {
      retcode = ossGetLastError() ;
      goto error ;
   }

   pathDevID = pathStat.st_dev ;

   // 2.2 get disk name of this path
   fp = setmntent ( OSS_GET_DISK_INFO_FILE, "r" ) ;
   if ( NULL == fp )
   {
      retcode = ossGetLastError() ;
      goto error ;
   }

   // 2.3 get percentage of disk load in space
   // f_blocks means total space, f_bfree means free space in disk
   // f_bavail means free space excluded system reserved space
   // so total available space should be total space excluded system reserved space
   totalAvailale = totalBytes - freeBytes + availBytes ;
   if ( 0 != totalAvailale )
   {
      loadPercent = 1 + 100 * ( totalBytes - freeBytes ) /
                    totalAvailale ;
      loadPercent = loadPercent > 100 ? 100 : loadPercent ;
      loadPercent = loadPercent < 0 ? 0 : loadPercent ;
   }
   else
   {
      loadPercent = 0 ;
   }

   while ( NULL != ( me = getmntent_r ( fp, &dummy,
                                        tmpBuff, OSS_MAX_PATHSIZE ) ) )
   {
      struct stat fsStat ;

      if ( !stat ( me->mnt_fsname, &fsStat ) )
      {
         dev_t devID = fsStat.st_rdev ;

         if ( pathDevID == devID )
         {
            ossStrncpy( fsName, me->mnt_fsname, fsNameSize - 1 ) ;
            fsName[ fsNameSize - 1 ] = '\0' ;
            findOut = TRUE ;
            break ;
         }
      }
   }

   // set disk name, if it hasn't find out
   if ( FALSE == findOut )
   {
      ossStrncpy( fsName, "unknown-disk", fsNameSize -1 ) ;
      fsName[ fsNameSize - 1 ] = '\0' ;
   }

#endif
done :
#if defined (_WINDOWS)
   if ( pszWString )
   {
      SDB_OSS_FREE ( pszWString ) ;
      pszWString = NULL ;
   }
   if ( lpszVolumePath )
   {
      SDB_OSS_FREE ( lpszVolumePath ) ;
      lpszVolumePath = NULL ;
   }
#elif defined (_LINUX) || defined (_AIX)
   if ( NULL != fp )
   {
      endmntent( fp ) ;
   }
   switch( retcode )
   {
   case 0:
      break ;
   case EACCES:
      rc = SDB_PERM ;
      break ;
   case EINTR:
      rc = SDB_INTERRUPT ;
      break ;
   case EIO:
      rc = SDB_IO ;
      break ;
   case ENOENT:
      rc = SDB_FNE ;
      break ;
   case ENOMEM:
      rc = SDB_OOM ;
      break ;
   case ENOTDIR:
   case EFAULT:
      rc = SDB_INVALIDARG ;
      break ;
   default:
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Failed to get disk info, errno = %d", retcode ) ;
      break ;
   }
#endif
   PD_TRACE_EXITRC ( SDB_OSSGETDISKINFO, rc );
   return rc ;
error :
   goto done ;
}

/*
   ossGetFDNum implement
   get the number of file description / open files
*/
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETFILEDESP, "ossGetFileDesp" )
INT32 ossGetFileDesp ( INT64 &usedNum )
{
   PD_TRACE_ENTRY ( SDB_OSSGETFILEDESP ) ;
   INT32 rc = SDB_OK ;

#if defined (_LINUX) || defined (_AIX)

   CHAR pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = { 0 } ;
   DIR *dp = NULL ;
   struct dirent *dir = NULL ;
   INT32 ossErr = 0 ;
   BOOLEAN isOpen = FALSE;

   // Calculate the number of files in the path:  /proc/[pid]/fd
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/fd", getpid() ) ;
   dp = opendir( pathName ) ;
   if( dp == NULL )
   {
      ossErr = ossGetLastError() ;
      if ( EMFILE == ossErr )
      {
         rc = SDB_TOO_MANY_OPEN_FD ;
      }
      else
      {
         rc = SDB_SYS ;
      }
      PD_LOG( PDERROR, "Failed to open dir: %s, errno: %d, rc = %d",
              pathName, ossErr, rc );
      goto error ;
   }
   isOpen = TRUE ;

   usedNum = 0 ;
   while ( ( dir = readdir(dp) ) != NULL )
   {
      // there may be . (current dir) or .. (parent dir),
      // but we don't need to count them
      if ( 0 != ossStrcmp( dir->d_name, "." ) &&
           0 != ossStrcmp( dir->d_name, ".." ) )
      {
         usedNum++ ;
      }
   }
   closedir( dp ) ;

#elif defined (_WINDOWS)

   INT32 retcode = 0 ;
   retcode = GetProcessHandleCount( GetCurrentProcess(), ( PDWORD )&usedNum ) ;
   PD_CHECK( retcode != 0, SDB_SYS, error, PDERROR, "Failed to get handle count"
             "of current process, errno: %d, rc = %d", ossGetLastError(), rc );
#endif

done :
   PD_TRACE_EXITRC( SDB_OSSGETFILEDESP, rc ) ;
   return rc ;
error :

#if defined (_LINUX) || defined (_AIX)
   if( isOpen )
   {
      closedir( dp ) ;
   }
#endif
   goto done ;
}

#if defined (_LINUX) || defined (_AIX)
#define OSS_GET_PROC_MEM_VMSIZE   "VmSize"
#define OSS_GET_PROC_MEM_VMRSS    "VmRSS"
#endif

INT32 ossGetProcessMemory( OSSPID pid, INT64 &vmRss, INT64 &vmSize )
{
   INT32 rc = SDB_OK ;
   INT32 ossErr = 0 ;
   vmSize = -1 ;
   vmRss = -1 ;

#if defined (_WINDOWS)
   PROCESS_MEMORY_COUNTERS pmc ;
   if ( GetProcessMemoryInfo ( GetCurrentProcess(), &pmc, sizeof(pmc) ) )
   {
      vmRss  = pmc.WorkingSetSize ;
      vmSize = pmc.WorkingSetSize + pmc.PagefileUsage ;
   }
   else
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }

#elif defined (_LINUX) || defined (_AIX)
   CHAR pathName[OSS_PROC_PATH_LEN_MAX + 1] = {0} ;
   CHAR lineBuffer [OSS_PROC_PATH_LEN_MAX+1] = {0} ;
   INT32 inputNum = 0 ;
   FILE *fp = NULL ;
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/status", pid ) ;
   fp = fopen ( pathName, "r" ) ;
   if ( !fp )
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
   // loop until hitting end of the file or variables are read
   while ( fgets ( lineBuffer, OSS_PROC_PATH_LEN_MAX, fp ) &&
           ( vmSize == -1 || vmRss == -1 ) )
   {
      if ( ossStrncmp ( lineBuffer, OSS_GET_PROC_MEM_VMSIZE,
                        ossStrlen ( OSS_GET_PROC_MEM_VMSIZE ) ) == 0 )
      {
         // +1 to skip ":"
         sscanf ( &lineBuffer[ ossStrlen( OSS_GET_PROC_MEM_VMSIZE ) + 1 ],
                  "%d", &inputNum ) ;
         vmSize = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if ( ossStrncmp ( lineBuffer, OSS_GET_PROC_MEM_VMRSS,
                             ossStrlen ( OSS_GET_PROC_MEM_VMRSS ) ) == 0 )
      {
         // +1 to skip ":"
         sscanf ( &lineBuffer[ ossStrlen( OSS_GET_PROC_MEM_VMRSS ) + 1 ],
                  "%d", &inputNum ) ;
         vmRss = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      ossMemset ( lineBuffer, 0, sizeof(lineBuffer) ) ;
   }
   fclose ( fp ) ;

#endif

done :
   return rc ;
error :
   PD_LOG ( PDERROR, "Failed to get memory info, error = %d", ossErr ) ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSREADLINK, "ossReadlink" )
INT32 ossReadlink ( const CHAR *pPath, CHAR *pLinkedPath, INT32 maxLen )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSREADLINK ) ;

#if defined (_LINUX) || defined (_AIX)
   INT32 len = readlink( pPath, pLinkedPath, maxLen ) ;
   if ( len <= 0 || len >= maxLen )
   {
      rc = SDB_INVALIDARG ;
   }
   else
   {
      pLinkedPath[len] = '\0' ;
   }
#endif

   PD_TRACE_EXITRC( SDB_OSSREADLINK, rc ) ;
   return rc ;
}

#if defined (_LINUX) || defined (_AIX)
#define OSS_DISK_IO_STAT_FILE "/proc/diskstats"
#endif

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETDISKIOSTAT, "ossGetDiskIOStat" )
INT32 ossGetDiskIOStat ( const CHAR *pDriverName, ossDiskIOStat &ioStat )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY ( SDB_OSSGETDISKIOSTAT ) ;

#if defined (_LINUX) || defined (_AIX)
   INT32 ossErr = 0 ;
   CHAR pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = {0} ;
   CHAR lineBuffer[ OSS_PROC_PATH_LEN_MAX + 1 ] = {0} ;
   CHAR driverName[ OSS_PROC_PATH_LEN_MAX + 1 ] = {0} ;
   FILE *fp = NULL ;

   ossSnprintf ( pathName, sizeof( pathName ), OSS_DISK_IO_STAT_FILE ) ;
   fp = fopen ( pathName, "r" ) ;
   if ( !fp )
   {
      ossErr = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to open disk stat file, error = %d",
               ossErr ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while ( fgets( lineBuffer, sizeof( lineBuffer ), fp) != NULL ) {
      INT32 fieldRead = 0 ;

      UINT32 major, minor ;
      UINT32 ios_pgr, tot_ticks, rq_ticks, wr_ticks ;
      UINT64 rd_ios, rd_merges_or_rd_sec, wr_ios, wr_merges ;
      UINT64 rd_sec_or_wr_ios, wr_sec, rd_ticks_or_wr_sec ;
      fieldRead = sscanf( lineBuffer,
                          "%u %u %s %llu %llu %llu %llu %llu %llu %llu %u %u %u %u",
                          &major, &minor, driverName,
                          &rd_ios, &rd_merges_or_rd_sec, &rd_sec_or_wr_ios,
                          &rd_ticks_or_wr_sec, &wr_ios, &wr_merges, &wr_sec,
                          &wr_ticks, &ios_pgr, &tot_ticks, &rq_ticks) ;
      if ( 0 != ossStrcmp( pDriverName, driverName ) )
      {
         continue ;
      }
      if ( fieldRead == 14 )
      {
         /* Device or partition */
         ioStat.rdIos     = rd_ios ;
         ioStat.rdMerges  = rd_merges_or_rd_sec ;
         ioStat.rdSectors = rd_sec_or_wr_ios ;
         ioStat.rdTicks   = (unsigned int) rd_ticks_or_wr_sec ;
         ioStat.wrIos     = wr_ios ;
         ioStat.wrMerges  = wr_merges ;
         ioStat.wrSectors = wr_sec ;
         ioStat.wrTicks   = wr_ticks ;
         ioStat.iosPgr    = ios_pgr ;
         ioStat.totTicks  = tot_ticks ;
         ioStat.rqTicks   = rq_ticks ;
      }
      else if ( fieldRead == 7 ) {
         /* Partition without extended statistics */
         ioStat.rdIos      = rd_ios;
         ioStat.rdSectors  = rd_merges_or_rd_sec ;
         ioStat.wrIos      = rd_sec_or_wr_ios ;
         ioStat.wrSectors  = rd_ticks_or_wr_sec ;
      }
      break ;
   }

   fclose( fp ) ;

done :
   PD_TRACE_EXITRC ( SDB_OSSGETDISKIOSTAT, rc ) ;
   return rc ;
error :
   goto done ;
#else
   PD_TRACE_EXITRC ( SDB_OSSGETDISKIOSTAT, rc ) ;
   return rc ;
#endif //(_LINUX) || defined (_AIX)
}

#if defined (_WINDOWS)
typedef DWORD SYSTEM_INFORMATION_CLASS ;
#define SYSTEM_PROC_TIME 0x08
#define STATUS_SUCCESS ((NTSTATUS)0x0000000L)
typedef struct __SYSTEM_PROCESSOR_TIMES
{
   LARGE_INTEGER IdleTime ;
   LARGE_INTEGER KernelTime ;
   LARGE_INTEGER UserTime ;
   LARGE_INTEGER DpcTime ;
   LARGE_INTEGER InterruptTime ;
   ULONG         InterruptCount ;
} SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES ;

typedef NTSTATUS (__stdcall *NTQUERYSYSTEMINFORMATION)
                 (SYSTEM_INFORMATION_CLASS,
                  PVOID,
                  ULONG,
                  PULONG ) ;
#define OSS_NTQUERYSYSTEMINFORMATION_STR "NtQuerySystemInformation"
#elif defined (_LINUX) || defined (_AIX)
#define OSS_GET_CPU_INFO_FILE      "/proc/stat"
#define OSS_GET_CPU_INFO_PATTERN   "%lld%lld%lld%lld%lld%lld%lld"
#endif
// output is based on milliseconds
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETCPUINFO, "ossGetCPUInfo" )
INT32 ossGetCPUInfo ( SINT64 &user, SINT64 &sys,
                      SINT64 &idle, SINT64 &iowait,
                      SINT64 &other )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETCPUINFO );
   INT32 ossErr = 0 ;
#if defined (_WINDOWS)
   if ( !GetSystemTimes ( (LPFILETIME)&idle, (LPFILETIME)&sys,
                          (LPFILETIME)&user  ) )
   {
      PD_LOG ( PDERROR, "Failed to get system times" ) ;
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
   // GetSystemTimes returns 100-nanosecond-based time
   other = 0 ;
   idle /= 10000 ;
   sys /= 10000 ;
   // sys time also includes idle time
   sys -= idle ;
   user /= 10000 ;
   iowait = 0 ;
#elif defined (_LINUX) || defined (_AIX)
   CHAR pathName [ OSS_PROC_PATH_LEN_MAX + 1 ] = { 0 } ;
   CHAR buffer [ OSS_PROC_PATH_LEN_MAX + 1 ] = { 0 } ;
   SINT64 userTime = 0 ;
   SINT64 nicedTime = 0 ;
   SINT64 systemTime = 0 ;
   SINT64 idleTime = 0 ;
   SINT64 waitTime = 0 ;
   SINT64 irqTime = 0 ;
   SINT64 softirqTime = 0 ;
   SINT64 otherTime = 0 ;
   FILE *fp = NULL ;
   static int clkTck = 0 ;
   static int numMicrosecPerClkTck = 0 ;
   // On Linux, utime and stime use "jiffies" as unit of measurement.
   // Normally there are 100 jiffies per second
   if ( 0 == clkTck )
   {
      clkTck = sysconf ( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }
      // in time.h, CLOCKS_PER_SEC is required to be 1 million on all
      // XSI-conformant systems
      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }
   ossSnprintf ( pathName, sizeof(pathName), OSS_GET_CPU_INFO_FILE ) ;
   fp = fopen ( pathName, "r" ) ;
   if ( fp )
   {
      // read first line
      if ( !fgets ( buffer, OSS_PROC_PATH_LEN_MAX, fp ) )
      {
         ossErr = ossGetLastError () ;
         fclose ( fp ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      // 4 means skip "cpu"
      sscanf ( &buffer[4], OSS_GET_CPU_INFO_PATTERN,
               &userTime, &nicedTime, &systemTime,
               &idleTime, &waitTime, &irqTime, &softirqTime ) ;
      sys = systemTime / clkTck * 1000 +
            ( systemTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      user = ( userTime + nicedTime ) / clkTck * 1000 +
             ( ( userTime + nicedTime ) % clkTck ) * numMicrosecPerClkTck/1000;
      idle = idleTime / clkTck * 1000 +
             ( idleTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      iowait = waitTime / clkTck * 1000 +
             ( waitTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      otherTime = ( irqTime + softirqTime ) ;
      other = otherTime / clkTck * 1000 +
              ( otherTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      fclose ( fp ) ;
   }
   else
   {
      ossErr = ossGetLastError () ;
      rc = SDB_SYS ;
      goto error ;
   }
#endif
done :
   PD_TRACE_EXITRC (SDB_OSSGETCPUINFO, rc );
   return rc ;
error :
   PD_LOG ( PDERROR, "Failed to get CPU info, error = %d",
            ossErr ) ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETPROCMEMINFO, "ossGetProcMemInfo" )
INT32 ossGetProcMemInfo( ossProcMemInfo &memInfo,
                         OSSPID pid )
{
   INT32 rc = SDB_OK ;
   // PD_TRACE_ENTRY ( SDB_OSSGETPROCMEMINFO );
#if defined (_WINDOWS)
   HANDLE handle = GetCurrentProcess() ;
   PROCESS_MEMORY_COUNTERS pmc ;
   if ( GetProcessMemoryInfo( handle, &pmc, sizeof( pmc ) ) )
   {
      memInfo.rss = pmc.WorkingSetSize ;
      memInfo.vSize = pmc.PagefileUsage ;
      memInfo.shr = 0 ;
      memInfo.fault = pmc.PageFaultCount ;
   }
   else
   {
      PD_RC_CHECK( SDB_SYS, PDERROR,
                  "failed to get process memory info(errorno:%d)",
                  GetLastError() ) ;
   }
#elif defined (_LINUX) || defined (_AIX)
   ossProcStatInfo procInfo( pid ) ;
   memInfo.rss = procInfo._rss ;
   memInfo.vSize = procInfo._vSize ;
   memInfo.shr = procInfo._shr ;
   memInfo.fault = procInfo._majFlt ;
   PD_CHECK( procInfo._pid != -1, SDB_SYS, error, PDERROR,
            "failed to get process info(pid=%d)", pid ) ;
#else
   PD_RC_CHECK( SDB_SYS, PDERROR,
               "the OS is not supported!" ) ;
#endif
done:
   // PD_TRACE_EXITRC (SDB_OSSGETPROCMEMINFO, rc );
   return rc;
error:
   goto done;
}

#if defined (_LINUX) || defined (_AIX)
ossProcStatInfo::ossProcStatInfo( OSSPID pid )
:_pid(-1),_state(0),_ppid(-1),_pgrp(-1),
_session(-1),_tty(-1),_tpgid(-1),_flags(0),
_minFlt(0),_cMinFlt(0),_majFlt(0),_cMajFlt(0),
_uTime(0),_sTime(0),_cuTime(-1),_csTime(-1),
_priority(-1),_nice(-1),_nlwp(-1),_alarm(0),
_startTime(0),_vSize(0),_rss(-1),_shr(-1),_rssRlim(0),
_startCode(0),_endCode(0),_startStack(0),
_kstkEsp(0),_kstkEip(0)
{
   ossMemset( _comm, 0, OSS_MAX_PATHSIZE + 1 ) ;
   CHAR pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = {0} ;
   INT64 pagesz = ossGetPageSize() ;
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/stat", pid ) ;
   FILE *fp = NULL ;

   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
      INT32 rc = 0;
      INT64 rsPageNum = 0 ;

      rc = fscanf( fp,
                  "%d %s %c "             // &_pid, _comm, &_state
                  "%d %d %d %d %d "       // &_ppid, &_pgrp, &_session, &_tty, &_tpgid
                  "%u %u %u %u %u "       // &_flags, &_minFlt, &_cMinFlt, &_majFlt, &_cMajFlt
                  "%u %u %d %d "          // &_uTime, &_sTime, &_cuTime, &_csTime
                  "%d %d "                // &_priority, &_nice
                  "%d "                   // &_nlwp
                  "%u "                   // &_alarm
                  "%u "                   // &_startTime
                  "%lld "                 // &_vSize
                  "%lld "                 // &rsPageNum
                  "%llu %u %u "           // &_rssRlim, &_startCode, &_endCode
                  "%llu %llu %llu ",      // &_startStack, &_kstkEsp, &_kstkEip
                  &_pid, _comm, &_state,
                  &_ppid, &_pgrp, &_session, &_tty, &_tpgid,
                  &_flags, &_minFlt, &_cMinFlt, &_majFlt, &_cMajFlt,
                  &_uTime, &_sTime, &_cuTime, &_csTime,
                  &_priority, &_nice,
                  &_nlwp,
                  &_alarm,
                  &_startTime,
                  &_vSize,
                  &rsPageNum,
                  &_rssRlim, &_startCode, &_endCode,
                  &_startStack, &_kstkEsp, &_kstkEip ) ;
      fclose(fp) ;

      // change the number of pages to bytes
      _rss = rsPageNum * pagesz ;

      if ( rc <= 0 )
      {
         PD_LOG( PDERROR, "failed to read proc-info(stat)" );
      }
   }
   else
   {
      PD_LOG( PDWARNING, "open failed(%s)", pathName );
   }

   /// read shr
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/statm", pid ) ;
   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
      INT64 vSizePages = 0, resPages = 0, sharedPages = 0 ;
      INT32 rc = 0 ;
      rc = fscanf( fp,
                   "%lld %lld %lld ",      // &vSizePages, &resPages, &sharedPages
                   &vSizePages, &resPages, &sharedPages ) ;
      fclose(fp) ;

      // change the number of pages to bytes
      _vSize = vSizePages * pagesz ;
      _rss = resPages * pagesz ;
      _shr = sharedPages * pagesz ;

      if ( rc <= 0 )
      {
         PD_LOG( PDERROR, "Failed to read proc-info(statm)" );
      }
   }
   else
   {
      PD_LOG( PDWARNING, "open failed(%s)", pathName );
   }
}
#endif

ossIPInfo::ossIPInfo()
:_ipNum(0), _ips(NULL), _initRC(SDB_OK)
{
   INT32 rc = _init() ;
   if ( SDB_OK != rc )
   {
      _initRC = rc ;
      PD_LOG( PDERROR, "failed to get ip-info, errno = %d", ossGetLastError()) ;
   }
}

ossIPInfo::~ossIPInfo()
{
   SAFE_OSS_FREE( _ips ) ;
   _ipNum = 0 ;
}

#if defined (_LINUX) || defined (_AIX)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

INT32 ossIPInfo::_init()
{
   struct ifconf ifc = {0} ;
   struct ifreq* buf = NULL ;
   struct ifreq* ifr = NULL ;
   INT32 sock = -1 ;
   INT32 rc = SDB_OK ;

   sock = socket( AF_INET, SOCK_DGRAM, 0 ) ;
   if ( -1 == sock )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   // get actual ifc_len
   rc = ioctl( sock, SIOCGIFCONF, &ifc ) ;
   if ( 0 != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 0 == ifc.ifc_len )
   {
      goto done ;
   }

   // alloc mem for ifreq
   buf = (struct ifreq*)SDB_OSS_MALLOC( ifc.ifc_len ) ;
   if ( NULL == buf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   // get ip info with buf
   ifc.ifc_req = buf ;
   rc = ioctl( sock, SIOCGIFCONF, &ifc ) ;
   if ( 0 != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   // alloc mem for ip info
   _ipNum = ifc.ifc_len / sizeof( struct ifreq ) ;
   _ips = (ossIP*)SDB_OSS_MALLOC( _ipNum * sizeof(ossIP) ) ;
   if ( NULL == _ips )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( _ips, 0, _ipNum * sizeof(ossIP) ) ;

   // copy ip info
   ifr = buf ;
   for ( INT32 i = 0; i < _ipNum; i++ )
   {
      ossIP* ip = &_ips[i] ;
      ossStrncpy( ip->ipName, ifr->ifr_name, OSS_MAX_IP_NAME ) ;
      ossStrncpy( ip->ipAddr,
                  inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr),
                  OSS_MAX_IP_ADDR ) ;
      ifr++ ;
   }

done:
   if ( -1 != sock )
   {
      close( sock ) ;
   }
   SAFE_OSS_FREE( ifc.ifc_req ) ;
   return rc ;
error:
   _ipNum = 0 ;
   SAFE_OSS_FREE( _ips ) ;
   goto done ;
}
#elif defined (_WINDOWS)
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

INT32 ossIPInfo::_init()
{
   PIP_ADAPTER_INFO adapterInfo = NULL ;
   ULONG bufLen = sizeof( IP_ADAPTER_INFO ) ;
   INT32 retVal = 0 ;
   INT32 rc = SDB_OK ;

   adapterInfo = (PIP_ADAPTER_INFO)SDB_OSS_MALLOC( bufLen ) ;
   if ( !adapterInfo )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   // first call GetAdapterInfo to get actual bufLen size
   retVal = GetAdaptersInfo( adapterInfo, &bufLen ) ;
   if ( ERROR_BUFFER_OVERFLOW == retVal )
   {
      SDB_OSS_FREE( adapterInfo ) ;

      adapterInfo = (PIP_ADAPTER_INFO)SDB_OSS_MALLOC( bufLen ) ;
      if ( !adapterInfo )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      retVal = GetAdaptersInfo( adapterInfo, &bufLen ) ;
   }

   if ( NO_ERROR != retVal )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   // alloc mem for ip info
   _ipNum = bufLen / sizeof( IP_ADAPTER_INFO ) ;
   _ips = (ossIP*)SDB_OSS_MALLOC( _ipNum * sizeof(ossIP) ) ;
   if ( NULL == _ips )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( _ips, 0, _ipNum * sizeof(ossIP) ) ;

   // copy ip info
   PIP_ADAPTER_INFO adapter = adapterInfo ;
   for ( INT32 i = 0; adapter != NULL; i++ )
   {
      if ( MIB_IF_TYPE_ETHERNET != adapter->Type )
      {
         continue;
      }

      ossIP* ip = &_ips[i] ;
      ossSnprintf( ip->ipName, OSS_MAX_IP_NAME, "eth%d", adapter->Index ) ;
      ossStrncpy( ip->ipAddr,
                  adapter->IpAddressList.IpAddress.String,
                  OSS_MAX_IP_ADDR ) ;

      adapter = adapter->Next ;
   }

done:
   SAFE_OSS_FREE( adapterInfo ) ;
   return rc ;
error:
   _ipNum = 0 ;
   SAFE_OSS_FREE( _ips ) ;
   goto done ;
}
#else
#error "unsupported os"
#endif

ossProcLimits::ossProcLimits()
{
   INT32 rc = init();
   if ( SDB_OK != rc )
   {
      PD_LOG( PDWARNING, "Init limit info failed, rc:%d", rc ) ;
   }
}

#if defined (_LINUX) || defined (_AIX)
#include <sys/resource.h>

std::string ossProcLimits::str()const
{
   std::stringstream ss ;
   std::map<const CHAR *, std::pair<INT64, INT64>, cmp >::const_iterator itr =
                                    _desc.begin() ;
   for ( ; itr != _desc.end(); ++itr )
   {
      ss << itr->first << "\t" << itr->second.first << "\t" << itr->second.second << "\n" ;
   }
   return ss.str() ;
}

INT32 ossProcLimits::init()
{
   INT32 rc = SDB_OK ;
   _initRLimit( RLIMIT_AS,       OSS_LIMIT_VIRTUAL_MEM ) ;
   _initRLimit( RLIMIT_CORE,     OSS_LIMIT_CORE_SZ ) ;
   _initRLimit( RLIMIT_CPU,      OSS_LIMIT_CPU_TIME ) ;
   _initRLimit( RLIMIT_DATA,     OSS_LIMIT_DATA_SEG_SZ ) ;
   _initRLimit( RLIMIT_FSIZE,    OSS_LIMIT_FILE_SZ ) ;
   _initRLimit( RLIMIT_STACK,    OSS_LIMIT_STACK_SIZE ) ;
   _initRLimit( RLIMIT_NOFILE,   OSS_LIMIT_OPEN_FILE ) ;
#if !defined (_AIX)
   _initRLimit( RLIMIT_LOCKS,    OSS_LIMIT_FILE_LOCK ) ;
   _initRLimit( RLIMIT_MEMLOCK,  OSS_LIMIT_MEM_LOCK ) ;
   _initRLimit( RLIMIT_MSGQUEUE, OSS_LIMIT_MSG_QUEUE ) ;
   _initRLimit( RLIMIT_RTPRIO,   OSS_LIMIT_SCHE_PRIO ) ;
   _initRLimit( RLIMIT_NPROC,    OSS_LIMIT_PROC_NUM ) ;
#endif  //_AIX
   return rc ;
}

void ossProcLimits::_initRLimit( INT32 resource, const CHAR *str )
{
   struct rlimit r ;
   INT32 rc = ::getrlimit( resource, &r ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to get limit of:%d, errno:%d",
              resource, ossGetLastError() ) ;
   }
   else
   {
      std::pair<INT64, INT64> &p = _desc[str] ;
      p.first = ( RLIM_INFINITY == r.rlim_cur ) ?
                -1 : r.rlim_cur ;
      p.second = ( RLIM_INFINITY == r.rlim_max ) ?
                 -1 : r.rlim_max ;
   }
   return ;
}

BOOLEAN ossProcLimits::getLimit( const CHAR *str,
                                 INT64 &soft,
                                 INT64 &hard ) const
{
    std::map<const CHAR *, std::pair<INT64, INT64>, cmp >::const_iterator
                                                       itr = _desc.find( str ) ;
    if ( _desc.end() == itr )
    {
       return FALSE ;
    }
    else
    {
       soft = itr->second.first ;
       hard = itr->second.second ;
       return TRUE ;
    }
}

// This function return system errno, instead of sequoiadb error code
INT32 ossProcLimits::setLimit( const CHAR *str, INT64 soft, INT64 hard )
{
   INT32 rc = 0 ;
   soft = ( -1 == soft ) ? RLIM_INFINITY : soft ;
   hard = ( -1 == hard ) ? RLIM_INFINITY : hard ;
   struct rlimit lim = { (rlim_t) soft, (rlim_t) hard } ;
   INT32 resource = 0 ;

   if ( ossStrcmp( str, OSS_LIMIT_VIRTUAL_MEM ) == 0 )
   {
      resource = RLIMIT_AS ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_CORE_SZ ) == 0 )
   {
      resource = RLIMIT_CORE ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_CPU_TIME ) == 0 )
   {
      resource = RLIMIT_CPU ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_DATA_SEG_SZ ) == 0 )
   {
      resource = RLIMIT_DATA ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_FILE_SZ ) == 0 )
   {
      resource = RLIMIT_FSIZE ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_STACK_SIZE ) == 0 )
   {
      resource = RLIMIT_STACK ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_OPEN_FILE ) == 0 )
   {
      resource = RLIMIT_NOFILE ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_FILE_LOCK ) == 0 )
   {
      resource = RLIMIT_LOCKS ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_MEM_LOCK ) == 0 )
   {
      resource = RLIMIT_MEMLOCK ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_MSG_QUEUE ) == 0 )
   {
      resource = RLIMIT_MSGQUEUE ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_SCHE_PRIO ) == 0 )
   {
      resource = RLIMIT_RTPRIO ;
   }
   else if ( ossStrcmp( str, OSS_LIMIT_PROC_NUM ) == 0 )
   {
      resource = RLIMIT_NPROC ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   rc = ::setrlimit( resource, &lim ) ;
   if( rc != 0 )
   {
      rc = ossGetLastError() ;
      PD_LOG( PDERROR, "failed to set limit of [%s], errno:%d", str, rc ) ;
      goto error ;
   }
   else
   {
      _initRLimit( resource, str ) ;
   }

done :
   return rc ;
error :
   goto done ;
}

#else
INT32 ossProcLimits::init()
{
   return SDB_SYS ;
}

void ossProcLimits::_initRLimit( INT32 resource, const CHAR *str )
{
   return ;
}

BOOLEAN ossProcLimits::getLimit( const CHAR *str,
                                 INT64 &soft,
                                 INT64 &hard ) const
{
   return TRUE ;
}

INT32 ossProcLimits::setLimit( const CHAR *str, INT64 soft, INT64 hard )
{
   return SDB_OK ;
}

std::string ossProcLimits::str()const
{
   return "" ;
}
#endif // defined (_LINUX) || defined (_AIX)

BOOLEAN ossNetIpIsValid( const CHAR *ip, INT32 len )
{
   INT32 section = 0 ;
   INT32 dot = 0 ;
   INT32 last = -1 ;
   INT32 curLen = 0 ;
   while ( *ip && curLen < len  )
   {
      if ( '.' == *ip )
      {
         if ( ++dot > 3 )
         {
            return FALSE ;
         }
         section = 0 ;
      }
      else if ( *ip >= '0' && *ip <= '9' )
      {
         section = section * 10 + *ip - '0' ;
         if ( section < 0 || section > 255 )
         {
            return FALSE ;
         }
      }
      else
      {
         return FALSE ;
      }
      last = *ip ;
      ++ip ;
      ++curLen ;
   }
   if ( 3 == dot && last != '.' )
   {
       return TRUE ;
   }
   return FALSE ;

}

INT32& ossGetSignalShieldFlag()
{
   static OSS_THREAD_LOCAL INT32 s_signalShieldFlag = 0 ;
   return s_signalShieldFlag ;
}

INT32& ossGetPendingSignal()
{
   static OSS_THREAD_LOCAL INT32 s_pendingSignal = 0 ;
   return s_pendingSignal ;
}

/*
   ossSignalShield implement
*/
ossSignalShield::ossSignalShield()
{
   ++ ossGetSignalShieldFlag() ;
}

ossSignalShield::~ossSignalShield()
{
   close() ;
}

void ossSignalShield::close()
{
   -- ossGetSignalShieldFlag() ;

   if ( 0 == ossGetSignalShieldFlag() )
   {
#if defined (_LINUX)
      if ( ossGetPendingSignal() > 0 &&
           SIGPIPE != ossGetPendingSignal() )
      {
         ossPThreadKill( ossPThreadSelf(), ossGetPendingSignal() ) ;
      }
#endif // _LINUX
      ossGetPendingSignal() = 0 ;
   }
}

INT32 ossException2RC( std::exception *pe )
{
   if ( NULL != dynamic_cast<std::bad_alloc*>(pe) )
   {
      return SDB_OOM ;
   }
   return SDB_SYS ;
}

OSS_INLINE BOOLEAN ossIsNaN( FLOAT64 d )
{
   return d != d ;
}

OSS_INLINE BOOLEAN ossIsInf( FLOAT64 d, INT32 * sign )
{
   volatile FLOAT64 tmp = d ;

   if ( ( tmp == d ) && ( ( tmp - d ) != 0.0 ) )
   {
      if ( sign )
      {
         *sign = ( d < 0.0 ? -1 : 1 ) ;
      }
      return TRUE ;
   }
   if ( sign )
   {
      *sign = 0 ;
   }

   return FALSE ;
}

