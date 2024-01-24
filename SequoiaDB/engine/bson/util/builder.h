/* builder.h */

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

#include <cfloat>
#include <string>
#include <string.h>
#include <stdio.h>
#include "../inline_decls.h"
#include "../stringdata.h"
#include "../bsonassert.h"

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
   #include "utilMemListPool.hpp"
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

#define BSON_INFO_STR        "#*BSON$@"

namespace bson {
    /* Accessing unaligned doubles on ARM generates an alignment trap and aborts
 * with SIGBUS on Linux.
       Wrapping the double in a packed struct forces gcc to generate code that
works with unaligned values too.
       The generated code for other architectures (which already allow unaligned
accesses) is the same as if
       there was a direct pointer access.
    */
    struct PackedDouble {
        double d;
    } ;

    /* Note the limit here is rather arbitrary and is simply a standard.
       generally the code works with any object that fits in ram.

       Also note that the server has some basic checks to enforce this limit but
       those checks are not exhaustive for example need to check for size too
       big after
         update $push (append) operation
         various db.eval() type operations
    */
    const int BSONObjMaxUserSize = 16 * 1024 * 1024;

    /*
       Sometimeswe we need objects slightly larger - an object in the replication local.oplog
       is slightly larger than a user object for example.
    */
    const int BSONObjMaxInternalSize = BSONObjMaxUserSize + ( 16 * 1024 );

    const int BufferMaxSize = 64 * 1024 * 1024;

    class StringBuilder;

    void msgasserted(int msgid, const char *msg, bool out_of_mem);

   class HeapAllocator {
   public:
       HeapAllocator() { _pInfo = NULL ; }

       void* Malloc(size_t sz) { return malloc(sz); }
       void* Realloc(void *p, size_t sz) { return realloc(p, sz); }
       void Free(void *p) { free(p); }

       void setInfo( const char *pInfo ) { _pInfo = pInfo ; }

    private:
       const char *_pInfo ;
   } ;

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
   class TrivialAllocator {
   public:
       TrivialAllocator() { _pInfo = NULL ; }

       void* Malloc(size_t sz) { return engine::utilThreadAlloc(sz,__FILE__,__LINE__,NULL,_pInfo) ; }
       void* Realloc(void *p, size_t sz) { return engine::utilThreadRealloc(p,sz,__FILE__,__LINE__,NULL,_pInfo) ; }
       void Free(void *p) { SDB_THREAD_FREE( p ) ; }

       void setInfo( const char *pInfo ) { _pInfo = pInfo ; }
       private:
          const char *_pInfo ;
   } ;
#else
   typedef HeapAllocator TrivialAllocator ;
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

    class StackAllocator {
    public:
        enum { SZ = 512 };

        StackAllocator() {}

        void* Malloc(size_t sz) {
            if( sz <= SZ ) return buf;
            return al.Malloc(sz);
        }
        void* Realloc(void *p, size_t sz) {
            if( !p || p == buf ) {
                if( sz <= SZ ) return buf;
                void *d = al.Malloc(sz);
                if ( d && p )
                   memcpy(d, p, SZ);
                return d;
            }
            return al.Realloc(p, sz);
        }
        void Free(void *p) {
            if( p != buf )
                al.Free(p);
        }

        void setInfo( const char *pInfo ) { al.setInfo( pInfo ) ; }
    private:
        char buf[SZ];
        TrivialAllocator al ;
    };

    template< class Allocator >
    class _BufBuilder {
    public:
        typedef Allocator   myAllocator ;
    private:
        // non-copyable, non-assignable
        _BufBuilder( const _BufBuilder& );
        _BufBuilder& operator=( const _BufBuilder& );
        myAllocator al;
    public:
        _BufBuilder( int initsize = 512,
                     int maxBuffSize = BufferMaxSize,
                     const char *pInfo = NULL )
        : _initsize(initsize), _maxBuffSize(maxBuffSize), size(0) {
            if ( _initsize <= 0 ) {
               _initsize = 256 ;
            }
            if ( _maxBuffSize < _initsize ) {
               _maxBuffSize = BufferMaxSize ;
            }
            data = 0;
            reservedBytes = 0;
            _isdelay = false;
            l = 0;
            al.setInfo( pInfo ) ;
        }
        ~_BufBuilder() { kill(); }

        void kill() {
            if ( data ) {
                al.Free(data);
                data = 0;
                size = 0;
            }
            reset() ;
        }

        void reset() {
            l = 0;
            reservedBytes = 0;
            _isdelay = false;
        }
        void reset( int maxSize ) {
            l = 0;
            reservedBytes = 0;
            _isdelay = false;
            if ( maxSize && size > maxSize ) {
                al.Free(data);
                data = (char*)al.Malloc(maxSize);
                size = maxSize;
            }
        }

        /** leave room for some stuff later
            @return point to region that was skipped.  pointer may change later
            (on realloc), so for immediate use only
        */
        char* skip(int n) { return grow(n); }

