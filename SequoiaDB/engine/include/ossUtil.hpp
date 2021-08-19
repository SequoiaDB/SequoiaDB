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

   Source File Name = ossUtil.hpp

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
#ifndef OSSUTIL_HPP_
#define OSSUTIL_HPP_
#include "core.hpp"
#include "oss.hpp"
#include <ctime>
#include <time.h>
#include <sys/types.h>
#include "ossUtil.h"
#include <string>
#include <map>

#if defined( SDB_ENGINE )
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>
#endif // SDB_ENGINE

UINT32 ossRand () ;
// djb2 hashing algorithm
OSS_INLINE UINT32 ossHash ( const CHAR *str )
{
   UINT32 hash = 5381 ;
   CHAR c ;
   while ( (c = *(str++)) )
      hash = ((hash << 5) + hash) + c;
   return hash ;
}

OSS_INLINE UINT32 ossHash( const CHAR *value, UINT32 size, UINT32 bit = 5)
{
   UINT32 hash = 5381 ;
   const CHAR *end = value + size ;
   while ( value < end )
      hash = ( (hash << bit) + hash + *value++ ) ;
   return hash ;
}

OSS_INLINE UINT32 ossHash( const BYTE *v1, UINT32 s1,
                           const BYTE *v2, UINT32 s2 )
{
   UINT32 hash = 5381 ;
   for ( UINT32 i = 0; i < s1; ++i )
   {
      hash = ( (hash << 5) + hash + *v1++ ) ;
   }

   for ( UINT32 i = 0; i < s2; ++i )
   {
      hash = ( (hash << 5) + hash + *v2++ ) ;
   }

   return hash ;
}

BOOLEAN ossIsPowerOf2( UINT32 num, UINT32 *pSquare = NULL ) ;
UINT64 ossNextPowerOf2( UINT32 num, UINT32 *pSquare = NULL ) ;

#if defined (_WINDOWS)
OSS_INLINE void ossSleepmillis ( UINT64 s )
{
   Sleep ( ( DWORD ) s ) ;
}
// sad truth in windows is that we don't have nansleep nor usleep,
// so we need to depends on boost again
OSS_INLINE void ossSleepmicros(UINT64 s)
{
#if defined( SDB_ENGINE )
   boost::xtime xt ;
   boost::xtime_get ( &xt, boost::TIME_UTC_ ) ;
   xt.sec += (INT32)(s/1000000);
   xt.nsec += (int)((s%1000000)*1000);
   if(xt.nsec>=1000000000)
   {
      xt.nsec-=1000000000;
      xt.sec++;
   }
   boost::thread::sleep(xt);
#else
   ossSleepmillis( s / 1000 ) ;
#endif // SDB_ENGINE
}
#else
// we use nanosleep instead of usleep because we want to capture signal during sleep
OSS_INLINE void ossSleepmicros(UINT64 s)
{
   struct timespec t;
   t.tv_sec = (time_t)(s / 1000000);
   t.tv_nsec = 1000 * ( s % 1000000 );
   // nanosleep can be interrupted by signal, the remaining time is put in rem
   while(nanosleep( &t , &t )==-1 && ossGetLastError()==EINTR);
}
OSS_INLINE void ossSleepmillis(UINT64 s)
{
   ossSleepmicros( s * 1000 );
}
#endif
OSS_INLINE void ossSleepsecs(UINT32 s)
{
   ossSleepmicros((UINT64)s * 1000000);
}

class ossTime : public SDBObject
{
public :
   UINT32 seconds ;
   UINT32 microsec ;
} ;
typedef class ossTime ossTime ;

class ossTimestamp : public SDBObject
{
public :
   time_t time ;     // tv_sec ,  seconds
   UINT32 microtm ;  // tv_usec,  microseconds

   ossTimestamp ()
   : time( 0 ),
     microtm( 0 )
   {
   }

   ossTimestamp ( UINT64 curTime )
   : time( curTime / 1000 ),
     microtm( ( curTime % 1000 ) * 1000 )
   {
   }

   ossTimestamp ( const ossTimestamp & timestamp )
   : time( timestamp.time ),
     microtm( timestamp.microtm )
   {
   }

   ossTimestamp &operator= ( const ossTimestamp &rhs )
   {
      time    = rhs.time ;
      microtm = rhs.microtm ;
      return *this ;
   }

   void clear ()
   {
      time = 0 ;
      microtm = 0 ;
   }
} ;
typedef class ossTimestamp ossTimestamp ;


// timestamp format: yyyy-mm-dd-hh.mm.ss.uuuuuu
#define OSS_TIMESTAMP_STRING_LEN 26

// convert ossTimestamp into local calendar time string
void ossTimestampToString( ossTimestamp &Tm, CHAR * pStr ) ;

// convert ossTimestamp into UTC calendar time string
void ossTimestampToUTCString( ossTimestamp &Tm, CHAR * pStr ) ;

