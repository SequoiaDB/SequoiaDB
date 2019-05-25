
/*    Copyright 2009 10gen Inc.
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
#include "misc.h"
namespace bson {

    /* replsets use RSOpTime.
       M/S uses OpTime.
       But this is useable from both.
       */
    typedef unsigned long long ReplTime;

    /* Operation sequence #.  A combination of current second plus an ordinal
       value.
     */
#pragma pack(4)
    class OpTime {
        unsigned i;
        signed secs;
        static OpTime last;
    public:
        static void setLast(const Date_t &date) {
            last = OpTime(date);
        }
        signed getSecs() const {
            return secs;
        }
        OpTime(Date_t date) {
            reinterpret_cast<long long&>(*this) = date.millis;
        }
        OpTime(ReplTime x) {
            reinterpret_cast<unsigned long long&>(*this) = x;
        }
        OpTime(signed a, unsigned b) {
            secs = a;
            i = b;
        }
        OpTime() {
            secs = 0;
            i = 0;
        }
        static OpTime now() {
            signed t = (signed) time(0);
            if ( t < last.secs ) {
                t = last.secs;
            }
            if ( last.secs == t ) {
                last.i++;
                return last;
            }
            last = OpTime(t, 1);
            return last;
        }

        /* We store OpTime's in the database as BSON Date datatype -- we needed
            some sort of 64 bit "container" for these values.  While these are
            not really "Dates", that seems a better choice for now than say,
            Number, which is floating point.  Note the BinData type is perhaps
            the cleanest choice, lacking a true unsigned64 datatype, but BinData
            has 5 bytes of overhead.
         */
        long long asDate() const {
            long long time = 0 ;
            memcpy( (char *)&time, &i, sizeof( unsigned ) ) ;
            memcpy( (char *)&time + sizeof( unsigned ), &secs,
                    sizeof( signed ) ) ;
            return time ;
        }
        long long asLL() const {
            long long time = 0 ;
            memcpy( (char *)&time, &i, sizeof( unsigned ) ) ;
            memcpy( (char *)&time + sizeof( unsigned ), &secs,
                    sizeof( signed ) ) ;
            return time ;
        }

        bool isNull() const { return secs == 0; }

        string toStringLong() const {
            char buf[64];
            time_t_to_String(secs, buf);
            stringstream ss;
            ss << time_t_to_String_short(secs) << ' ';
            ss << hex << secs << ':' << i;
            return ss.str();
        }

        string toStringPretty() const {
            stringstream ss;
            ss << time_t_to_String_short(secs) << ':' << hex << i;
            return ss.str();
        }

        string toString() const {
            stringstream ss;
            ss << hex << secs << ':' << i;
            return ss.str();
        }

        bool operator==(const OpTime& r) const {
            return i == r.i && secs == r.secs;
        }
        bool operator!=(const OpTime& r) const {
            return !(*this == r);
        }
        bool operator<(const OpTime& r) const {
            int l_secs = ( int ) secs ;
            int r_secs = ( int ) r.secs ;
            if ( l_secs != r_secs )
                return l_secs < r_secs;
            return i < r.i;
        }
        bool operator<=(const OpTime& r) const {
            return *this < r || *this == r;
        }
        bool operator>(const OpTime& r) const {
            return !(*this <= r);
        }
        bool operator>=(const OpTime& r) const {
            return !(*this < r);
        }
    };
#pragma pack()

} // namespace mongo