        void  skipDeplay( int n ) {
           if ( !_isdelay && l + n + reservedBytes > size ) {
               _isdelay = true ;
           }
           l = l + n ;
        }

        char* ensureBuf() { return grow( 0 ) ; }

        /* note this may be deallocated (realloced) if you keep writing. */
        char* buf() {
           if ( _isdelay ) {
               return ensureBuf() ;
           }
           return data;
        }

        const char* buf() const {
            if ( _isdelay ) {
                msgasserted(13551, "BufBuilder should call ensureBuff() "
                                   "before buf()");
            }
            return data;
        }

        /* assume ownership of the buffer - you must then free() it */
        void decouple() {
            data = 0 ;
            size = 0 ;
            reset() ;
        }

        void appendUChar(unsigned char j) {
            *((unsigned char*)grow(sizeof(unsigned char))) = j;
        }
        void appendChar(char j) {
            *((char*)grow(sizeof(char))) = j;
        }
        void appendNum(char j) {
            *((char*)grow(sizeof(char))) = j;
        }
        void appendNum(short j) {
            *((short*)grow(sizeof(short))) = j;
        }
        void appendNum(int j) {
            *((int*)grow(sizeof(int))) = j;
        }
        void appendNum(unsigned j) {
            *((unsigned*)grow(sizeof(unsigned))) = j;
        }
        void appendNum(bool j) {
            *((bool*)grow(sizeof(bool))) = j;
        }
        void appendNum(double j) {
            *((double*)grow(sizeof(double))) = j;
        }
        void appendNum(long long j) {
            *((long long*)grow(sizeof(long long))) = j;
        }
        void appendNum(unsigned long long j) {
            *((unsigned long long*)grow(sizeof(unsigned long long))) = j;
        }

        void appendBuf(const void *src, size_t len) {
            memcpy(grow((int) len), src, len);
        }

        template<class T>
        void appendStruct(const T& s) {
            appendBuf(&s, sizeof(T));
        }

        void appendStr(const StringData &str , bool includeEndingNull = true ) {
            const int len = str.size() + ( includeEndingNull ? 1 : 0 );
            memcpy(grow(len), str.data(), len);
        }

        /** @return length of current string */
        int len() const { return l; }
        void setlen( int newLen ) {
           l = newLen;
           if ( l + reservedBytes > size ) {
               _isdelay = true ;
           } else {
               _isdelay = false ;
           }
        }
        int  getReserveBytes() const { return reservedBytes ; }
        void setReserveBytes( int bytes ) {
           reservedBytes = bytes ;
           if ( l + reservedBytes > size ) {
               _isdelay = true ;
           }
        }
        /** @return size of the buffer */
        int getSize() const { return size; }

        /* returns the pre-grow write position */
        inline char* grow(int by) {
            uassert( 13656, "grow len should not be negative", by >= 0 ) ;
            int oldlen = l;
            int newLen = l + by;
            int minSize = newLen + reservedBytes;
            if ( minSize > size ) {
                grow_reallocate(minSize);
            }
            l = newLen;
            _isdelay = false;
            return data + oldlen;
        }

        /**
         * Reserve room for some number of bytes to be claimed at a later time.
         */
        void reserveBytes(int bytes, bool delay=false) {
            int minSize = l + reservedBytes + bytes;
            if (minSize > size) {
                if ( delay ) {
                    _isdelay = true ;
                } else {
                    grow_reallocate(minSize);
                    _isdelay = false ;
                }
            }

            // This must happen *after* any attempt to grow.
            reservedBytes += bytes;
        }

        /**
         * Claim an earlier reservation of some number of bytes. These bytes
         * must already have been reserved. Appends of up to this many bytes
         * immediately following a claim are guaranteed to succeed without a
         * need to reallocate.
         */
        void claimReservedBytes(int bytes) {
            if ( _isdelay ) {
               ensureBuf() ;
            }
            assert(reservedBytes >= bytes);
            reservedBytes -= bytes;
        }
    private:
        /* "slow" portion of 'grow()'  */
        void NOINLINE_DECL grow_reallocate(int minSize) {
            int a = size * 2;
            if ( a == 0 ) {
                a = minSize <= _initsize ? _initsize : minSize ;
            } else if ( minSize > a ) {
                a = minSize + 16 * 1024;
            }
            if ( a > _maxBuffSize ) {
                if ( minSize > _maxBuffSize ) {
                    char errMsg[ 50 + 1 ] = "" ;
#if defined (_WIN32) || defined (_WIN64)
                     _snprintf( errMsg, 50, "BufBuilder grow() > %d",
                                _maxBuffSize ) ;
                     errMsg[50] = '\0' ;
#else
                     snprintf( errMsg, 50, "BufBuilder grow() > %d",
                               _maxBuffSize ) ;
#endif
                    msgasserted(13548, errMsg) ;
                } else {
                    a = _maxBuffSize ;
                }
            }
            char * newData = (char *)0 ;
            if ( !data )
               newData = (char *) al.Malloc( a ) ;
            else
               newData = (char *) al.Realloc(data, a);
            if ( !newData )
               msgasserted(13550, "BufBuilder grow() out-of-memory");
            data = newData;
            size = a;
        }

