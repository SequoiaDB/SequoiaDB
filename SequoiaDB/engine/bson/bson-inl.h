/** @file bsoninlines.h
  a goal here is that the most common bson methods can be used inline-only, a
  la boost. thus some things are inline that wouldn't necessarily be otherwise.
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

#include <map>
#include <limits>
#include <time.h>
#include "ossTypes.h"
#include "util/misc.h"
#include "util/hex.h"
#include "base64c.h"

#if defined(_WIN32)
#undef max
#undef min
#endif

static void local_time ( time_t *Time, struct tm *TM )
{
   if ( !Time || !TM )
      return ;
#if defined (__linux__ ) || defined (_AIX)
   localtime_r( Time, TM ) ;
#elif defined (_WIN32)
   localtime_s( TM, Time ) ;
#endif
}

namespace bson {
    inline bool isNaN(double d) {
        return d != d;
    }

    inline bool isInf(double d, int* sign = 0) {
        volatile double tmp = d;

        if ((tmp == d) && ((tmp - d) != 0.0)) {
            if ( sign ) {
                *sign = (d < 0.0 ? -1 : 1);
            }
            return true;
        }
        if ( sign ) {
            *sign = 0;
        }

        return false;
    }

    /* TODO(jbenet) evaluate whether 'int bson::compareElementValues(const
        bson::BSONElement&, const bson::BSONElement&)' does belong here. */

    /* wo = "well ordered" */
    inline int BSONElement::woCompare( const BSONElement &e,
                                bool considerFieldName ) const {
        int lt = (int) canonicalType();
        int rt = (int) e.canonicalType();
        int x = lt - rt;
        if( x != 0 && (!isNumber() || !e.isNumber()) )
            return x;
        if ( considerFieldName ) {
            x = strcmp(fieldName(), e.fieldName());
            if ( x != 0 )
                return x;
        }
        x = compareElementValues(*this, e);
        return x;
    }

    inline BSONObjIterator BSONObj::begin() const {
        return BSONObjIterator(*this);
    }

    inline BSONObj BSONElement::embeddedObjectUserCheck() const {
        if ( isABSONObj() )
            return BSONObj(value());
        stringstream ss;
        ss << "invalid parameter: expected an object (" << fieldName() << ")";
        uasserted( 10065 , ss.str() );
        return BSONObj(); // never reachable
    }

    inline BSONObj BSONElement::embeddedObject() const {
        assert( isABSONObj() );
        return BSONObj(value());
    }

    inline BSONObj BSONElement::codeWScopeObject() const {
        assert( type() == CodeWScope );
        int strSizeWNull = *(int *)( value() + 4 );
        return BSONObj( value() + 4 + 4 + strSizeWNull );
    }

    inline bool BSONObj::equal(const BSONObj &rhs) const {
        BSONObjIterator i(*this);
        BSONObjIterator j(rhs);
        BSONElement l,r;
        do {
            l = i.next();
            r = j.next();
            if ( l.eoo() )
                return r.eoo();
        } while( l == r );
        return false;
    }

    inline NOINLINE_DECL void BSONObj::_assertInvalid() const {
        StringBuilder ss;
        int os = objsize();
        ss << "Invalid BSONObj size: " << os << " (0x"
           << toHex( &os, 4 ) << ')';
        try {
            BSONElement e = firstElement();
            ss << " first element: " << e.toString();
        }
        catch ( ... ) { }
        massert( 10334 , ss.str().c_str() , 0 );
    }

    /* the idea with NOINLINE_DECL here is to keep this from inlining in the
       getOwned() method.  the presumption being that is better.
    */
    inline NOINLINE_DECL BSONObj BSONObj::copy() const {
        Holder *h = (Holder*) malloc(objsize() + sizeof(unsigned));
        h->zero();
        memcpy(h->data, objdata(), objsize());
        return BSONObj(h);
    }

    inline BSONObj BSONObj::getOwned() const {
        if ( isOwned() )
            return *this;
        return copy();
    }

    inline BSONObj BSONElement::wrap() const {
        BSONObjBuilder b(size()+6);
        b.append(*this);
        return b.obj();
    }

    inline BSONObj BSONElement::wrap( const char * newName ) const {
        BSONObjBuilder b(size()+6+(int)strlen(newName));
        b.appendAs(*this,newName);
        return b.obj();
    }

    inline void BSONObj::getFields(unsigned n, const char **fieldNames, BSONElement *fields) const {
        BSONObjIterator i(*this);
        while ( i.more() ) {
            BSONElement e = i.next();
            const char *p = e.fieldName();
            for( unsigned i = 0; i < n; i++ ) {
                if( strcmp(p, fieldNames[i]) == 0 ) {
                    fields[i] = e;
                    break;
                }
            }
        }
    }

    inline BSONElement BSONObj::getField(const StringData& name) const {
        BSONObjIterator i(*this);
        while ( i.more() ) {
            BSONElement e = i.next();
            if ( strcmp(e.fieldName(), name.data()) == 0 )
                return e;
        }
        return BSONElement();
    }

    inline int BSONObj::getIntField(const char *name) const {
        BSONElement e = getField(name);
        return e.isNumber() ? (int) e.number() : std::numeric_limits< int >::min();
    }

    inline bool BSONObj::getBoolField(const char *name) const {
        BSONElement e = getField(name);
        return e.type() == Bool ? e.boolean() : false;
    }

    inline const char * BSONObj::getStringField(const char *name) const {
        BSONElement e = getField(name);
        return e.type() == String ? e.valuestr() : "";
    }

    /* add all the fields from the object specified to this object */
    inline BSONObjBuilder& BSONObjBuilder::appendElements(BSONObj x) {
        BSONObjIterator it(x);
        while ( it.moreWithEOO() ) {
            BSONElement e = it.next();
            if ( e.eoo() ) break;
            append(e);
        }
        return *this;
    }

    inline BSONObjBuilder& BSONObjBuilder::appendElementsWithoutName(BSONObj x) 
    {
        BSONObjIterator it(x);
        while ( it.moreWithEOO() ) {
            BSONElement e = it.next();
            if ( e.eoo() ) break;
            appendAs(e, "");
        }
        return *this;
    }

    /* add all the fields from the object specified to this object if they don't
       exist */
    inline BSONObjBuilder& BSONObjBuilder::appendElementsUnique(BSONObj x) {
        set<string> have;
        {
            BSONObjIterator i = iterator();
            while ( i.more() )
                have.insert( i.next().fieldName() );
        }

        BSONObjIterator it(x);
        while ( it.more() ) {
            BSONElement e = it.next();
            if ( have.count( e.fieldName() ) )
                continue;
            append(e);
        }
        return *this;
    }


    inline bool BSONObj::isValid() {
        int x = objsize();
        return x > 0 && x <= BSONObjMaxInternalSize;
    }

    inline bool BSONObj::getObjectID(BSONElement& e) const {
        BSONElement f = getField("_id");
        if( !f.eoo() ) {
            e = f;
            return true;
        }
        return false;
    }

    inline BSONObjBuilderValueStream::BSONObjBuilderValueStream(
      BSONObjBuilder * builder ) {
        _fieldName = 0;
        _builder = builder;
    }

    template<class T>
    inline BSONObjBuilder& BSONObjBuilderValueStream::operator<<( T value ) {
        _builder->append(_fieldName, value);
        _fieldName = 0;
        return *_builder;
    }

    inline BSONObjBuilder& BSONObjBuilderValueStream::operator<<(
      const BSONElement& e ) {
        _builder->appendAs( e , _fieldName );
        _fieldName = 0;
        return *_builder;
    }

    inline Labeler BSONObjBuilderValueStream::operator<<(
      const Labeler::Label &l ) {
        return Labeler( l, this );
    }

    inline void BSONObjBuilderValueStream::endField(const char *nextFieldName) {
        if ( _fieldName && haveSubobj() ) {
            _builder->append( _fieldName, subobj()->done() );
        }
        _subobj.reset();
        _fieldName = nextFieldName;
    }

    inline BSONObjBuilder *BSONObjBuilderValueStream::subobj() {
        if ( !haveSubobj() )
            _subobj.reset( new BSONObjBuilder() );
        return _subobj.get();
    }

    template<class T> inline
    BSONObjBuilder& Labeler::operator<<( T value ) {
        s_->subobj()->append( l_.l_, value );
        return *s_->_builder;
    }

    inline
    BSONObjBuilder& Labeler::operator<<( const BSONElement& e ) {
        s_->subobj()->appendAs( e, l_.l_ );
        return *s_->_builder;
    }

    void nested2dotted(BSONObjBuilder& b, const BSONObj& obj, const string&
      base="");
    inline BSONObj nested2dotted(const BSONObj& obj) {
        BSONObjBuilder b;
        nested2dotted(b, obj);
        return b.obj();
    }

    void dotted2nested(BSONObjBuilder& b, const BSONObj& obj);
    inline BSONObj dotted2nested(const BSONObj& obj) {
        BSONObjBuilder b;
        dotted2nested(b, obj);
        return b.obj();
    }

    inline BSONObjIterator BSONObjBuilder::iterator() const {
        const char * s = _b.buf() + _offset;
        const char * e = _b.buf() + _b.len();
        return BSONObjIterator( s , e );
    }

    inline bool BSONObjBuilder::hasField( const StringData& name ) const {
        BSONObjIterator i = iterator();
        while ( i.more() )
            if ( strcmp( name.data() , i.next().fieldName() ) == 0 )
                return true;
        return false;
    }

    /* WARNING: nested/dotted conversions are not 100% reversible
     * nested2dotted(dotted2nested({a.b: {c:1}})) -> {a.b.c: 1}
     * also, dotted2nested ignores order
     */

    typedef map<string, BSONElement> BSONMap;
    inline BSONMap bson2map(const BSONObj& obj) {
        BSONMap m;
        BSONObjIterator it(obj);
        while (it.more()) {
            BSONElement e = it.next();
            m[e.fieldName()] = e;
        }
        return m;
    }

    struct BSONElementFieldNameCmp {
        bool operator()( const BSONElement &l, const BSONElement &r ) const {
            return strcmp( l.fieldName() , r.fieldName() ) <= 0;
        }
    };

    typedef set<BSONElement, BSONElementFieldNameCmp> BSONSortedElements;
    inline BSONSortedElements bson2set( const BSONObj& obj ) {
        BSONSortedElements s;
        BSONObjIterator it(obj);
        while ( it.more() )
            s.insert( it.next() );
        return s;
    }

    inline string BSONObj::toString( bool isArray, bool full ) const {
        if ( isEmpty() ) 
        {
           if ( isArray )
           {
              return "[]" ;
           }
           else
           {
              return "{}";
           }
        }
        StringBuilder s;
        toString(s, isArray, full);
        return s.str();
    }
    inline void BSONObj::toString(StringBuilder& s,  bool isArray, bool full )
      const {
        if ( isEmpty() )
        {
            if ( isArray )
            {
               s << "[]";
            }
            else
            {
               s << "{}";
            }
            return;
        }

        s << ( isArray ? "[ " : "{ " );
        BSONObjIterator i(*this);
        bool first = true;
        while ( 1 ) {
            massert( 10327 ,  "Object does not end with EOO", i.moreWithEOO() );
            BSONElement e = i.next( true );
            massert( 10328 ,  "Invalid element size", e.size() > 0 );
            massert( 10329 ,  "Element too large", e.size() < ( 1 << 30 ) );
            int offset = (int) (e.rawdata() - this->objdata());
            massert( 10330 ,  "Element extends past end of object",
                     e.size() + offset <= this->objsize() );
            e.validate();
            bool end = ( e.size() + offset == this->objsize() );
            if ( e.eoo() ) {
                massert( 10331 ,  "EOO Before end of object", end );
                break;
            }
            if ( first )
                first = false;
            else
                s << ", ";
            e.toString(s, !isArray, full );
        }
        s << ( isArray ? " ]" : " }" );
    }

    inline void BSONElement::validate() const {
        const BSONType t = type();

        switch( t ) {
        case DBRef:
        case Code:
        case Symbol:
        case bson::String: {
            unsigned x = (unsigned) valuestrsize();
            bool lenOk = x > 0 && x < (unsigned) BSONObjMaxInternalSize;
            if( lenOk && valuestr()[x-1] == 0 )
                return;
            StringBuilder buf;
            buf <<  "Invalid dbref/code/string/symbol size: " << x;
            if( lenOk )
                buf << " strnlen:" << bson::strnlen( valuestr() , x );
            msgasserted( 10321 , buf.str() );
            break;
        }
        case CodeWScope: {
            int totalSize = *( int * )( value() );
            massert( 10322 ,  "Invalid CodeWScope size", totalSize >= 8 );
            int strSizeWNull = *( int * )( value() + 4 );
            massert( 10323 ,  "Invalid CodeWScope string size",
              totalSize >= strSizeWNull + 4 + 4 );
            massert( 10324 ,  "Invalid CodeWScope string size",
                     strSizeWNull > 0 &&
                     (strSizeWNull - 1) ==
                      bson::strnlen( codeWScopeCode(), strSizeWNull ) );
            massert( 10325 ,  "Invalid CodeWScope size",
              totalSize >= strSizeWNull + 4 + 4 + 4 );
            int objSize = *( int * )( value() + 4 + 4 + strSizeWNull );
            massert( 10326 ,  "Invalid CodeWScope object size",
              totalSize == 4 + 4 + strSizeWNull + objSize );
        }
        case Object:
        default:
            break;
        }
    }

    inline int BSONElement::size( int maxLen ) const {
        if ( totalSize >= 0 )
            return totalSize;

        int remain = maxLen - fieldNameSize() - 1;

        int x = 0;
        switch ( type() ) {
        case EOO:
        case Undefined:
        case jstNULL:
        case MaxKey:
        case MinKey:
            break;
        case bson::Bool:
            x = 1;
            break;
        case NumberInt:
            x = 4;
            break;
        case Timestamp:
        case bson::Date:
        case NumberDouble:
        case NumberLong:
            x = 8;
            break;
        case NumberDecimal:
            x = *reinterpret_cast< const int* >( value() ) ;
            break;
        case jstOID:
            x = 12;
            break;
        case Symbol:
        case Code:
        case bson::String:
            massert( 10313 ,  "Insufficient bytes to calculate element size",
              maxLen == -1 || remain > 3 );
            x = valuestrsize() + 4;
            break;
        case CodeWScope:
            massert( 10314 ,  "Insufficient bytes to calculate element size",
              maxLen == -1 || remain > 3 );
            x = objsize();
            break;

        case DBRef:
            massert( 10315 ,  "Insufficient bytes to calculate element size",
              maxLen == -1 || remain > 3 );
            x = valuestrsize() + 4 + 12;
            break;
        case Object:
        case bson::Array:
            massert( 10316 ,  "Insufficient bytes to calculate element size",
              maxLen == -1 || remain > 3 );
            x = objsize();
            break;
        case BinData:
            massert( 10317 ,  "Insufficient bytes to calculate element size",
              maxLen == -1 || remain > 3 );
            x = valuestrsize() + 4 + 1/*subtype*/;
            break;
        case RegEx: {
            const char *p = value();
            size_t len1 = ( maxLen == -1 ) ? strlen( p ) :
              (size_t)bson::strnlen( p, remain );
            p = p + len1 + 1;
            size_t len2;
            if( maxLen == -1 )
                len2 = strlen( p );
            else {
                size_t x = remain - len1 - 1;
                assert( x <= 0x7fffffff );
                len2 = bson::strnlen( p, (int) x );
            }
            x = (int) (len1 + 1 + len2 + 1);
        }
        break;
        default: {
            StringBuilder ss;
            ss << "BSONElement: bad type " << (int) type();
            string msg = ss.str();
            massert( 13655 , msg.c_str(),false);
        }
        }
        totalSize =  x + fieldNameSize() + 1; // BSONType

        return totalSize;
    }

    inline int BSONElement::size() const {
        if ( totalSize >= 0 )
            return totalSize;

        int x = 0;
        switch ( type() ) {
        case EOO:
        case Undefined:
        case jstNULL:
        case MaxKey:
        case MinKey:
            break;
        case bson::Bool:
            x = 1;
            break;
        case NumberInt:
            x = 4;
            break;
        case Timestamp:
        case bson::Date:
        case NumberDouble:
        case NumberLong:
            x = 8;
            break;
        case NumberDecimal:
            x = *reinterpret_cast< const int* >( value() ) ;
            break;
        case jstOID:
            x = 12;
            break;
        case Symbol:
        case Code:
        case bson::String:
            x = valuestrsize() + 4;
            break;
        case DBRef:
            x = valuestrsize() + 4 + 12;
            break;
        case CodeWScope:
        case Object:
        case bson::Array:
            x = objsize();
            break;
        case BinData:
            x = valuestrsize() + 4 + 1/*subtype*/;
            break;
        case RegEx:
            {
                const char *p = value();
                size_t len1 = strlen(p);
                p = p + len1 + 1;
                size_t len2;
                len2 = strlen( p );
                x = (int) (len1 + 1 + len2 + 1);
            }
            break;
        default:
            {
                StringBuilder ss;
                ss << "BSONElement: bad type " << (int) type();
                string msg = ss.str();
                massert(10320 , msg.c_str(),false);
            }
        }
        totalSize =  x + fieldNameSize() + 1; // BSONType

        return totalSize;
    }

    inline string BSONElement::_numberDecimalStr() const
    {
        int rc = 0 ;
        StringBuilder s;
        bsonDecimal decimal ;
        if ( type() != NumberDecimal )
        {
            return "" ;
        }

        rc = decimal.fromBsonValue( value() ) ;
        if ( 0 != rc )
        {
            return "" ;
        }

        s << decimal.toJsonString() ;

        return s.str() ;
    }

    inline string BSONElement::toString( bool includeFieldName, bool full )
      const {
        StringBuilder s;
        toString(s, includeFieldName, full);
        return s.str();
    }

    inline void escapeString( StringBuilder& s, const char *pStr, int len )
    {
        for ( int i = 0; i < len; ++i )
        {
           switch( *pStr )
           {
           case '\"':
           {
              s << "\\\"" ;
              break ;
           }
           case '\\':
           {
              s << "\\\\" ;
              break ;
           }
           case '\b':
           {
              s << "\\b" ;
              break ;
           }
           case '\f':
           {
              s << "\\f" ;
              break ;
           }
           case '\n':
           {
              s << "\\n" ;
              break ;
           }
           case '\r':
           {
              s << "\\r" ;
              break ;
           }
           case '\t':
           {
              s << "\\t" ;
              break ;
           }
           default:
           {
              s << (*pStr) ;
              break ;
           }
           }
           ++pStr ;
        }
    }

    inline void escapeString( StringBuilder& s, const char *pStr )
    {
        escapeString( s, pStr, strlen( pStr ) ) ;
    }

    inline void BSONElement::toString(StringBuilder& s, bool includeFieldName,
      bool full ) const {
        if ( includeFieldName && type() != EOO )
        {
            s << "\"" ;
            escapeString( s, fieldName() ) ;
            s << "\": " ;
        }
        switch ( type() ) {
        case EOO:
            s << "EOO";
            break;
        case bson::Date:
        {
            long long milli = date() ;
            char buffer[64] ;
            struct tm psr ;
            memset ( buffer, 0, 64 ) ;
            time_t timer = (time_t)( ( (long long)milli ) / 1000 ) ;
            local_time ( &timer, &psr ) ;
            if( psr.tm_year + 1900 >= 0 &&
                psr.tm_year + 1900 <= 9999 )
            {
               sprintf ( buffer,
                         "{\"$date\": \"%04d-%02d-%02d\"}",
                         psr.tm_year + 1900,
                         psr.tm_mon + 1,
                         psr.tm_mday ) ;
               s << buffer ;
            }
            else
            {
               s << "{ \"$date\": "  ;
               sprintf ( buffer, "%lld", (unsigned long long)milli ) ;
               s << buffer << " }" ;
            }
            break ;
        }
        case RegEx: {
            /*
            s << "{ \"$regex\": \"" << regex() << "\", \"$options\": \"" ;
            const char *p = regexFlags () ;
            if ( p ) s << p ;
            s << "\" }" ;
            */
            s << "{ \"$regex\": \"" ;
            escapeString( s, regex() ) ;
            s << "\", \"$options\": \"" ;
            const char *p = regexFlags() ;
            if ( p ) s << p ;
            s << "\" }" ;
        }
        break ;
        case NumberDouble:
        {
            int sign = 0 ;
            double valNum = number() ;
            if( isInf( valNum, &sign ) == false )
            {
               s.appendDoubleNice( valNum );
            }
            else
            {
               if( sign == 1 )
               {
                  s << "Infinity" ;
               }
               else
               {
                  s << "-Infinity" ;
               }
            }
            break;
        }
        case NumberLong:
        {
            long long num = _numberLong();
            if ( !BSONObj::getJSCompatibility() )
            {
                s << num;
            }
            else
            {
                if ( num >= OSS_SINT64_JS_MIN && num <= OSS_SINT64_JS_MAX )
                {
                    s << num;
                }
                else
                {
                    s << "{ \"$numberLong\": \"" << num << "\" }";
                }

            }
            break;
        }
        case NumberDecimal:
            s << _numberDecimalStr();
            break;
        case NumberInt:
            s << _numberInt();
            break;
        case bson::Bool:
            s << ( boolean() ? "true" : "false" );
            break;
        case Object:
            embeddedObject().toString(s, false, full);
            break;
        case bson::Array:
            embeddedObject().toString(s, true, full);
            break;
        case Undefined:
            s << "{\"$undefined\":1}";
            break;
        case jstNULL:
            s << "null";
            break;
        case MaxKey:
            s << "{\"$maxKey\":1}";
            break;
        case MinKey:
            s << "{\"$minKey\":1}";
            break;
        case CodeWScope:
            s << "CodeWScope( "
              << codeWScopeCode() << ", "
              << codeWScopeObject().toString(false, full) << ")";
            break;
        case Code:
            s << "{ \"$code\": \"" ;
            if ( !full &&  valuestrsize() > 80 ) {
                const char *pStr = valuestr() ;
                int len = strlen( pStr ) ;
                len = len > 70 ? 70 : len ;
                escapeString( s, valuestr(), len ) ;
                s << "...";
            }
            else {
                escapeString( s, valuestr() ) ;
            }
            s << "\" }";
            break;
        case Symbol:
        case bson::String:
            s << '"';
            if ( !full &&  valuestrsize() > 160 ) {
                const char *pStr = valuestr() ;
                int len = strlen( pStr ) ;
                len = len > 150 ? 150 : len ;
                escapeString( s, valuestr(), len ) ;
                s << "...";
            }
            else {
               escapeString( s, valuestr() ) ;
               s << '"';
            }
            break;
        case DBRef:
            s << "DBRef('" << valuestr() << "',";
            {
                bson::OID *x = (bson::OID *) (valuestr() + valuestrsize());
                s << *x << ')';
            }
            break;
        case jstOID:
            s << "{ \"$oid\": \"";
            s << __oid() << "\" }";
            break;
        case BinData:
            s << "{ \"$binary\": \"" ;
            if (full) {
                int len;
                int base64_size = 0 ;
                char *pBase64Buf = NULL ;
                const char* data = binDataClean(len);
                base64_size = getEnBase64Size( len ) ;
                if( len > 0 )
                {
                   pBase64Buf = (char *)malloc( base64_size + 1 ) ;
                   if ( pBase64Buf )
                   {
                      memset( pBase64Buf, 0, base64_size + 1 ) ;
                      if ( base64Encode( data, len, pBase64Buf, base64_size ) >= 0 )
                      {
                        s << pBase64Buf << "\", \"$type\": \"" << binDataType() ;
                      }
                      free( pBase64Buf ) ;
                   }
                }
                else if( len == 0 )
                {
                   s << "\", \"$type\": \"" << binDataType() ;
                }
            }
            s << "\" }" ;
            break;
        case Timestamp:
        {
            Date_t date = timestampTime () ;
            unsigned int inc = timestampInc () ;
            char buffer[128] ;
            time_t timer = (time_t)(((long long)date.millis)/1000) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            memset ( buffer, 0, 128 ) ;
            sprintf ( buffer,
                      "{\"$timestamp\": \"%04d-%02d-%02d-%02d.%02d.%02d.%06d\"}",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday,
                      psr.tm_hour,
                      psr.tm_min,
                      psr.tm_sec,
                      inc ) ;
            s << buffer ;
            break ;
        }
        default:
            s << "?type=" << type();
            break;
        }
    }

    /* return has eoo() true if no match
       supports "." notation to reach into embedded objects
    */
    inline BSONElement BSONObj::getFieldDotted(const char *name) const {
        BSONElement e = getField( name );
        if ( e.eoo() ) {
            const char *p = strchr(name, '.');
            if ( p ) {
                string left(name, p-name);
                BSONObj sub = getObjectField(left.c_str());
                return sub.isEmpty() ? BSONElement() : sub.getFieldDotted(p+1);
            }
        }

        return e;
    }

    inline BSONObj BSONObj::getObjectField(const char *name) const {
        BSONElement e = getField(name);
        BSONType t = e.type();
        return t == Object || t == Array ? e.embeddedObject() : BSONObj();
    }

    inline int BSONObj::nFields() const {
        int n = 0;
        BSONObjIterator i(*this);
        while ( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            n++;
        }
        return n;
    }

    inline BSONObj::BSONObj() {
        /* little endian ordering here, but perhaps that is ok regardless as
           BSON is spec'd to be little endian external to the system. (i.e. the
           rest of the implementation of bson, not this part, fails to support
           big endian)
        */
        static int data[] = { /*size*/5, /*eoo*/0 } ;
        _objdata = (const char*)&data[0] ;
    }

    inline BSONObj BSONElement::Obj() const { return embeddedObjectUserCheck(); }

    inline BSONElement BSONElement::operator[] (const string& field) const {
        BSONObj o = Obj();
        return o[field];
    }

    inline void BSONObj::elems(vector<BSONElement> &v) const {
        BSONObjIterator i(*this);
        while( i.more() )
            v.push_back(i.next());
    }

    inline void BSONObj::elems(list<BSONElement> &v) const {
        BSONObjIterator i(*this);
        while( i.more() )
            v.push_back(i.next());
    }

    template <class T>
    void BSONObj::Vals(vector<T>& v) const {
        BSONObjIterator i(*this);
        while( i.more() ) {
            T t;
            i.next().Val(t);
            v.push_back(t);
        }
    }
    template <class T>
    void BSONObj::Vals(list<T>& v) const {
        BSONObjIterator i(*this);
        while( i.more() ) {
            T t;
            i.next().Val(t);
            v.push_back(t);
        }
    }

    template <class T>
    void BSONObj::vals(vector<T>& v) const {
        BSONObjIterator i(*this);
        while( i.more() ) {
            try {
                T t;
                i.next().Val(t);
                v.push_back(t);
            }
            catch(...) { }
        }
    }
    template <class T>
    void BSONObj::vals(list<T>& v) const {
        BSONObjIterator i(*this);
        while( i.more() ) {
            try {
                T t;
                i.next().Val(t);
                v.push_back(t);
            }
            catch(...) { }
        }
    }

    inline ostream& operator<<( ostream &s, const BSONObj &o ) {
        return s << o.toString();
    }

    inline ostream& operator<<( ostream &s, const BSONElement &e ) {
        return s << e.toString();
    }

    inline StringBuilder& operator<<( StringBuilder &s, const BSONObj &o ) {
        o.toString( s );
        return s;
    }
    inline StringBuilder& operator<<( StringBuilder &s, const BSONElement &e ) {
        e.toString( s );
        return s;
    }


    inline void BSONElement::Val(BSONObj& v) const { v = Obj(); }

    template<typename T>
    inline BSONFieldValue<BSONObj> BSONField<T>::query( const char * q ,
      const T& t ) const {
        BSONObjBuilder b;
        b.append( q , t );
        return BSONFieldValue<BSONObj>( _name , b.obj() );
    }
}
