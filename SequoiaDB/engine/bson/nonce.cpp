
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

#include "lib/nonce.h"
#include <stdlib.h>
#include <time.h>
using namespace std;
namespace Nonce {


    Security::Security() {
        init();
    }

    void Security::init() {
        if( _initialized ) return;
            _initialized = true;

        srand((unsigned int)time(NULL));
    }

    nonce Security::getNonce() {

        nonce n;
      #if defined(_WIN32)
        unsigned int a=0, b=0;
        rand_s(&a);
        rand_s(&b);
        n = (((unsigned long long)a)<<32) | b;
      #else
        n = (((unsigned long long)random())<<32) | random();
      #endif
        return n;
    }
    unsigned getRandomNumber() { return (unsigned) security.getNonce(); }

    bool Security::_initialized;
    Security security;

} // namespace mongo