// convert time_t from local to UTC in the same DateString
// for example:
//   [in] local timezone:CST, date:"2019-08-06 20:13:54", local:1565093634
//   [out] utc  timezone:UTC, date:"2019-08-06 20:13:54", utc:  1565122434
// quick check:
// 1. echo "Asia/Shanghai" > /etc/timezone && cat /usr/share/zoneinfo/Asia/Shanghai > /etc/localtime
// 2. date -d@1565093634  ==> Tue Aug  6 20:13:54 CST 2019
// 3. echo "UTC" > /etc/timezone && cat /usr/share/zoneinfo/UTC > /etc/localtime
// 4. date -d@1565122434  ==> Tue Aug  6 20:13:54 UTC 2019
void ossTimeLocalToUTCInSameDate( const time_t &local, time_t &utc ) ;

time_t ossTimeDiffWithUTC() ;

INT32 ossTimeGetMaxDay( INT32 year, INT32 month ) ;

// convert string into ossTimestamp
void ossStringToTimestamp( const CHAR * pStr, ossTimestamp &Tm ) ;

// convert string into ossTimestamp with parse number
void ossStringToTimestamp( const CHAR * pStr, ossTimestamp &Tm,
                           INT32 &parseNum ) ;

// Wrapper of localtime, convert a time value and correct for the local time
// zone. The input pTime represents the seconds elapsed since the Epoch,
// midnight (00:00:00), January 1, 1970, UTC
void ossLocalTime ( time_t &Time, struct tm &TM ) ;

//  Wrapper of gmtime, converts a time value to a broken-down time structure.
//  The pTime is represented as seconds elapsed since the Epoch.
void ossGmtime ( time_t &Time, struct tm &TM ) ;

// Get current time, expressed as seconds and microseconds since the Epoch
void ossGetCurrentTime( ossTimestamp &TM ) ;

UINT64 ossGetCurrentMicroseconds() ;
UINT64 ossTimestampToMicroseconds( const ossTimestamp &timestamp ) ;
ossTimestamp ossMicrosecondsToTimestamp( const UINT64 &microseconds ) ;
UINT64 ossGetCurrentMilliseconds() ;

SINT32 ossGetCPUUsage
(
   ossTime &usrTime,
   ossTime &sysTime
) ;

SINT32 ossGetCPUUsage
(
#if defined (_WINDOWS)
   HANDLE tHandle,
#elif defined (_LINUX) || defined (_AIX)
   OSSTID tid,
#endif
   ossTime &usrTime,
   ossTime &sysTime
) ;

