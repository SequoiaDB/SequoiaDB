/**
 * @file bsonobjiterator.h
 * @brief CPP BSONObjIterator Declarations
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

/** \namespace bson
    \brief Include files for C++ BSON module
*/
namespace bson {

    /** iterator for a BSONObj

       Note each BSONObj ends with an EOO element: so you will get more() on an
       empty object, although next().eoo() will be true.

       todo: we may want to make a more stl-like iterator interface for this
             with things like begin() and end()
    */
    class BSONObjIterator {
    public:
        /** Create an iterator for a BSON object.
        */
        BSONObjIterator(const BSONObj& jso) {
            int sz = jso.objsize();
            if ( sz == 0 ) {
                _pos = _theend = 0;
                return;
            }
            _pos = jso.objdata() + 4;
            _theend = jso.objdata() + sz - 1;
        }

        BSONObjIterator() {
            _pos    = NULL ;
            _theend = NULL ;
        }

        BSONObjIterator( const char * start , const char * end ) {
            _pos = start + 4;
            _theend = end - 1;
        }

        /** @return true if more elements exist to be enumerated. */
        bool more() { return _pos < _theend; }

        /** @return true if more elements exist to be enumerated INCLUDING the
            EOO element which is always at the end. */
        bool moreWithEOO() { return _pos <= _theend; }

        /** @return the next element in the object. For the final element,
            element.eoo() will be true. */
        BSONElement next( bool checkEnd ) {
            assert( _pos <= _theend );
            BSONElement e( _pos, checkEnd ? (int)(_theend + 1 - _pos) : -1 );
            _pos += e.size( checkEnd ? (int)(_theend + 1 - _pos) : -1 );
            return e;
        }
        BSONElement next() {
            assert( _pos <= _theend );
            BSONElement e(_pos);
            _pos += e.size();
            return e;
        }
        void operator++() { next(); }
        void operator++(int) { next(); }

        BSONElement operator*() {
            assert( _pos <= _theend );
            return BSONElement(_pos);
        }

    private:
        const char* _pos;
        const char* _theend;
    };

#define BSONOBJITERSORTED_DFTFIELDS 100
    class BSONObjIteratorSorted {
    public:
        BSONObjIteratorSorted( const BSONObj& o );

        ~BSONObjIteratorSorted() {
            assert( _fields );
            if ( _fields != &_staticFields[0] )
               delete[] _fields;
            _fields = 0;
        }

        bool more() {
            return _cur < _nfields;
        }

        BSONElement next() {
            assert( _fields );
            if ( _cur < _nfields )
                return BSONElement( _fields[_cur++] );
            return BSONElement();
        }

    private:
        const char ** _fields;
        const char *_staticFields[BSONOBJITERSORTED_DFTFIELDS] ;
        int _nfields;
        int _cur;
    };

    /** Similar to BOOST_FOREACH
     *
     *  because the iterator is defined outside of the for, you must use {}
     *  around the surrounding scope. Don't do this:
     *
     *  if (foo)
     *      BSONForEach(e, obj)
     *          doSomething(e);
     *
     *  but this is OK:
     *
     *  if (foo) {
     *      BSONForEach(e, obj)
     *          doSomething(e);
     *  }
     *
     */

/*#define BSONForEach(e, obj)                                     \
    BSONObjIterator BOOST_PP_CAT(it_,__LINE__)(obj);            \
    for ( BSONElement e;                                        \
            (BOOST_PP_CAT(it_,__LINE__).more() ?                  \
             (e = BOOST_PP_CAT(it_,__LINE__).next(), true) :  \
             false) ; ) */
}
