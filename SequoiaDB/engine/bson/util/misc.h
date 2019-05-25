/* @file misc.h
*/

/*
 *    Copyright 2009 10gen Inc.
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

#include <ctime>

namespace bson {

    using namespace std;

    inline void time_t_to_String(time_t t, char *buf) {
#if defined(_WIN32)
        ctime_s(buf, 32, &t);
#else
        ctime_r(&t, buf);
#endif
        buf[24] = 0; // don't want the \n
    }

    inline string time_t_to_String(time_t t = time(0) ) {
        char buf[64];
#if defined(_WIN32)
        ctime_s(buf, sizeof(buf), &t);
#else
        ctime_r(&t, buf);
#endif
        buf[24] = 0; // don't want the \n
        return buf;
    }

    inline string time_t_to_String_no_year(time_t t) {
        char buf[64];
#if defined(_WIN32)
        ctime_s(buf, sizeof(buf), &t);
#else
        ctime_r(&t, buf);
#endif
        buf[19] = 0;
        return buf;
    }

    inline string time_t_to_String_short(time_t t) {
        char buf[64];
#if defined(_WIN32)
        ctime_s(buf, sizeof(buf), &t);
#else
        ctime_r(&t, buf);
#endif
        buf[19] = 0;
        if( buf[0] && buf[1] && buf[2] && buf[3] )
            return buf + 4; // skip day of week
        return buf;
    }

    struct Date_t {
        long long millis;
        Date_t(): millis(0) {}
        Date_t(long long m): millis(m) {}
        operator long long&() { return millis; }
        operator const long long&() const { return millis; }
        string toString() const {
            char buf[64];
            time_t_to_String(millis/1000, buf);
            return buf;
        }
    };

    inline int strnlen( const char *s, int n ) {
        for( int i = 0; i < n; ++i )
            if ( !s[ i ] )
                return i;
        return -1;
    }
}