/*
   Get Operator System Infomation
*/
struct _ossOSInfo
{
   CHAR _distributor[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR _release[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR _desp[ OSS_MAX_PATHSIZE + 1 ] ;
   INT32 _bit ;
} ;
typedef _ossOSInfo ossOSInfo ;

INT32 ossGetOSInfo( ossOSInfo &info ) ;

OSS_INLINE UINT64 ossPack32To64( UINT32 hi, UINT32 lo )
{
   return ((UINT64)hi << 32) | (UINT64)lo ;
}

OSS_INLINE void ossUnpack32From64 ( UINT64 u64, UINT32 & hi, UINT32 & lo )
{
   hi = (UINT32)( u64 >> 32 ) ;
   lo = (UINT32)( u64 & 0xFFFFFFFF ) ;
}


///////////////////////////////////////////////////////////////////////////////
//  OSS Ticks
///////////////////////////////////////////////////////////////////////////////
//
// OSS Timestamp utilities
//
// Sample code illustrating usage interval timer and how interval can be
// used / convert to normal time unites.
//
//    ossTick tk1, tk2 ;
//    ossTickDelta diff ;
//    UINT32 sec, microsec ;
//    ossTickConversionFactor factor ;
//
//    tk1.sample() ;
//    ...... do workloads
//    tk2.sample() ;
//
//    diff = tk2 - tk1 ;
//    diff.convertToTime( factor, sec, microsec ) ;
//
//    printf("elapsed time : %dSeconds %dMicroseconds\n", sec, microsec ) ;
//
//

// The Time Stamp Counter is a 64bit register present on all x86 processors
// since Pentium. It counts the number of cycles since reset. The time stamp
// counter has, until recently, been an excellent high-resolution, low-overhead
// way of getting CPU timing information. With the advent of multi-core CPUs,
// systems with multiple CPUs, and "hibernating" operating systems, the TSC
// cannot be relied on to provide accurate results unless great care is taken
// to correct the possible flaws: rate of tick and whether all cores (
// processors ) have identical values in their time-keeping registers. There
// is no promise that the timestamp counters of multiple CPUs on a single
// motherboard will be synchronized. In such cases, programmers can only get
// reliable results by locking their code to a single CPU.

// In our ossTick, we use the most portable method gettimeofday( Linux ) and
// QueryPerformanceCounter( windows ) to get the 'timestamp', then convert
// and save it as 'ticks' into a 64bit integer. We also provide, ossTickDelta,
// ossTickConversionFactor for tick's operation and calculation.

// The ossRdtsc is also provided as single API, which may be used/fit for
// some specific condition in the future.
#if defined (_LINUX)
OSS_INLINE UINT64 ossRdtsc()
{
   UINT32 lo, hi;
   __asm__ __volatile__ (
     " xorl %%eax,%%eax \n"
     " cpuid"
     ::: "%rax", "%rbx", "%rcx", "%rdx" ) ;
   __asm__ __volatile__ ( "rdtsc" : "=a" (lo), "=d" (hi) ) ;
   return (UINT64)( ((UINT64) lo) | ( ((UINT64) hi) << 32 ) );
}
#elif defined (_WINDOWS)
   #define  ossRdtsc()  __rdtsc()
#endif

#if defined (_LINUX) || defined (_AIX)
OSS_INLINE void ossGetTimeOfDay( struct timeval * pTV )
{
   if ( pTV )
   {
      if ( -1 == gettimeofday( pTV, NULL ) )
      {
         pTV->tv_sec = 0 ;
         pTV->tv_usec = 0 ;
      }
   }
   return ;
}

OSS_INLINE UINT64 ossTimeValToUint64( struct timeval & tv )
{
   return ossPack32To64( (UINT32)tv.tv_sec, (UINT32)tv.tv_usec ) ;
}
#endif

#define OSS_TICKS_OP_ADD 1
#define OSS_TICKS_OP_SUB 2
#define OSS_TICKS_OP_NO_SCALE 4

// class ossTickCore is base class of ossTick and ossTickDelta,
// it is primitive and for the tick storage only.
// The mininum measure unit is microsecond.
class ossTickCore : public SDBObject
{
public :
   ossTickCore()
   {
#if defined (_WINDOWS)
      _value.QuadPart = 0 ;
#else
      _value = 0 ;
#endif
   }

   ossTickCore ( const ossTickCore & tick )
   {
#if defined (_WINDOWS)
      _value.QuadPart = tick._value.QuadPart ;
#else
      _value = tick._value ;
#endif
   }

#if defined (_WINDOWS)
   LARGE_INTEGER _value ;
#else
   UINT64 _value ;
#endif

   OSS_INLINE UINT64 peek(void) const
   {
   #if defined (_WINDOWS)
      return _value.QuadPart ;
   #else
      return _value ;
   #endif
   } ;

   OSS_INLINE void poke(const UINT64 val)
   {
   #if defined (_WINDOWS)
      _value.QuadPart = val ;
   #else
      _value = val ;
   #endif
   } ;

   static UINT64 addOrSub
   (
      const ossTickCore op1, const ossTickCore op2, const UINT32 flags
   )
   {
      UINT64 val ;
   #if defined (_WINDOWS)
      if ( OSS_TICKS_OP_ADD & flags )
      {
         val = op1.peek() + op2.peek() ;
      }
      else
      {
         val = op1.peek() - op2.peek() ;
         if ( ( OSS_TICKS_OP_SUB & flags ) && ( (SINT64)val < 0 ) )
         {
            val = 0 ;
         }
      }
   #else
      val = addOrSub( op1._value, op2._value, flags ) ;
   #endif
      return val ;
   } ;


   // calculate the difference or sum between two ticks values( represented by
   // a 64bit integer ). On Linux platform, the timeval is stored as seconds
   // and microseconds pair
   static UINT64 addOrSub
   (
      const UINT64 op1, const UINT64 op2, const UINT32 flags
   )
   {
   #if defined (_WINDOWS)
      UINT64 result ;
      if ( OSS_TICKS_OP_ADD & flags )
      {
         result = op1 + op2 ;
      }
      else
      {
         result = op1 - op2 ;
         if ( ( OSS_TICKS_OP_SUB & flags ) && ( (SINT64)result < 0 ) )
         {
            result = 0 ;
         }
      }
      return result ;
   #elif defined (_LINUX) || defined (_AIX)
      SINT64 resultHi, resultLo ;

      if ( OSS_TICKS_OP_ADD & flags )
      {
         resultHi = (SINT64)(op1 >> 32) + (SINT64)(op2 >> 32) ;
         resultLo = (SINT64)(op1 & 0xFFFFFFFF) + (SINT64)(op2 & 0xFFFFFFFF) ;
         if ( resultLo >= OSS_ONE_MILLION )
         {
            resultHi++ ;
            resultLo -= OSS_ONE_MILLION ;
         }
      }
      else
      {
         resultHi = (SINT64)(op1 >> 32) - (SINT64)(op2 >> 32) ;
         resultLo = (SINT64)(op1 & 0xFFFFFFFF) - (SINT64)(op2 & 0xFFFFFFFF ) ;

         if ( resultLo < 0 )
         {
            resultHi-- ;
            resultLo += OSS_ONE_MILLION ;
         }
         if ( ( OSS_TICKS_OP_SUB & flags ) && resultHi < 0 )
         {
            resultHi = 0 ;
            resultLo = 0 ;
         }
      }
      return (UINT64)( ((UINT64)resultHi << 32) | (UINT64)resultLo  ) ;
   #endif
   } ;
} ;

class ossTickDelta ;
class ossTick ;

// Used for saving the conversion factor required to convert ossTick
// into time units
// A tick value is only valid to the machine where it was taken from,
// this class allows for saving and setting such a conversion factor so
// ticks can be interpreted at later time ( even on different machine )
class ossTickConversionFactor : public SDBObject
{
protected :
   friend class ossTickDelta ;
   UINT64 factor ;
public :
   ossTickConversionFactor() ;
} ;

ossTickDelta operator - (const ossTick      &x, const ossTick      &y) ;
ossTickDelta operator - (const ossTickDelta &x, const ossTickDelta &y) ;

ossTickDelta operator + (const ossTickDelta &x, const ossTickDelta &y) ;
ossTick operator + (const ossTick &x, const ossTickDelta &y) ;
ossTick operator + (const ossTickDelta &x, const ossTick &y) ;

BOOLEAN operator >  (const ossTickDelta &x, const ossTickDelta &y) ;
BOOLEAN operator >= (const ossTickDelta &x, const ossTickDelta &y) ;
BOOLEAN operator <  (const ossTickDelta &x, const ossTickDelta &y) ;
BOOLEAN operator <= (const ossTickDelta &x, const ossTickDelta &y) ;
BOOLEAN operator == (const ossTickDelta &x, const ossTickDelta &y) ;

BOOLEAN operator >  (const ossTick &x, const ossTick &y) ;
BOOLEAN operator >= (const ossTick &x, const ossTick &y) ;
BOOLEAN operator <  (const ossTick &x, const ossTick &y) ;
BOOLEAN operator <= (const ossTick &x, const ossTick &y) ;
BOOLEAN operator == (const ossTick &x, const ossTick &y) ;

// ossTickDelta represents the interval( ticks delta's internal representation )
// of two tick values
class ossTickDelta : protected ossTickCore
{
   friend ossTickDelta operator- (const ossTick      &x, const ossTick      &y);
   friend ossTickDelta operator- (const ossTickDelta &x, const ossTickDelta &y);
   friend ossTickDelta operator+ (const ossTickDelta &x, const ossTickDelta &y);

   friend BOOLEAN operator >  (const ossTickDelta &x, const ossTickDelta &y) ;
   friend BOOLEAN operator >= (const ossTickDelta &x, const ossTickDelta &y) ;
   friend BOOLEAN operator <  (const ossTickDelta &x, const ossTickDelta &y) ;
   friend BOOLEAN operator <= (const ossTickDelta &x, const ossTickDelta &y) ;
   friend BOOLEAN operator == (const ossTickDelta &x, const ossTickDelta &y) ;

   friend ossTick operator +  (const ossTick      &x, const ossTickDelta &y) ;
   friend ossTick operator +  (const ossTickDelta &x, const ossTick      &y) ;
public :
   ossTickDelta ()
   : ossTickCore()
   {
   }

   ossTickDelta ( const ossTickDelta & delta )
   : ossTickCore( delta )
   {
   }

   // set the tick value to zero
   OSS_INLINE void clear(void)
   {
      poke( 0 ) ;
   } ;

   // test tick for non-zero
   OSS_INLINE operator BOOLEAN() const
   {
      return ( 0 != peek() ) ;
   } ;

   OSS_INLINE UINT64 peek(void) const
   {
      return ossTickCore::peek() ;
   } ;

   OSS_INLINE void poke(const UINT64 val)
   {
      ossTickCore::poke( val ) ;
   } ;

   // retrive _value as a UINT64
   OSS_INLINE UINT64 toUINT64() const
   {
      UINT64 val = peek() ;
   #if defined (_LINUX) || defined (_AIX)
      UINT32 hi, lo ;

      hi = (UINT32)( val >> 32 ) ;
      lo = (UINT32)( val & 0xFFFFFFFF ) ;

      // we use gettimeofday on Linux platform, which uses seconds and
      // microseconds to represent the ticks.
      val = ( UINT64 )hi * OSS_ONE_MILLION + lo ;
   #endif
      return val ;
   } ;

   // cast to UINT64
   OSS_INLINE operator UINT64() const
   {
      return toUINT64() ;
   } ;

   // store a tick delta value into the internal value, i.e., _value
   // here we expect the v is accquired by QueryPerformanceCounter ( windows )
   // gettimeofday(Linux)
   OSS_INLINE void fromUINT64( const UINT64 v )
   {
   #if defined (_WINDOWS)
      poke( v ) ;
   #elif defined(_LINUX) || defined (_AIX)
      UINT32 hi, lo ;
      UINT64 val ;

      // On Linux we use gettimeofday for sampling, and ticks is represents
      // as seconds and microseconds pair. We assume that v is a 64bit
      // presenting as number of microseconds.
      hi = v / OSS_ONE_MILLION ;
      lo = v % OSS_ONE_MILLION ;
      val= ((UINT64)hi << 32) | (UINT64)lo ;

      poke( val ) ;
   #endif
   } ;

   // add a tick delta to an existing tick interval.
   OSS_INLINE ossTickDelta & operator += ( const ossTickDelta & x )
   {
      poke( ossTickCore::addOrSub( *this, x, OSS_TICKS_OP_ADD ) ) ;
      return *this ;
   } ;

   // subtract a tick delta from an existing tick interval.
   OSS_INLINE ossTickDelta & operator -= ( const ossTickDelta & x )
   {
      poke( ossTickCore::addOrSub( *this, x, OSS_TICKS_OP_SUB ) ) ;
      return *this ;
   } ;

   OSS_INLINE ossTickDelta & operator= ( const ossTickDelta &rhs )
   {
      poke( rhs.peek() ) ;
      return *this ;
   }

   // convert into time units ( seconds and microseconds )
   OSS_INLINE void convertToTime
   (
      const ossTickConversionFactor &  cFactor,
      UINT32 &                         seconds,
      UINT32 &                         microseconds
   ) const
   {
      UINT64 ticks = peek() ;
   #if defined (_WINDOWS)
      FLOAT64 microsecondsInTotal = (FLOAT64)(ticks * OSS_ONE_MILLION /
                                              cFactor.factor) ;

      seconds = (UINT32)( microsecondsInTotal / OSS_ONE_MILLION ) ;
      microseconds = (UINT32)(microsecondsInTotal - seconds * OSS_ONE_MILLION) ;
   #elif defined (_LINUX) || defined (_AIX)
      ossUnpack32From64( ticks, seconds, microseconds ) ;
   #endif
   } ;

   // convert a time value( microseconds ) to a tick equivalent.
   OSS_INLINE SINT32 initFromTimeValue
   (
      const ossTickConversionFactor &  cFactor,
      const UINT64                     timeValueInMicroseconds
   )
   {
      SINT32 rc = SDB_OK ;
      UINT64 numTicksForInterval ;
   #if defined (_WINDOWS)
      UINT64 maxValue = OSS_UINT64_MAX / cFactor.factor ;

      if ( timeValueInMicroseconds > maxValue )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         numTicksForInterval = timeValueInMicroseconds * cFactor.factor ;
      }
   #elif defined (_LINUX) || defined (_AIX)
      UINT32 hi, lo ;

      hi = timeValueInMicroseconds / OSS_ONE_MILLION ;
      lo = timeValueInMicroseconds % OSS_ONE_MILLION ;
      numTicksForInterval = ossPack32To64( hi, lo ) ;
   #endif
      poke( numTicksForInterval ) ;
      return rc ;
   } ;
} ;


class ossTick : protected ossTickCore
{
   friend BOOLEAN operator >  (const ossTick &x, const ossTick &y ) ;
   friend BOOLEAN operator >= (const ossTick &x, const ossTick &y ) ;
   friend BOOLEAN operator <  (const ossTick &x, const ossTick &y ) ;
   friend BOOLEAN operator <= (const ossTick &x, const ossTick &y ) ;
   friend BOOLEAN operator == (const ossTick &x, const ossTick &y ) ;
   friend ossTick operator +  (const ossTick &x,      const ossTickDelta &y ) ;
   friend ossTick operator +  (const ossTickDelta &x, const ossTick &y ) ;
   friend ossTickDelta operator - (const ossTick &x, const ossTick &y ) ;
public :
   ossTick ()
   : ossTickCore()
   {
   }

