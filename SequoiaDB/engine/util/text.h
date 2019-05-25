/******************************************************************************/
/*
 *    Copyright 2010 10gen Inc.
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
#include "pd.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <stdlib.h>
using namespace std;

    class StringSplitter {
    public:
        /** @param big the string to be split
            @param splitter the delimiter
        */
        StringSplitter( const char * big , const char * splitter )
            : _big( big ) , _splitter( splitter ) {
        }

        /** @return true if more to be taken via next() */
        bool more() {
            return _big[0] != 0;
        }

        /** get next split string fragment */
        string next() {
            const char * foo = strstr( _big , _splitter );
            if ( foo ) {
                string s( _big , foo - _big );
                _big = foo + 1;
                while ( *_big && strstr( _big , _splitter ) == _big )
                    _big++;
                return s;
            }

            string s = _big;
            _big += strlen( _big );
            return s;
        }

        void split( vector<string>& l ) {
            while ( more() ) {
                l.push_back( next() );
            }
        }

        vector<string> split() {
            vector<string> l;
            split( l );
            return l;
        }

        static vector<string> split( const string& big , const string& splitter ) {
            StringSplitter ss( big.c_str() , splitter.c_str() );
            return ss.split();
        }

        static string join( vector<string>& l , const string& split ) {
            stringstream ss;
            for ( unsigned i=0; i<l.size(); i++ ) {
                if ( i > 0 )
                    ss << split;
                ss << l[i];
            }
            return ss.str();
        }

    private:
        const char * _big;
        const char * _splitter;
    };

    /* This doesn't defend against ALL bad UTF8, but it will guarantee that the
     * string can be converted to sequence of codepoints. However, it doesn't
     * guarantee that the codepoints are valid.
     */
    bool isValidUTF8(const char *s);
    bool isValidUTF8WSize(const char *s,int size);
    inline bool isValidUTF8(string s) { return isValidUTF8(s.c_str()); }

#if defined(_WIN32)

    string toUtf8String(const wstring& wide);

    wstring toWideString(const char *s);

    /* like toWideString but UNICODE macro sensitive */
# if !defined(_UNICODE)
#error temp error 
    inline string toNativeString(const char *s) { return s; }
# else
    inline wstring toNativeString(const char *s) { return toWideString(s); }
# endif

#endif

    inline BOOLEAN parseLL( const char *n, INT64 *out ) {
	if(!n)
		return 0LL;
#if defined(_LINUX)
        char *endPtr = 0;
        errno = 0;
        *out = strtoll( n, &endPtr, 10 );
	if(*endPtr!=0 || errno!=0)
		return FALSE;
#else
        size_t endLen = 0;
        try {
            *out = stoll( n, &endLen, 10 );
        }
        catch ( ... ) {
            endLen = 0;
        }
	if(endLen == 0 || n[endLen]!=0)
		return FALSE;
#endif // !defined(_WIN32)
        return TRUE;
    }