        char *data;
        int l;
        int _initsize ;
        int _maxBuffSize ;
        bool _isdelay ;
        int size;
        // eagerly grow_reallocate to keep this many bytes of spare room.
        int reservedBytes;

        friend class StringBuilder;
    };

    typedef _BufBuilder<TrivialAllocator> BufBuilder;

    /** The StackBufBuilder builds smaller datasets on the stack instead of using malloc.
          this can be significantly faster for small bufs.  However, you can not decouple() the
          buffer with StackBufBuilder.
        While designed to be a variable on the stack, if you were to dynamically allocate one,
          nothing bad would happen.  In fact in some circumstances this might make sense, say,
          embedded in some other object.
    */
    class StackBufBuilder : public _BufBuilder<StackAllocator> {
    public:
        StackBufBuilder() : _BufBuilder<StackAllocator>(StackAllocator::SZ) { }
        void decouple(); // not allowed. not implemented.
    };

#if defined(_WIN32)
#pragma warning( push )
// warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
#pragma warning( disable : 4996 )
#endif

#if defined(_WIN32) && _MSC_VER < 1900
#pragma push_macro("snprintf")
#define snprintf _snprintf
#endif

    /** stringstream deals with locale so this is a lot faster than std::stringstream for UTF8 */
    class StringBuilder {
    public:
       static const size_t SDB_DBL_SIZE = 3 + DBL_MANT_DIG - DBL_MIN_EXP + 1;
       static const size_t SDB_S32_SIZE = 12;
       static const size_t SDB_U32_SIZE = 11;
       static const size_t SDB_S64_SIZE = 23;
       static const size_t SDB_U64_SIZE = 22;
       static const size_t SDB_S16_SIZE = 7;
       static const size_t SDB_PTR_SIZE = 19;

        StringBuilder( int initsize=256 )
            : _buf( initsize ) {
        }

        StringBuilder& operator<<( double x ) {
            return SBNUM( x , SDB_DBL_SIZE, "%g" );
        }
        StringBuilder& operator<<( int x ) {
            return SBNUM( x , SDB_S32_SIZE, "%d" );
        }
        StringBuilder& operator<<( unsigned x ) {
            return SBNUM( x , SDB_U32_SIZE, "%u" );
        }
        StringBuilder& operator<<( long x ) {
            return SBNUM( x , SDB_S64_SIZE, "%ld" );
        }
        StringBuilder& operator<<( unsigned long x ) {
            return SBNUM( x , SDB_U64_SIZE, "%lu" );
        }
        StringBuilder& operator<<( long long x ) {
            return SBNUM( x , SDB_S64_SIZE, "%lld" );
        }
        StringBuilder& operator<<( unsigned long long x ) {
            return SBNUM( x , SDB_U64_SIZE, "%llu" );
        }
        StringBuilder& operator<<( short x ) {
            return SBNUM( x , SDB_S16_SIZE, "%hd" );
        }
        StringBuilder& operator<<( char c ) {
            _buf.grow( 1 )[0] = c;
            return *this;
        }

        void appendDoubleNice( double x ) {
            int prev = _buf.l;
            const int maxSize = 32 ;
            char * start = _buf.grow( maxSize );
            int z = snprintf( start, maxSize, "%.16g" , x );
            assert( z >= 0 );
            _buf.l = prev + z;
            if( strchr(start, '.') == 0 && strchr(start, 'E') == 0 &&
                strchr(start, 'N') == 0 && strchr(start, 'e') == 0 &&
                strchr(start, 'n') == 0 ) {
                write( ".0" , 2 );
            }
        }

        void write( const char* buf, int len)
          { memcpy( _buf.grow( len ) , buf , len ); }

        void append( const StringData& str )
          { memcpy( _buf.grow( str.size() ) , str.data() , str.size() ); }

        StringBuilder& operator<<( const StringData& str ) {
            append( str );
            return *this;
        }

        void reset( int maxSize = 0 ) { _buf.reset( maxSize ); }

        std::string str() const { return std::string(_buf.data, _buf.l); }

        int len() const { return _buf.l; }

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
        ossPoolString poolStr() const { return ossPoolString(_buf.data, _buf.l); }
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

    private:
        BufBuilder _buf;

        // non-copyable, non-assignable
        StringBuilder( const StringBuilder& );
        StringBuilder& operator=( const StringBuilder& );

        template <typename T>
        StringBuilder& SBNUM(T val,int maxSize,const char *macro)  {
            int prev = _buf.l;
            int z = snprintf( _buf.grow(maxSize) , maxSize, macro , (val) );
            assert( z >= 0 );
            _buf.l = prev + z;
            return *this;
        }
    };

#if defined(_WIN32)
#pragma warning( pop )
#endif

#if defined(_WIN32) && _MSC_VER < 1900
#undef snprintf
#pragma pop_macro("snprintf")
#endif

} // namespace bson
