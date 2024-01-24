// @file time_support.h

/*    Copyright 2010 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <cstdio> // sscanf
#include <ctime>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>

namespace bson {

    inline void time_t_to_Struct(time_t t, struct tm * buf , bool local = false)
    {
#if defined(_WIN32)
        if ( local )
            localtime_s( buf , &t );
        else
            gmtime_s(buf, &t);
#else
        if ( local )
            localtime_r(&t, buf);
        else
            gmtime_r(&t, buf);
#endif
    }

    // uses ISO 8601 dates without trailing Z
    // colonsOk should be false when creating filenames
    inline string terseCurrentTime(bool colonsOk=true) {
        struct tm t;
        time_t_to_Struct( time(0) , &t );

        const char* fmt = (colonsOk ? "%Y-%m-%dT%H:%M:%S"
                                    : "%Y-%m-%dT%H-%M-%S");
        char buf[32];
        assert(strftime(buf, sizeof(buf), fmt, &t) == 19);
        return buf;
    }

    inline boost::gregorian::date currentDate() {
        boost::posix_time::ptime now =
          boost::posix_time::second_clock::local_time();
        return now.date();
    }

    // parses time of day in "hh:mm" format assuming 'hh' is 00-23
    inline bool toPointInTime( const string& str , boost::posix_time::ptime*
      timeOfDay ) {
        int hh = 0;
        int mm = 0;
        if ( 2 != sscanf( str.c_str() , "%d:%d" , &hh , &mm ) ) {
            return false;
        }

        // verify that time is well formed
        if ( ( hh / 24 ) || ( mm / 60 ) ) {
            return false;
        }

        boost::posix_time::ptime res( currentDate() ,
          boost::posix_time::hours( hh ) + boost::posix_time::minutes( mm ) );
        *timeOfDay = res;
        return true;
    }

#define MONGO_asctime _asctime_not_threadsafe_
#define asctime MONGO_asctime
#define MONGO_gmtime _gmtime_not_threadsafe_
#define gmtime MONGO_gmtime
#define MONGO_localtime _localtime_not_threadsafe_
#define localtime MONGO_localtime
#define MONGO_ctime _ctime_is_not_threadsafe_
#define ctime MONGO_ctime

#if defined(_WIN32)
    inline void sleepsecs(int s) {
        Sleep(s*1000);
    }
    inline void sleepmillis(long long s) {
        assert( s <= 0xffffffff );
        Sleep((DWORD) s);
    }
    inline void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += (int)( s / 1000000 );
        xt.nsec += (int)(( s % 1000000 ) * 1000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
#elif defined(__sunos__)
    inline void sleepsecs(int s) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += s;
        boost::thread::sleep(xt);
    }
    inline void sleepmillis(long long s) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += (int)( s / 1000 );
        xt.nsec += (int)(( s % 1000 ) * 1000000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
    inline void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        xt.sec += (int)( s / 1000000 );
        xt.nsec += (int)(( s % 1000000 ) * 1000);
        if ( xt.nsec >= 1000000000 ) {
            xt.nsec -= 1000000000;
            xt.sec++;
        }
        boost::thread::sleep(xt);
    }
#else
    inline void sleepsecs(int s) {
        struct timespec t;
        t.tv_sec = s;
        t.tv_nsec = 0;
        if ( nanosleep( &t , 0 ) ) {
            cout << "nanosleep failed" << endl;
        }
    }
    inline void sleepmicros(long long s) {
        if ( s <= 0 )
            return;
        struct timespec t;
        t.tv_sec = (int)(s / 1000000);
        t.tv_nsec = 1000 * ( s % 1000000 );
        struct timespec out;
        if ( nanosleep( &t , &out ) ) {
            cout << "nanosleep failed" << endl;
        }
    }
    inline void sleepmillis(long long s) {
        sleepmicros( s * 1000 );
    }
#endif

    // // note this wraps
    // inline int tdiff(unsigned told, unsigned tnew) {
    //     return WrappingInt::diff(tnew, told);
    // }

    /** curTimeMillis will overflow - use curTimeMicros64 instead if you care
        about that. */
    inline unsigned curTimeMillis() {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        unsigned t = xt.nsec / 1000000;
        return (xt.sec & 0xfffff) * 1000 + t;
    }

    /** Date_t is milliseconds since epoch */
    inline Date_t jsTime() {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        unsigned long long t = xt.nsec / 1000000;
        return ((unsigned long long) xt.sec * 1000) + t;
    }

    inline unsigned long long curTimeMicros64() {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        unsigned long long t = xt.nsec / 1000;
        return (((unsigned long long) xt.sec) * 1000000) + t;
    }

    // measures up to 1024 seconds.  or, 512 seconds with tdiff that is...
    inline unsigned curTimeMicros() {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC_);
        unsigned t = xt.nsec / 1000;
        unsigned secs = xt.sec % 1024;
        return secs*1000000 + t;
    }

    class Timer {
    public:
        Timer() {
            reset();
        }

        Timer( unsigned long long start ) {
            old = start;
        }

        int seconds() const {
            return (int)(micros() / 1000000);
        }

        int millis() const {
            return (long)(micros() / 1000);
        }

        unsigned long long micros() const {
            unsigned long long n = curTimeMicros64();
            return n - old;
        }

        unsigned long long micros(unsigned long long & n) const {
            // returns cur time in addition to timer result
            n = curTimeMicros64();
            return n - old;
        }

        unsigned long long startTime() {
            return old;
        }

        void reset() {
            old = curTimeMicros64();
        }

    private:
        unsigned long long old;
    };

}  // namespace bson