   ossTick ( const ossTick & tick )
   : ossTickCore( tick )
   {
   }

   // Get a timestamp represented by tick number.
   // Tick values are only valid relative to each other, and cannot be
   // converted to time of day/date values. Conversion to time units can be
   // done via convertToTime.
   OSS_INLINE void sample(void)
   {
   #if defined (_WINDOWS)
      if ( ! QueryPerformanceCounter( & _value ) )
      {
          _value.QuadPart = 0L ;
      }
   #else
      struct timeval tv ;

      tv.tv_sec = 0 ;
      tv.tv_usec = 0 ;
      gettimeofday( &tv, NULL ) ;
      _value = ((UINT64)tv.tv_sec << 32) | (UINT64)tv.tv_usec ;
   #endif
   } ;

   // Set the tick value to zero
   OSS_INLINE void clear(void)
   {
      poke( 0 ) ;
   } ;

   // Test tick for non-zero
   OSS_INLINE operator BOOLEAN() const
   {
      return (0 != peek()) ;
   } ;


   OSS_INLINE ossTick & operator= ( const ossTick &rhs )
   {
      poke( rhs.peek() ) ;
      return *this ;
   }

   SINT32 initFromTimeValue
   (
      const ossTickConversionFactor &  cFactor,
      const UINT64                     timeValueInMicroseconds
   )
   {
      return ((ossTickDelta*)this)->initFromTimeValue( cFactor,
                                                       timeValueInMicroseconds);
   } ;

