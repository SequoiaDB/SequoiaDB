// nonce.h

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

#include <iostream>
#include <fstream>
#include <ios>
namespace Nonce {

    typedef unsigned long long nonce;

    struct Security {
        Security();

        nonce getNonce();
        /** safe during global var initialization */
        nonce getNonceInitSafe() {
            init();
            return getNonce();
        }

    private:
        std::ifstream *_devrandom;
        static bool _initialized;
        void init();

    };

    extern Security security;

} // namespace mongo
