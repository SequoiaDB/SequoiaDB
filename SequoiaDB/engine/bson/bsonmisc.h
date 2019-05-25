/**
 * @file bsonmisc.h
 * @brief BSON macro
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
#include "bsonnoncopyable.h"
/** \namespace bson
    \brief Include files for C++ BSON module
*/
namespace bson {

    int getGtLtOp(const BSONElement& e);
  /** \class BSONElementCmpWithoutField
       \brief 
   */
    struct BSONElementCmpWithoutField {
        bool operator()( const BSONElement &l, const BSONElement &r ) const {
            return l.woCompare( r, false ) < 0;
        }
    };

    class BSONObjCmp {
    public:
        BSONObjCmp( const BSONObj &order = BSONObj() ) : _order( order ) {}
        bool operator()( const BSONObj &l, const BSONObj &r ) const {
            return l.woCompare( r, _order ) < 0;
        }
        BSONObj order() const { return _order; }
    private:
        BSONObj _order;
    };

    typedef set<BSONObj,BSONObjCmp> BSONObjSet;

    enum FieldCompareResult {
        LEFT_SUBFIELD = -2,/**<  */
        LEFT_BEFORE = -1,/**< */
        SAME = 0,/**<  */
        RIGHT_BEFORE = 1 ,/**<  */
        RIGHT_SUBFIELD = 2/**<  */
    };

    FieldCompareResult
      compareDottedFieldNames( const char* l , const char* r );

    /** Use BSON macro to build a BSONObj from a stream

        e.g.,
           BSON( "name" << "joe" << "age" << 33 )

        with auto-generated object id:
           BSON( GENOID << "name" << "joe" << "age" << 33 )

        The labels GT, GTE, LT, LTE, NE can be helpful for stream-oriented
        construction of a BSONObj, particularly when assembling a Query.  For
        example,
          BSON( "a" << GT << 23.4 << NE << 30 << "b" << 2 )
        produces the object
          { a: { \$gt: 23.4, \$ne: 30 }, b: 2 }.
    */
#define BSON(x) (( bson::BSONObjBuilder(64) << x ).obj())

    /** Use BSON_ARRAY macro like BSON macro, but without keys

        BSONArray arr = BSON_ARRAY( "hello" << 1 << BSON( "foo" <<
          BSON_ARRAY( "bar" << "baz" << "qux" ) ) );

     */
#define BSON_ARRAY(x) (( bson::BSONArrayBuilder() << x ).arr())

    /** Utility class to auto assign object IDs.
       Example:
         cout << BSON( GENOID << "z" << 3 ); // { _id : ..., z : 3 }
    */
    extern struct GENOIDLabeler { } GENOID;

    /* Utility class to add a Date element with the current time
       Example:
         cout << BSON( "created" << DATENOW );
    */
    extern struct DateNowLabeler { } DATENOW;

    /* Utility class to add the minKey (minus infinity) to a given attribute
       Example:
         cout << BSON( "a" << MINKEY ); // { "a" : { "$minKey" : 1 } }
    */
    extern struct MinKeyLabeler { } MINKEY;
    extern struct MaxKeyLabeler { } MAXKEY;

    class Labeler {
    public:
        struct Label {
            Label( const char *l ) : l_( l ) {}
            const char *l_;
        };
        Labeler( const Label &l, BSONObjBuilderValueStream *s )
          : l_( l ), s_( s ) {}

        template<class T>
        BSONObjBuilder& operator<<( T value );

        /* the value of the element e is appended i.e. for
             "age" << GT << someElement
           one gets
             { age : { $gt : someElement's value } }
        */
        BSONObjBuilder& operator<<( const BSONElement& e );
    private:
        const Label &l_;
        BSONObjBuilderValueStream *s_;
    };

    extern Labeler::Label GT;
    extern Labeler::Label GTE;
    extern Labeler::Label LT;
    extern Labeler::Label LTE;
    extern Labeler::Label NE;
    extern Labeler::Label SIZE;


    inline BSONObj OR(const BSONObj& a, const BSONObj& b);
    inline BSONObj OR(const BSONObj& a, const BSONObj& b, const BSONObj& c);
    inline BSONObj OR(const BSONObj& a, const BSONObj& b, const BSONObj& c,
      const BSONObj& d);
    inline BSONObj OR(const BSONObj& a, const BSONObj& b, const BSONObj& c,
      const BSONObj& d, const BSONObj& e);
    inline BSONObj OR(const BSONObj& a, const BSONObj& b, const BSONObj& c,
      const BSONObj& d, const BSONObj& e, const BSONObj& f);

    class BSONObjBuilderValueStream : public bsonnoncopyable {
    public:
        friend class Labeler;
        BSONObjBuilderValueStream( BSONObjBuilder * builder );

        BSONObjBuilder& operator<<( const BSONElement& e );

        template<class T>
        BSONObjBuilder& operator<<( T value );


        BSONObjBuilder& operator<<(MinKeyLabeler& id);
        BSONObjBuilder& operator<<(MaxKeyLabeler& id);

        Labeler operator<<( const Labeler::Label &l );

        void endField( const char *nextFieldName = 0 );
        bool subobjStarted() const { return _fieldName != 0; }

    private:
        const char * _fieldName;
        BSONObjBuilder * _builder;

        bool haveSubobj() const { return _subobj.get() != 0; }
        BSONObjBuilder *subobj();
        auto_ptr< BSONObjBuilder > _subobj;
    };

    /**
       used in conjuction with BSONObjBuilder, allows for proper buffer size to
       prevent crazy memory usage
     */
    class BSONSizeTracker {
    public:
        BSONSizeTracker() {
            _pos = 0;
            for ( int i=0; i<SIZE; i++ )
                _sizes[i] = 512; // this is the default, so just be consistent
        }

        ~BSONSizeTracker() {
        }

        void got( int size ) {
            _sizes[_pos++] = size;
            if ( _pos >= SIZE )
                _pos = 0;
        }

        /**
         * right now choosing largest size
         */
        int getSize() const {
            int x = 16; // sane min
            for ( int i=0; i<SIZE; i++ ) {
                if ( _sizes[i] > x )
                    x = _sizes[i];
            }
            return x;
        }

    private:
        enum { SIZE = 10 };
        int _pos;
        int _sizes[SIZE];
    };

}