   void convertToTime
   (
      const ossTickConversionFactor &  cFactor,
      UINT32 &                         seconds,
      UINT32 &                         microseconds
   ) const
   {
      ((ossTickDelta*)this)->convertToTime( cFactor, seconds, microseconds ) ;
   } ;

   // convert into timestamp
   void convertToTimestamp( ossTimestamp &Tm ) const
   {
      UINT64 ticks = peek() ;
      Tm.time = (time_t)( ticks >> 32 ) ;
      Tm.microtm = ticks & 0x00000000ffffffff ;
   }

   static UINT64 addOrSub
   (
      const UINT64 op1, const UINT64 op2, const UINT32 flags
   )
   {
      return ossTickCore::addOrSub( op1, op2, flags ) ;
   } ;
} ;


OSS_INLINE BOOLEAN operator > (const ossTickDelta &x, const ossTickDelta &y )
{
#if defined (_WINDOWS)
   return ( x.peek() > y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) > 0 ;
#endif
}

OSS_INLINE BOOLEAN operator >= (const ossTickDelta &x, const ossTickDelta &y )
{
#if defined (_WINDOWS)
   return ( x.peek() >= y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) >= 0 ;
#endif
}

OSS_INLINE BOOLEAN operator < (const ossTickDelta &x, const ossTickDelta &y )
{
#if defined (_WINDOWS)
   return ( x.peek() < y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) < 0 ;
#endif
}

OSS_INLINE BOOLEAN operator <= (const ossTickDelta &x, const ossTickDelta &y )
{
#if defined (_WINDOWS)
   return ( x.peek() <= y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) <= 0 ;
#endif
}

OSS_INLINE BOOLEAN operator == (const ossTickDelta &x, const ossTickDelta &y )
{
#if defined (_WINDOWS)
   return ( x.peek() == y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) == 0 ;
#endif
}

OSS_INLINE BOOLEAN operator > (const ossTick &x, const ossTick &y)
{
#if defined (_WINDOWS)
   return ( x.peek() > y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) > 0 ;
#endif
}

OSS_INLINE BOOLEAN operator >= (const ossTick &x, const ossTick &y)
{
#if defined (_WINDOWS)
   return ( x.peek() >= y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) >= 0 ;
#endif
}

OSS_INLINE BOOLEAN operator < (const ossTick &x, const ossTick &y)
{
#if defined (_WINDOWS)
   return ( x.peek() < y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) < 0 ;
#endif
}

OSS_INLINE BOOLEAN operator <= (const ossTick &x, const ossTick &y)
{
#if defined (_WINDOWS)
   return ( x.peek() <= y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) <= 0 ;
#endif
}

OSS_INLINE BOOLEAN operator == (const ossTick &x, const ossTick &y)
{
#if defined (_WINDOWS)
   return ( x.peek() == y.peek() ) ;
#elif defined (_LINUX) || defined (_AIX)
   return (SINT64)(ossTickCore::addOrSub(x, y, OSS_TICKS_OP_NO_SCALE)) == 0 ;
#endif
}

OSS_INLINE ossTickDelta operator + (const ossTickDelta &x, const ossTickDelta &y )
{
   ossTickDelta result ;

   result.poke(  ossTickCore::addOrSub( x, y, OSS_TICKS_OP_ADD ) ) ;
   return result ;
}

OSS_INLINE ossTick operator + (const ossTick &x, const ossTickDelta &y)
{
   ossTick result ;

   result.poke( ossTickCore::addOrSub( x,y, OSS_TICKS_OP_ADD ) ) ;
   return result ;
}

OSS_INLINE ossTick operator + (const ossTickDelta &x, const ossTick &y)
{
   ossTick result ;

   result.poke( ossTickCore::addOrSub( x,y, OSS_TICKS_OP_ADD ) ) ;
   return result ;
}

OSS_INLINE ossTickDelta operator - (const ossTickDelta &x, const ossTickDelta &y )
{
   ossTickDelta result ;

   result.poke( ossTickCore::addOrSub( x, y, OSS_TICKS_OP_SUB ) ) ;
   return result ;
}

OSS_INLINE ossTickDelta operator - (const ossTick &x, const ossTick &y )
{
   ossTickDelta result ;

   result.poke( ossTickCore::addOrSub( x, y, OSS_TICKS_OP_SUB ) ) ;
   return result ;
}


typedef UINT32_64 UintPtr ;

#if defined OSS_ARCH_64
   #define OSS_PRIxPTR "%016lx"
   #define OSS_PRIXPTR "%016lX"
#elif defined ( OSS_ARCH_32 )
   #define OSS_PRIxPTR "%08lx"
   #define OSS_PRIXPTR "%08lX"
#endif

#define OSS_INT8_MAX_HEX_STRING   "0xFF"
#define OSS_INT16_MAX_HEX_STRING  "0xFFFF"
#define OSS_INT32_MAX_HEX_STRING  "0xFFFFFFFF"
#define OSS_INT64_MAX_HEX_STRING  "0xFFFFFFFFFFFFFFFF"

#if defined OSS_ARCH_64
   #define OSS_INTPTR_MAX_HEX_STRING   OSS_INT64_MAX_HEX_STRING
#elif defined ( OSS_ARCH_32 )
   #define OSS_INTPTR_MAX_HEX_STRING   OSS_INT32_MAX_HEX_STRING
#endif

#define OSS_HEXDUMP_SPLITER " : "
#define OSS_HEXDUMP_BYTES_PER_LINE 16
#define OSS_HEXDUMP_ADDRESS_SIZE  (   sizeof( OSS_INTPTR_MAX_HEX_STRING \
                                              OSS_HEXDUMP_SPLITER )     \
                                    - sizeof( '\0' ) )
