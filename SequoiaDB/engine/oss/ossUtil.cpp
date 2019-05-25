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

void ossLocalTime ( time_t &Time, struct tm &TM )
{
#if defined (_LINUX ) || defined (_AIX)
   localtime_r( &Time, &TM ) ;
#elif defined (_WINDOWS)
   localtime_s( &TM, &Time ) ;
#endif
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

void ossGmtime ( time_t &Time, struct tm &TM )
{
#if defined (_LINUX ) || defined (_AIX)
   gmtime_r( &Time, &TM ) ;
#elif defined (_WINDOWS)
   gmtime_s( &TM, &Time ) ;
#endif
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSTS2STR, "ossTimestampToString" )
void ossTimestampToString( ossTimestamp &Tm, CHAR * pStr )
{
   PD_TRACE_ENTRY ( SDB_OSSTS2STR );
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

// PD_TRACE_DECLARE_FUNCTION ( SDB_STR2OSSTS, "ossStringToTimestamp" )
void ossStringToTimestamp( const CHAR * pStr, ossTimestamp &Tm )
{
   PD_TRACE_ENTRY ( SDB_STR2OSSTS );
   struct tm tmp ;
   CHAR format[] = "%04d-%02d-%02d-%02d.%02d.%02d.%06d" ;

   ossSscanf( pStr, format, &tmp.tm_year, &tmp.tm_mon, &tmp.tm_mday,
              &tmp.tm_hour, &tmp.tm_min, &tmp.tm_sec, &Tm.microtm ) ;
   tmp.tm_year -= 1900 ;
   tmp.tm_mon  -= 1 ;

   Tm.time = mktime( &tmp ) ;

   PD_TRACE_EXIT ( SDB_STR2OSSTS );
}

void ossGetCurrentTime( ossTimestamp &TM )
{
#if defined (_LINUX) || defined (_AIX)
   struct timeval tv ;

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

   uLargeIntegerTime.LowPart  = fileTime.dwLowDateTime ;
   uLargeIntegerTime.HighPart = fileTime.dwHighDateTime ;

   uLargeIntegerTime.QuadPart -= ( DELTA_EPOCH_IN_MICROSECS * 10 ) ;

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

UINT64 ossGetCurrentMilliseconds()
{
   return ossGetCurrentMicroseconds() / 1000L ;
}

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

   if ( 0 == clkTck )
   {
      clkTck = sysconf( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }

      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }

   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/stat",
                getpid() ) ;

   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
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

   if ( 0 == clkTck )
   {
      clkTck = sysconf( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }

      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }

   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/task/%lu/stat",
                getpid(),
               (unsigned long)tid ) ;

   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
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
#if defined (_PPCLIN64)
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
                                 "0x"OSS_PRIXPTR " : ", (UintPtr)cPtr) ;
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
                            "0x"OSS_PRIXPTR OSS_HEXDUMP_SPLITER,
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
               ossStrncpy(curPos, "*"OSS_NEWLINE, sizeof( "*"OSS_NEWLINE )) ;
               curPos += sizeof( "*"OSS_NEWLINE ) - sizeof( '\0' ) ;
               outBufSz -= sizeof( "*"OSS_NEWLINE ) - sizeof( '\0' ) ;
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
#define OSS_GET_MEM_INFO_SWAPTOTAL        "SwapTotal"
#define OSS_GET_MEM_INFO_SWAPFREE         "SwapFree"
#define OSS_GET_MEM_INFO_COMMITLIM        "CommitLimit"
#define OSS_GET_MEM_INFO_COMMITTED        "Committed_AS"
#define OSS_GET_MEM_INFO_AMPLIFIER        1024ll
#elif defined (_WINDOWS)
#define OSS_GET_MEM_INFO_AMPLIFIER        1024LL
#endif
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETMEMINFO, "ossGetMemoryInfo" )
INT32 ossGetMemoryInfo ( INT32 &loadPercent,
                         INT64 &totalPhys,    INT64 &availPhys,
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
   while ( fgets ( lineBuffer, OSS_PROC_PATH_LEN_MAX, fp ) &&
           ( totalPhys   == -1 ||
             availPhys   == -1 ||
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
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_MEMTOTAL )+1],
                  "%d", &inputNum ) ;
         totalPhys = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if ( ossStrncmp ( lineBuffer,
                             OSS_GET_MEM_INFO_MEMFREE,
                             ossStrlen ( OSS_GET_MEM_INFO_MEMFREE ) ) == 0 )
      {
         sscanf ( &lineBuffer[ossStrlen ( OSS_GET_MEM_INFO_MEMFREE )+1],
                  "%d", &inputNum ) ;
         availPhys = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
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

INT32 ossGetMemoryInfo ( INT32 &loadPercent,
                         INT64 &totalPhys,    INT64 &availPhys,
                         INT64 &totalPF,      INT64 &availPF,
                         INT64 &totalVirtual, INT64 &availVirtual )
{
   INT32 overCommitMode = 0 ;
   INT64 commitLimit = 0 ;
   INT64 committedAS = 0 ;

   return ossGetMemoryInfo ( loadPercent,
                             totalPhys, availPhys,
                             totalPF, availPF,
                             totalVirtual, availVirtual,
                             overCommitMode,
                             commitLimit, committedAS ) ;
}

#if defined (_LINUX) || defined (_AIX)
#define OSS_GET_DISK_INFO_FILE      "/etc/mtab"
#endif

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETDISKINFO, "ossGetDiskInfo" )
INT32 ossGetDiskInfo ( const CHAR *pPath, INT64 &totalBytes, INT64 &freeBytes,
                       CHAR* fsName, INT32 fsNameSize )
{
   INT32 rc                = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSGETDISKINFO );
