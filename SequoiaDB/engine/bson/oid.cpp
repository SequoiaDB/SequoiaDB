
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
#include <stdio.h>
#if !defined (_WIN32)
#include <sys/types.h>
#include <unistd.h>
#endif
#include "oid.h"
#include "lib/atomic_int.h"
#include "lib/nonce.h"



using namespace Nonce;

namespace bson {

    OID::MachineAndPid OID::ourMachine;

    unsigned OID::ourPid() {
        unsigned pid;
#if defined(_WIN32)
        pid = (unsigned short) GetCurrentProcessId();
#elif defined(__linux__) || defined(__APPLE__) || defined(__sunos__) || defined (_AIX)
        pid = (unsigned short) getpid();
#else
        pid = (unsigned short) Security::getNonce();
#endif
        return pid;
    }

    void OID::foldInPid(OID::MachineAndPid& x) {
        unsigned p = ourPid();
        x._pid ^= (unsigned short) p;
        unsigned short& rest = (unsigned short &) x._machineNumber[1];
        rest ^= p >> 16;
    }

    OID::MachineAndPid OID::genMachineAndPid() {

        {
            Nonce::nonce a = Nonce::security.getNonceInitSafe();
            Nonce::nonce b = Nonce::security.getNonceInitSafe();
            Nonce::nonce c = Nonce::security.getNonceInitSafe();
            assert( !(a==b && b==c) );
        }

        unsigned long long n = Nonce::security.getNonceInitSafe();
        OID::MachineAndPid x = ourMachine = (OID::MachineAndPid&) n;
        foldInPid(x);
        return x;
    }

    OID::MachineAndPid OID::ourMachineAndPid = OID::genMachineAndPid();
/*
    void OID::regenMachineId() {
        ourMachineAndPid = genMachineAndPid();
    }
*/
    inline bool OID::MachineAndPid::operator!=(const OID::MachineAndPid& rhs)
      const {
        return _pid != rhs._pid || _machineNumber != rhs._machineNumber;
    }

    unsigned OID::getMachineId() {
        unsigned char x[4];
        x[0] = ourMachineAndPid._machineNumber[0];
        x[1] = ourMachineAndPid._machineNumber[1];
        x[2] = ourMachineAndPid._machineNumber[2];
        x[3] = 0;
        return (unsigned&) x[0];
    }

/*
    void OID::justForked() {
        MachineAndPid x = ourMachine;
        foldInPid(x);
        ourMachineAndPid = genMachineAndPid();
        assert( x != ourMachineAndPid );
        ourMachineAndPid = x;
    }
*/
    void OID::init() {
        static mongo::AtomicUInt inc = (unsigned) security.getNonce();

        {
            unsigned t = (unsigned) time(0);
            unsigned char *T = (unsigned char *) &t;
#if defined (SDB_BIG_ENDIAN)
            _time[0] = T[0];
            _time[1] = T[1];
            _time[2] = T[2];
            _time[3] = T[3];
#else
            _time[0] = T[3]; // big endian order because we use memcmp() to
            _time[1] = T[2]; // compare OID's
            _time[2] = T[1];
            _time[3] = T[0];
#endif
        }

        _machineAndPid = ourMachineAndPid;

        {
            int new_inc = inc++;
            unsigned char *T = (unsigned char *) &new_inc;
#if defined (SDB_BIG_ENDIAN)
            _inc[0] = T[1];
            _inc[1] = T[2];
            _inc[2] = T[3];
#else
            _inc[0] = T[2];
            _inc[1] = T[1];
            _inc[2] = T[0];
#endif
        }
    }

    void OID::init( string s ) {
        assert( s.size() == 24 );
        const char *p = s.c_str();
        for( int i = 0; i < 12; i++ ) {
            data[i] = fromHex(p);
            p += 2;
        }
    }

    void OID::init(Date_t date, bool max) {
        int time = (int) (date / 1000);
        char* T = (char *) &time;
#if defined (SDB_BIG_ENDIAN)
        data[0] = T[0];
        data[1] = T[1];
        data[2] = T[2];
        data[3] = T[3];
#else
        data[0] = T[3];
        data[1] = T[2];
        data[2] = T[1];
        data[3] = T[0];
#endif

        if (max)
            *(long long*)(data + 4) = 0xFFFFFFFFFFFFFFFFll;
        else
            *(long long*)(data + 4) = 0x0000000000000000ll;
    }

    time_t OID::asTimeT() {
        int time;
        char* T = (char *) &time;
#if defined (SDB_BIG_ENDIAN)
        T[0] = data[0];
        T[1] = data[1];
        T[2] = data[2];
        T[3] = data[3];
#else
        T[0] = data[3];
        T[1] = data[2];
        T[2] = data[1];
        T[3] = data[0];
#endif
        return time;
    }

}