#define OSS_HEXDUMP_HEX_LEN       (   ( OSS_HEXDUMP_BYTES_PER_LINE << 1 )   \
                                    + ( OSS_HEXDUMP_BYTES_PER_LINE >> 1 ) )
#define OSS_HEXDUMP_SPACES_IN_BETWEEN  2
#define OSS_HEXDUMP_START_OF_DATA_DISP (   OSS_HEXDUMP_HEX_LEN              \
                                         + OSS_HEXDUMP_SPACES_IN_BETWEEN )
#define OSS_HEXDUMP_LINEBUFFER_SIZE  (   OSS_HEXDUMP_ADDRESS_SIZE       \
                                       + OSS_HEXDUMP_START_OF_DATA_DISP \
                                       + OSS_HEXDUMP_BYTES_PER_LINE     \
                                       + sizeof(OSS_NEWLINE) )
#define OSS_HEXDUMP_NULL_PREFIX    ((CHAR *) NULL)

#define OSS_HEXDUMP_INCLUDE_ADDR    1
#define OSS_HEXDUMP_RAW_HEX_ONLY    2
#define OSS_HEXDUMP_PREFIX_AS_ADDR  4

// Hex dump one line into a buffer
// It converts each word into two 8-bit ASCII characters to a buffer
// from the given input address. If the 8-bit ASCII equivalent
// is unprintable a '.' is output instead.
//   inPtr[in]
//      The address of the memory to format
//   len[in]
//      The number of characters to format. It must be less or equal than
//      OSS_HEXDUMP_BYES_PER_LINE, i.e. 16 bytes.
//   szOutBuf[out]
//      An output buffer. The size of this bufer must be great or eual than
//      OSS_HEXDUMP_LINE_BUFFER_SIZE
//   flags[in]
//      A bitmask of the following :
//      - OSS_HEXDUMP_INCLUDE_ADDR
//        When set, the address of the buffer is displayed in the output
//        e.g., "0x12345678 : "
//      - OSS_HEXDUMP_RAW_HEX_ONLY
//        When set, only display the raw hex value, the ASCII
//        representation will not be displayed.
//   Return value:
//       The offset of last character written into the output buffer( not
//       including the NULL terminator )
UINT32 ossHexDumpLine
(
   const void *   inPtr,
   UINT32         len,
   CHAR *         szOutBuf,
   UINT32         flags
) ;