#if defined (_WINDOWS)
   LPWSTR pszWString       = NULL ;
   LPSTR lpszVolumePath    = NULL ;
   DWORD dwString          = 0 ;
   BOOL success            = FALSE ;
   DWORD sectorsPerCluster = 0 ;
   DWORD bytesPerSector    = 0 ;
   DWORD freeClusters      = 0 ;
   DWORD totalClusters     = 0 ;
   INT64 availBytes        = 0 ;
   WCHAR volumePath[ OSS_MAX_PATHSIZE ] = L"" ;

   rc = ossANSI2WC ( pPath, &pszWString, &dwString ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to convert ansi to wc, rc = %d", rc );

   success = GetDiskFreeSpaceEx ( pszWString, (PULARGE_INTEGER) &availBytes,
                                 (PULARGE_INTEGER) &totalBytes,
                                 (PULARGE_INTEGER) &freeBytes ) ;

   PD_CHECK( success, SDB_SYS, error, PDERROR, "Failed to get disk space"
             ", errno: %d, rc = %d", ossGetLastError(), rc );

   success = GetDiskFreeSpace ( pszWString, &sectorsPerCluster, &bytesPerSector,
                                &freeClusters, &totalClusters ) ;
   PD_CHECK( success, SDB_SYS, error, PDERROR, "Failed to get disk free"
             "space , errno: %d, rc = %d", ossGetLastError(), rc );

   freeBytes = freeClusters * sectorsPerCluster * bytesPerSector ;
   totalBytes = totalClusters * sectorsPerCluster * bytesPerSector ;

   if ( NULL == fsName )
   {
      goto done ;
   }

   success = GetVolumePathName ( pszWString, volumePath, OSS_MAX_PATHSIZE+1) ;
   PD_CHECK( success, SDB_SYS, error, PDERROR, "Failed to get disk name"
             ", errno: %d, rc = %d", ossGetLastError(), rc );

   rc = ossWC2ANSI ( volumePath, &lpszVolumePath, NULL ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to convert ansi to wc, rc = %d", rc );

   ossStrncpy ( fsName, lpszVolumePath, fsNameSize - 1 ) ;
   fsName[ fsNameSize ] = '\0' ;

#elif defined (_LINUX) || defined (_AIX)
   struct statvfs vfs ;
   FILE *fp = NULL ;
   struct mntent *me = NULL ;
   struct mntent dummy ;
   CHAR tmpBuff[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   struct stat pathStat ;
   INT32 retcode = 0 ;
   BOOLEAN findOut = FALSE ;
   dev_t pathDevID ;

   retcode = statvfs ( pPath, &vfs ) ;
   PD_CHECK( 0 == retcode, SDB_SYS, error, PDERROR, "Failed to statvfs"
             ", errno: %d, rc = %d", ossGetLastError (), rc );

   totalBytes = vfs.f_frsize * vfs.f_blocks ;
   freeBytes = vfs.f_bsize * vfs.f_bfree ;

   if ( NULL == fsName )
   {
      goto done ;
   }

   retcode = stat ( pPath, &pathStat );
   PD_CHECK( 0 == retcode, SDB_SYS, error, PDERROR, "Failed to stat"
             ", errno: %d, rc = %d", ossGetLastError (), rc );
   pathDevID = pathStat.st_dev ;

   fp = setmntent ( OSS_GET_DISK_INFO_FILE, "r" ) ;
   PD_CHECK( NULL != fp, SDB_SYS, error, PDERROR, "Failed to set mnt entry"
             ", errno: %d, rc = %d", ossGetLastError (), rc );

   while ( NULL != ( me = getmntent_r ( fp, &dummy, tmpBuff, OSS_MAX_PATHSIZE ) ) )
   {
      struct stat fsStat ;
      INT32 retcode = stat ( me->mnt_fsname, &fsStat );
      if ( -1 == retcode )
      {
         PD_LOG ( PDINFO, "Failed to stat %s, errno: %d, rc = %d",
                  me->mnt_fsname, errno, rc ) ;
      }
      else
      {
         dev_t devID = fsStat.st_rdev ;
         if ( pathDevID == devID )
         {
            ossStrncpy( fsName, me->mnt_fsname, fsNameSize -1 ) ;
            fsName[ fsNameSize ] = '\0' ;
            findOut = TRUE ;
            break ;
         }
      }
   }

   if ( FALSE == findOut )
   {
      ossStrncpy( fsName, "unknown-disk", fsNameSize -1 ) ;
      fsName[ fsNameSize ] = '\0' ;
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
   while ( fgets ( lineBuffer, OSS_PROC_PATH_LEN_MAX, fp ) &&
           ( vmSize == -1 || vmRss == -1 ) )
   {
      if ( ossStrncmp ( lineBuffer, OSS_GET_PROC_MEM_VMSIZE,
                        ossStrlen ( OSS_GET_PROC_MEM_VMSIZE ) ) == 0 )
      {
         sscanf ( &lineBuffer[ ossStrlen( OSS_GET_PROC_MEM_VMSIZE ) + 1 ],
                  "%d", &inputNum ) ;
         vmSize = OSS_GET_MEM_INFO_AMPLIFIER * inputNum ;
      }
      else if ( ossStrncmp ( lineBuffer, OSS_GET_PROC_MEM_VMRSS,
                             ossStrlen ( OSS_GET_PROC_MEM_VMRSS ) ) == 0 )
      {
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
      goto error ;
   }
   pLinkedPath[len] = '\0' ;
#endif

done :
   PD_TRACE_EXITRC( SDB_OSSREADLINK, rc ) ;
   return rc ;
error :
   goto done ;
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
#endif

done :
   PD_TRACE_EXITRC ( SDB_OSSGETDISKIOSTAT, rc ) ;
   return rc ;

error :
   goto done ;
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
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETCPUINFO, "ossGetCPUInfo" )
INT32 ossGetCPUInfo ( SINT64 &user, SINT64 &sys,
                      SINT64 &idle, SINT64 &other )
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
   other = 0 ;
   idle /= 10000 ;
   sys /= 10000 ;
   sys -= idle ;
   user /= 10000 ;
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
   if ( 0 == clkTck )
   {
      clkTck = sysconf ( _SC_CLK_TCK ) ;
      if ( -1 == clkTck )
      {
         clkTck = OSS_CLK_TCK ;
      }
      numMicrosecPerClkTck = OSS_ONE_MILLION / clkTck ;
   }
   ossSnprintf ( pathName, sizeof(pathName), OSS_GET_CPU_INFO_FILE ) ;
   fp = fopen ( pathName, "r" ) ;
   if ( fp )
   {
      if ( !fgets ( buffer, OSS_PROC_PATH_LEN_MAX, fp ) )
      {
         ossErr = ossGetLastError () ;
         fclose ( fp ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      sscanf ( &buffer[4], OSS_GET_CPU_INFO_PATTERN,
               &userTime, &nicedTime, &systemTime,
               &idleTime, &waitTime, &irqTime, &softirqTime ) ;
      sys = systemTime / clkTck * 1000 +
            ( systemTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      user = ( userTime + nicedTime ) / clkTck * 1000 +
             ( ( userTime + nicedTime ) % clkTck ) * numMicrosecPerClkTck/1000;
      idle = idleTime / clkTck * 1000 +
             ( idleTime % clkTck ) * numMicrosecPerClkTck / 1000 ;
      otherTime = ( waitTime + irqTime + softirqTime ) ;
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
#if defined (_WINDOWS)
   HANDLE handle = GetCurrentProcess() ;
   PROCESS_MEMORY_COUNTERS pmc ;
   if ( GetProcessMemoryInfo( handle, &pmc, sizeof( pmc ) ) )
   {
      memInfo.rss = pmc.WorkingSetSize ;
      memInfo.vSize = pmc.PagefileUsage ;
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
   memInfo.fault = procInfo._majFlt ;
   PD_CHECK( procInfo._pid != -1, SDB_SYS, error, PDERROR,
            "failed to get process info(pid=%d)", pid ) ;
#else
   PD_RC_CHECK( SDB_SYS, PDERROR,
               "the OS is not supported!" ) ;
#endif
done:
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
_startTime(0),_vSize(0),_rss(-1),_rssRlim(0),
_startCode(0),_endCode(0),_startStack(0),
_kstkEsp(0),_kstkEip(0)
{
   ossMemset( _comm, 0, OSS_MAX_PATHSIZE + 1 ) ;
   CHAR pathName[ OSS_PROC_PATH_LEN_MAX + 1 ] = {0} ;
   ossSnprintf( pathName, sizeof(pathName), "/proc/%d/stat", pid ) ;
   FILE *fp = NULL ;
   fp = fopen( pathName, "r" ) ;
   if ( fp )
   {
      INT32 rc = 0;
      rc = fscanf( fp,
                  "%d %s %c "             //&_pid, _comm, &_state,
                  "%d %d %d %d %d "
                  "%u %u %u %u %u "  //&_flags, &_minFlt, &_cMinFlt, &_majFlt, &_cMajFlt,
                  "%u %u %d %d "
                  "%d %d "
                  "%d "                  //&_nlwp,
                  "%u "
                  "%u "
                  "%u "                  //&_vSize,
                  "%d "
                  "%u %u %u %u %u %u ",
                  &_pid, _comm, &_state,
                  &_ppid, &_pgrp, &_session, &_tty, &_tpgid,
                  &_flags, &_minFlt, &_cMinFlt, &_majFlt, &_cMajFlt,
                  &_uTime, &_sTime, &_cuTime, &_csTime,
                  &_priority, &_nice,
                  &_nlwp,
                  &_alarm,
                  &_startTime,
                  &_vSize,
                  &_rss,
                  &_rssRlim, &_startCode, &_endCode, &_startStack, &_kstkEsp, &_kstkEip );
      fclose(fp) ;
      if ( rc <= 0 )
      {
         PD_LOG( PDERROR, "failed to read proc-info" );
      }
   }
   else
   {
      PD_LOG( PDWARNING, "open failed(%s)",
            pathName );
   }
}
#endif

ossIPInfo::ossIPInfo()
:_ipNum(0), _ips(NULL)
{
   INT32 rc = _init() ;
   if ( SDB_OK != rc )
   {
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

   buf = (struct ifreq*)SDB_OSS_MALLOC( ifc.ifc_len ) ;
   if ( NULL == buf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   ifc.ifc_req = buf ;
   rc = ioctl( sock, SIOCGIFCONF, &ifc ) ;
   if ( 0 != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   _ipNum = ifc.ifc_len / sizeof( struct ifreq ) ;
   _ips = (ossIP*)SDB_OSS_MALLOC( _ipNum * sizeof(ossIP) ) ;
   if ( NULL == _ips )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( _ips, 0, _ipNum * sizeof(ossIP) ) ;

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

   _ipNum = bufLen / sizeof( IP_ADAPTER_INFO ) ;
   _ips = (ossIP*)SDB_OSS_MALLOC( _ipNum * sizeof(ossIP) ) ;
   if ( NULL == _ips )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( _ips, 0, _ipNum * sizeof(ossIP) ) ;

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

INT32 ossProcLimits::setLimit( const CHAR *str, INT64 soft, INT64 hard )
{
   INT32 rc = 0 ;
   soft = ( -1 == soft ) ? RLIM_INFINITY : soft ;
   hard = ( -1 == hard ) ? RLIM_INFINITY : hard ;
   struct rlimit lim = { soft, hard } ;
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

BOOLEAN& ossGetSignalShieldFlag()
{
   static OSS_THREAD_LOCAL BOOLEAN s_signalShieldFlag = FALSE ;
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
   ossGetSignalShieldFlag() = TRUE ;
}

ossSignalShield::~ossSignalShield()
{
   close() ;
}

void ossSignalShield::close()
{
   ossGetSignalShieldFlag() = FALSE ;

#if defined (_LINUX)
   if ( ossGetPendingSignal() > 0 &&
        SIGPIPE != ossGetPendingSignal() )
   {
      ossPThreadKill( ossPThreadSelf(), ossGetPendingSignal() ) ;
   }
#endif // _LINUX
   ossGetPendingSignal() = 0 ;
}