// Hex dump into a buffer
//    Similar as ossHexDumpLine, but it takes bigger size buffer,
//    and dumps all the data provided till the fill the output buffer size.
//    If a line is same as previous line, a '*' will be printed instead
//    of duplicate the whole line. When mulitple lines are same, only one
//    '*' will be printed, e.g.
//      ...
//      0x004B : 6F66 2053 6571 756F 6961 4442 2049 6E63    of SequoiaDB Inc
//      0x005B : 2E0D 0A0D 0A20 2020 5468 6520 736F 7572    .....   The sour
//      *
//      0x009B : 2070 726F 6772 616D 2069 7320 6E6F 7420     program is not
//      ...
//
// inPtr [in]
//    The address of the memory to format.
// len [in]
//    The number of characters to format.
// szOutBuf [out]
//    The output buffer.
// outBufSz [in]
//    The size of the output buffer.
// szPrefix [in]
//    String prefix for the indentation
// flags [in]
//      A bitmask of the following :
//      - OSS_HEXDUMP_INCLUDE_ADDR
//        When set, the address of the buffer is displayed in the output
//        e.g., "0x12345678 : "
//      - OSS_HEXDUMP_PREFIX_AS_ADDR
//        When set, use szPreFix as staring address
//      - OSS_HEXDUMP_RAW_HEX_ONLY
//        When set, only display the raw hex value, the ASCII
//        representation will not be displayed.
// pBytesProcessed [out]
//     number of bytes processed/printed from input
// Return value:
//     The offset of last character written into the output buffer( not
//     including the NULL terminator )

//
// Example:
//   hexdump first 2K of file ossUtil.cpp :
//     char junk[2048] ;
//     char myBuf[4096] = { 0 } ;
//     int bytesRead = 0 ;
//     ossPrimitiveFileOp fop;
//     fop.Open( "/home/sequoiadb/sequoiadb/SequoiaDB/engine/oss/ossUtil.cpp" );
//     fop.Read( sizeof( junk ), junk, &bytesRead ) ;
//     ossHexDumpBuffer( junk, sizeof(junk), myBuf, sizeof(myBuf), NULL,
//                       OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
//     printf("\n%s\n", myBuf ) ;
UINT32 ossHexDumpBuffer
(
   const void *   inPtr,
   UINT32         len,
   CHAR *         szOutBuf,
   UINT32         outBufSz,
   const void *   szPrefix,
   UINT32         flags,
   UINT32     *   pBytesProcessed = NULL
) ;

INT32 ossGetMemoryInfo ( INT32 &loadPercent,
                         INT64 &totalPhys,   INT64 &availPhys,
                         INT64 &totalPF,     INT64 &availPF,
                         INT64 &totalVirtual, INT64 &availVirtual,
                         INT32 &overCommitMode,
                         INT64 &commitLimit,  INT64 &committedAS ) ;

INT32 ossGetMemoryInfo ( INT32 &loadPercent,
                         INT64 &totalPhys,   INT64 &availPhys,
                         INT64 &totalPF,     INT64 &availPF,
                         INT64 &totalVirtual, INT64 &availVirtual ) ;

INT32 ossGetDiskInfo ( const CHAR *pPath, INT64 &totalBytes, INT64 &freeBytes,
                       CHAR* fsName = NULL, INT32 fsNameSize = 0 ) ;

INT32 ossGetFileDesp ( INT64 &usedNum ) ;

INT32 ossGetProcessMemory( OSSPID pid, INT64 &vmRss, INT64 &vmSize ) ;

typedef struct _ossDiskIOStat
{
   UINT64 rdSectors ;
   UINT64 wrSectors ;
   UINT64 rdIos ;
   UINT64 rdMerges ;
   UINT64 wrIos ;
   UINT64 wrMerges ;
   UINT32 rdTicks ;
   UINT32 wrTicks ;
   UINT32 iosPgr ;
   UINT32 totTicks ;
   UINT32 rqTicks ;
} ossDiskIOStat ;

INT32 ossReadlink ( const CHAR *pPath, CHAR *pLinkedPath, INT32 maxLen ) ;

INT32 ossGetDiskIOStat ( const CHAR *pDriverName, ossDiskIOStat &ioStat ) ;

INT32 ossGetCPUInfo ( SINT64 &user, SINT64 &sys,
                      SINT64 &idle, SINT64 &other ) ;

typedef struct _ossProcMemInfo
{
   INT64    vSize;         // used virtual memory size(MB)
   INT64    rss;           // resident size(MB)
   INT64    fault;
}ossProcMemInfo;
INT32 ossGetProcMemInfo( ossProcMemInfo &memInfo,
                        OSSPID pid = ossGetCurrentProcessID() );
#if defined (_LINUX) || defined (_AIX)
class ossProcStatInfo
{
public:
   ossProcStatInfo( OSSPID pid );
   ~ossProcStatInfo(){}

public:
   INT32    _pid;
   char     _comm[ OSS_MAX_PATHSIZE + 1 ];
   char     _state;
   INT32    _ppid;
   INT32    _pgrp;
   INT32    _session;
   INT32    _tty;
   INT32    _tpgid;
   UINT32   _flags;
   UINT32   _minFlt;
   UINT32   _cMinFlt;
   UINT32   _majFlt;
   UINT32   _cMajFlt;
   UINT32   _uTime;
   UINT32   _sTime;
   INT32    _cuTime;
   INT32    _csTime;
   INT32    _priority;
   INT32    _nice;
   INT32    _nlwp;
   UINT32   _alarm;
   UINT32   _startTime;
   UINT32   _vSize;
   INT32    _rss;
   UINT32   _rssRlim;
   UINT32   _startCode;
   UINT32   _endCode;
   UINT32   _startStack;
   UINT32   _kstkEsp;
   UINT32   _kstkEip;
};
#endif   //#if defined (_LINUX)

#define OSS_MAX_IP_NAME 15
#define OSS_MAX_IP_ADDR 15
#define OSS_LOOPBACK_IP "127.0.0.1"
#define OSS_LOCALHOST   "localhost"

typedef struct _ossIP
{
   char  ipName[OSS_MAX_IP_NAME + 1];
   char  ipAddr[OSS_MAX_IP_ADDR + 1];
} ossIP;

class ossIPInfo
{
private:
   INT32   _ipNum;
   ossIP*  _ips;

public:
   ossIPInfo();
   ~ossIPInfo();
   inline INT32 getIPNum() const { return _ipNum; }
   inline ossIP* getIPs() const { return _ips; }

private:
   INT32 _init();
};

#define OSS_LIMIT_VIRTUAL_MEM "virtual memory"
#define OSS_LIMIT_CORE_SZ     "core file size"
#define OSS_LIMIT_DATA_SEG_SZ "data seg size"
#define OSS_LIMIT_FILE_SZ     "file size"
#define OSS_LIMIT_CPU_TIME    "cpu time"
#define OSS_LIMIT_FILE_LOCK   "file locks"
#define OSS_LIMIT_MEM_LOCK    "max locked memory"
#define OSS_LIMIT_MSG_QUEUE   "POSIX message queues"
#define OSS_LIMIT_OPEN_FILE   "open files"
#define OSS_LIMIT_SCHE_PRIO   "scheduling priority"
#define OSS_LIMIT_STACK_SIZE  "stack size"
#define OSS_LIMIT_PROC_NUM    "process num"

class ossProcLimits
{
public:
   ossProcLimits() ;

   std::string str() const ;

   INT32 init() ;

   /// -1 means unlimited
   BOOLEAN getLimit( const CHAR *str,
                     INT64 &soft,
                     INT64 &hard ) const ;

   INT32 setLimit( const CHAR *str, INT64 soft, INT64 hard ) ;

private:
   void _initRLimit( INT32 resource, const CHAR *str ) ;

private:
   struct cmp
   {
      BOOLEAN operator()( const CHAR *l, const CHAR *r ) const
      {
         return ossStrcmp( l, r ) < 0 ;
      }
   } ;
   std::map<const CHAR *, std::pair<INT64, INT64>, cmp > _desc ;
} ;

BOOLEAN ossNetIpIsValid( const CHAR *ip, INT32 len ) ;

INT32& ossGetSignalShieldFlag() ;
INT32& ossGetPendingSignal() ;

/*
   ossSignalShield define
*/
class ossSignalShield
{
   public:
      ossSignalShield() ;
      ~ossSignalShield() ;
      void doNothing() {}
   private:
      void close() ;
} ;

#endif  //OSSUTIL_HPP_

