/** @file jsobj.cpp - BSON implementation
    http://www.mongodb.org/display/DOCS/BSON
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
#if defined (SDB_ENGINE) || defined (SDB_CLIENT)
#include "core.hpp"
#endif
#include <stdlib.h>
#include "oid.h"
#include "bsonobj.h"
#include "bsonobjbuilder.h"
#include "bsonobjiterator.h"
#include "bson-inl.h"
#include "lib/atomic_int.h"
#include "lib/base64.h"
#include "lib/md5.hpp"
#include <limits>

//#include "util/json.h"
#include "bson.hpp"
#include "util/optime.h"
//#include <boost/static_assert.hpp>
//#include <boost/lexical_cast.hpp>

#include "ordering.h"
#include "util/embedded_builder.h"


// make sure our assumptions are valid
/*BOOST_STATIC_ASSERT( sizeof(short) == 2 );
BOOST_STATIC_ASSERT( sizeof(int) == 4 );
BOOST_STATIC_ASSERT( sizeof(long long) == 8 );
BOOST_STATIC_ASSERT( sizeof(double) == 8 );
BOOST_STATIC_ASSERT( sizeof(bson::Date_t) == 8 );
BOOST_STATIC_ASSERT( sizeof(bson::OID) == 12 );*/

//TODO(jbenet) fix these.
#define out() std::cout
#ifndef log
#define log(...) std::cerr
#endif

namespace bson {

    BSONElement nullElement;

    GENOIDLabeler GENOID;

    DateNowLabeler DATENOW;

    MinKeyLabeler MINKEY;
    MaxKeyLabeler MAXKEY;

    inline bool isNumber( char c ) {
        return c >= '0' && c <= '9';
    }

    inline unsigned stringToNum(const char *str) {
        unsigned x = 0;
        const char *p = str;
        while( 1 ) {
            if( !isNumber(*p) ) {
                if( *p == 0 && p != str )
                    break;
                throw 0;
            }
            x = x * 10 + *p++ - '0';
        }
        return x;
    }

    // for convenience, '{' is greater than anything and stops number parsing
    inline int lexNumCmp( const char *s1, const char *s2, bool pointend )
    {
        bool p1, p2, n1, n2 ;
        const char *e1 ;
        const char *e2 ;
        int len1, len2, result ;

        //cout << "START : " << s1 << "\t" << s2 << endl;
        while( ( *s1 && ( !pointend || '.' != *s1 ) ) &&
               ( *s2 && ( !pointend || '.' != *s2 ) ) )
        {
            p1 = ( *s1 == (char)255 );
            p2 = ( *s2 == (char)255 );
            //cout << "\t\t " << p1 << "\t" << p2 << endl;
            if ( p1 && !p2 )
                return 1 ;
            if ( p2 && !p1 )
                return -1 ;

            n1 = isNumber( *s1 );
            n2 = isNumber( *s2 );

            if ( n1 && n2 )
            {
               int zerolen1 = 0 ;
               int zerolen2 = 0 ;
                // get rid of leading 0s
                while ( *s1 == '0' )
                {
                    s1++ ;
                    ++zerolen1 ;
                }
                while ( *s2 == '0' )
                {
                    s2++ ;
                    ++zerolen2 ;
                }

                e1 = s1 ;
                e2 = s2 ;

                // find length
                // if end of string, will break immediately ('\0')
                while ( isNumber (*e1) ) e1++;
                while ( isNumber (*e2) ) e2++;

                len1 = (int)(e1-s1);
                len2 = (int)(e2-s2);

                // if one is longer than the other, return
                if ( len1 > len2 ) {
                    return 1;
                }
                else if ( len2 > len1 ) {
                    return -1;
                }
                // if the lengths are equal, just strcmp
                else if ( (result = strncmp(s1, s2, len1)) != 0 ) {
                    return result;
                }
                // compare the zero len
                else if ( zerolen1 != zerolen2 ) {
                    return zerolen1 < zerolen2 ? 1 : -1 ;
                }

                // otherwise, the numbers are equal
                s1 = e1;
                s2 = e2;
                continue;
            }

            if ( n1 )
                return 1;

            if ( n2 )
                return -1;

            if ( *s1 > *s2 )
                return 1;

            if ( *s2 > *s1 )
                return -1;

            s1++; s2++;
        }

        if ( *s1 && ( !pointend || '.' != *s1 ) )
            return 1;
        if ( *s2 && ( !pointend || '.' != *s2 ) )
            return -1;
        return 0;
    }

    string escape( string s , bool escape_slash=false) {
        StringBuilder ret;
        for ( string::iterator i = s.begin(); i != s.end(); ++i ) {
            switch ( *i ) {
            case '"':
                ret << "\\\"";
                break;
            case '\\':
                ret << "\\\\";
                break;
            case '/':
                ret << (escape_slash ? "\\/" : "/");
                break;
            case '\b':
                ret << "\\b";
                break;
            case '\f':
                ret << "\\f";
                break;
            case '\n':
                ret << "\\n";
                break;
            case '\r':
                ret << "\\r";
                break;
            case '\t':
                ret << "\\t";
                break;
            default:
                if ( *i >= 0 && *i <= 0x1f ) {
                    //TODO: these should be utf16 code-units not bytes
                    char c = *i;
                    ret << "\\u00" << toHexLower(&c, 1);
                }
                else {
                    ret << *i;
                }
            }
        }
        return ret.str();
    }

    string BSONElement::jsonString( JsonStringFormat format, bool
      includeFieldNames, int pretty ) const {
        BSONType t = type();
        if ( t == Undefined )
            return "";

        stringstream s;
        if ( includeFieldNames )
            s << '"' << escape( fieldName() ) << "\" : ";
        switch ( type() ) {
        case bson::String:
        case Symbol:
            s << '"' << escape( string(valuestr(), valuestrsize()-1) ) << '"';
            break;
        case NumberLong:
            s << _numberLong();
            break;
        case NumberDecimal:
            s << _numberDecimalStr();
            break ;
        case NumberInt:
        case NumberDouble:
            if ( number() >= -numeric_limits< double >::max() &&
                    number() <= numeric_limits< double >::max() ) {
                s.precision( (streamsize)16 );
                s << number();
            }
            else {
                StringBuilder ss;
                ss << "Number " << number() << " cannot be represented in JSON";
                string message = ss.str();
                massert( 10311 ,  message.c_str(), false );
            }
            break;
        case bson::Bool:
            s << ( boolean() ? "true" : "false" );
            break;
        case jstNULL:
            s << "null";
            break;
        case Object:
            s << embeddedObject().jsonString( format, pretty );
            break;
        case bson::Array: {
            if ( embeddedObject().isEmpty() ) {
                s << "[]";
                break;
            }
            s << "[ ";
            BSONObjIterator i( embeddedObject() );
            BSONElement e = i.next();
            if ( !e.eoo() )
                while ( 1 ) {
                    if( pretty ) {
                        s << '\n';
                        for( int x = 0; x < pretty; x++ )
                            s << "  ";
                    }
                    s << e.jsonString( format, false, pretty?pretty+1:0 );
                    e = i.next();
                    if ( e.eoo() )
                        break;
                    s << ", ";
                }
            s << " ]";
            break;
        }
        case DBRef: {
            bson::OID *x = (bson::OID *) (valuestr() + valuestrsize());
            if ( format == TenGen )
                s << "Dbref( ";
            else
                s << "{ \"$ref\" : ";
            s << '"' << valuestr() << "\", ";
            if ( format != TenGen )
                s << "\"$id\" : ";
            s << '"' << *x << "\" ";
            if ( format == TenGen )
                s << ')';
            else
                s << '}';
            break;
        }
        case jstOID:
            if ( format == TenGen ) {
                s << "ObjectId( ";
            }
            else {
                s << "{ \"$oid\" : ";
            }
            s << '"' << __oid() << '"';
            if ( format == TenGen ) {
                s << " )";
            }
            else {
                s << " }";
            }
            break;
        case BinData: {
            s << "{ \"$binary\" : \"";
            int len;
            const char* start = binDataClean( len );
            BinDataType type = binDataType();
            base64::encode( s , start , len );
            s << "\", \"$type\" : \"" << hex;
            s.width( (streamsize)2 );
            s.fill( '0' );
            s << type << dec;
            s << "\" }";
            break;
        }
        case bson::Date:
            if ( format == Strict )
                s << "{ \"$date\" : ";
            else
                s << "Date( ";
            if( pretty ) {
                Date_t d = date();
                if( d == 0 ) s << '0';
                else
                    s << '"' << date().toString() << '"';
            }
            else
                s << date();
            if ( format == Strict )
                s << " }";
            else
                s << " )";
            break;
        case RegEx:
            if ( format == Strict ) {
                s << "{ \"$regex\" : \"" << escape( regex() );
                s << "\", \"$options\" : \"" << regexFlags() << "\" }";
            }
            else {
                s << "/" << escape( regex() , true ) << "/";
                // FIXME Worry about alpha order?
                for ( const char *f = regexFlags(); *f; ++f ) {
                    switch ( *f ) {
                    case 'g':
                    case 'i':
                    case 'm':
                        s << *f;
                    default:
                        break;
                    }
                }
            }
            break;

        case CodeWScope: {
            BSONObj scope = codeWScopeObject();
            if ( ! scope.isEmpty() ) {
                s << "{ \"$code\" : " << _asCode() << " , "
                  << " \"$scope\" : " << scope.jsonString() << " }";
                break;
            }
        }


        case Code:
            s << _asCode();
            break;

        case Timestamp:
            s << "{ \"t\" : " << timestampTime() << " , \"i\" : "
              << timestampInc() << " }";
            break;

        case MinKey:
            s << "{ \"$minKey\" : 1 }";
            break;

        case MaxKey:
            s << "{ \"$maxKey\" : 1 }";
            break;

        default:
            StringBuilder ss;
            ss << "Cannot create a properly formatted JSON string with "
               << "element: " << toString() << " of type: " << type();
            string message = ss.str();
            massert( 10312 ,  message.c_str(), false );
        }
        return s.str();
    }

    int BSONElement::getGtLtOp( int def ) const {
        const char *fn = fieldName();
        if ( fn[0] == '$' && fn[1] ) {
            if ( fn[2] == 't' ) {
                if ( fn[1] == 'g' ) {
                    if ( fn[3] == 0 ) return BSONObj::GT;
                    else if ( fn[3] == 'e' && fn[4] == 0 ) return BSONObj::GTE;
                }
                else if ( fn[1] == 'l' ) {
                    if ( fn[3] == 0 ) return BSONObj::LT;
                    else if ( fn[3] == 'e' && fn[4] == 0 ) return BSONObj::LTE;
                }
                else if ( fn[1] == 'e' ) {
                   return BSONObj::Equality ;
                }
            }
            else if ( fn[1] == 'n' && fn[2] == 'e' ) {
                if ( fn[3] == 0 )
                    return BSONObj::NE;

                // matches anything with $near prefix
                if ( fn[3] == 'a' && fn[4] == 'r')
                    return BSONObj::opNEAR;
            }
            else if ( fn[1] == 'f' && fn[2] == 'i' && fn[3] == 'e' &&
                      fn[4] == 'l' && fn[5] == 'd' && fn[6] == 0 )
            {
               return BSONObj::Equality ;
            }
            else if ( fn[1] == 'm' ) {
                if ( fn[2] == 'o' && fn[3] == 'd' && fn[4] == 0 )
                    return BSONObj::opMOD;
                if ( fn[2] == 'a' && fn[3] == 'x' && fn[4] == 'D' &&
                     fn[5] == 'i' && fn[6] == 's' && fn[7] == 't' &&
                     fn[8] == 'a' && fn[9] == 'n' && fn[10] == 'c' &&
                     fn[11] == 'e' && fn[12] == 0 )
                    return BSONObj::opMAX_DISTANCE;
            }
            else if ( fn[1] == 't' && fn[2] == 'y' && fn[3] == 'p' &&
                      fn[4] == 'e' && fn[5] == 0 )
                return BSONObj::opTYPE;
            else if ( fn[1] == 'i' ) {
                if ( fn[2] == 'n' && fn[3] == 0 )
                    return BSONObj::opIN;
                if ( fn[2] == 's' && fn[3] == 'n' && fn[4] == 'u' &&
                     fn[5] == 'l' && fn[6] == 'l' && fn[7] == 0 )
                    return BSONObj::opISNULL;
            }
            else if ( fn[1] == 'n' && fn[2] == 'i' && fn[3] == 'n' &&
                      fn[4] == 0 )
                return BSONObj::NIN;
            else if ( fn[1] == 'a' && fn[2] == 'l' && fn[3] == 'l' &&
                      fn[4] == 0 )
                return BSONObj::opALL;
            else if ( fn[1] == 's' && fn[2] == 'i' && fn[3] == 'z' &&
                      fn[4] == 'e' && fn[5] == 0 )
                return BSONObj::opSIZE;
            else if ( fn[1] == 'e' ) {
                if ( fn[2] == 'x' && fn[3] == 'i' && fn[4] == 's' &&
                     fn[5] == 't' && fn[6] == 's' && fn[7] == 0 )
                    return BSONObj::opEXISTS;
                if ( fn[2] == 'l' && fn[3] == 'e' && fn[4] == 'm' &&
                     fn[5] == 'M' && fn[6] == 'a' && fn[7] == 't' &&
                     fn[8] == 'c' && fn[9] == 'h' && fn[10] == 0 )
                    return BSONObj::opELEM_MATCH;
            }
            else if ( fn[1] == 'r' && fn[2] == 'e' && fn[3] == 'g' &&
                      fn[4] == 'e' && fn[5] == 'x' && fn[6] == 0 )
                return BSONObj::opREGEX;
            else if ( fn[1] == 'o' && fn[2] == 'p' && fn[3] == 't' &&
                      fn[4] == 'i' && fn[5] == 'o' && fn[6] == 'n' &&
                      fn[7] == 's' && fn[8] == 0 )
                return BSONObj::opOPTIONS;
            else if ( fn[1] == 'w' && fn[2] == 'i' && fn[3] == 't' &&
                      fn[4] == 'h' && fn[5] == 'i' && fn[6] == 'n' &&
                      fn[7] == 0 )
                return BSONObj::opWITHIN;
        }
        return def;
    }

    /* wo = "well ordered" */
    /* TODO(jbenet): does this belong here or in bson-inl.h?
    int BSONElement::woCompare( const BSONElement &e,
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
    }*/

    inline int compareLongValues( long long lValue, long long rValue )
    {
       return lValue == rValue ? 0 : lValue < rValue ? -1 : 1 ;
    }

    inline int compareIntValues( int lValue, int rValue )
    {
       return lValue == rValue ? 0 : lValue < rValue ? -1 : 1 ;
    }

    inline int compareNumberValues( const BSONElement &l, const BSONElement &r )
    {
#define LONG_UPPER_SAFE_BOUND (9007199254740991L)
#define LONG_LOWER_SAFE_BOUND (-9007199254740991L)
       double x;
       if ( r.type() == NumberLong )
       {
           long long R = r._numberLong();
           double L = l.numberDouble() ;
           // out of safe bound,
           // convert to double will loss precision
           if ( ( R > LONG_UPPER_SAFE_BOUND ||
                  R < LONG_LOWER_SAFE_BOUND ) &&
                ( L > LONG_UPPER_SAFE_BOUND ||
                  L < LONG_LOWER_SAFE_BOUND ) )
           {
               BSONDecimalElement lEle( l ) ;
               BSONDecimalElement rEle( r ) ;
               return lEle.numberDecimal().compare( rEle.numberDecimal() ) ;
           }
       }
       else if ( l.type() == NumberLong )
       {
           long long L = l._numberLong();
           double R = r.numberDouble() ;
           // out of safe bound,
           // convert to double will loss precision
           if ( ( L > LONG_UPPER_SAFE_BOUND ||
                  L < LONG_LOWER_SAFE_BOUND ) &&
                ( R > LONG_UPPER_SAFE_BOUND ||
                  R < LONG_LOWER_SAFE_BOUND ) )
           {
               BSONDecimalElement lEle( l ) ;
               BSONDecimalElement rEle( r ) ;
               return lEle.numberDecimal().compare( rEle.numberDecimal() ) ;
           }
       }

       int sign = 0 ;
       double left = l.number();
       double right = r.number();
       bool lNan = isNaN( left ) ;
       bool rNan = isNaN( right ) ;
       if ( lNan ) {
           if ( rNan ) {
               return 0;
           }
           else {
               return -1;
           }
       }
       else if ( rNan ) {
           return 1;
       }
       if( isInf( left, &sign ) && isInf( right, &sign ) )
       {
          if( left == right )
          {
             return 0 ;
          }
          else if( left < right )
          {
             return -1 ;
          }
          else
          {
             return 1 ;
          }
       }
       x = left - right;
       if ( x < 0 ) return -1;
       return x == 0 ? 0 : 1;
    }

    /* must be same type when called, unless both sides are #s*/
    int compareElementValues(const BSONElement& l, const BSONElement& r) {
        int f;
        if ( l.type() == NumberDecimal || r.type() == NumberDecimal )
        {
            BSONDecimalElement lEle( l ) ;
            BSONDecimalElement rEle( r ) ;
            return lEle.numberDecimal().compare( rEle.numberDecimal() ) ;
        }

        switch ( l.type() ) {
        case EOO:
        case Undefined:
        case jstNULL:
        case MaxKey:
        case MinKey:
            f = l.canonicalType() - r.canonicalType();
            if ( f<0 ) return -1;
            return f==0 ? 0 : 1;
        case Bool:
            return *l.value() - *r.value();
        case Timestamp:
        {
            if ( Timestamp == r.type() )
            {
               OpTime l_optime( l.date() ) ;
               OpTime r_optime( r.date() ) ;
               if ( l_optime < r_optime )
               {
                  return -1 ;
               }

               return l_optime == r_optime ? 0 : 1 ;
            }
            /// r is date
            else
            {
               long long L_Macro = ( long long ) l.timestampTime()
                                   + l.timestampInc() / 1000 ;
               long long R_Macro = r.date() ;
               if ( L_Macro - R_Macro != 0 )
               {
                  return L_Macro > R_Macro ? 1 : -1 ;
               }
               /// Macro-second is same, compare millisec
               return ( l.timestampInc() % 1000 ) > 0 ? 1 : 0 ;
            }
        }
        case Date:
        {
            if ( Date == r.type() )
            {
               long long iL = l.date() ;
               long long iR = r.date() ;
               if ( iL < iR )
                   return -1;
               return iL == iR ? 0 : 1;
            }
            /// r is timestamp
            else
            {
               long long L_Macro = l.date() ;
               long long R_Macro = ( long long ) r.timestampTime()
                                   + r.timestampInc() / 1000 ;
               if ( L_Macro - R_Macro != 0 )
               {
                  return L_Macro > R_Macro ? 1 : -1 ;
               }
               /// Macro-second is same, compare millisec
               return ( r.timestampInc() % 1000 ) > 0 ? -1 : 0 ;
            }
        }
        case NumberInt:
           if ( NumberInt == r.type() )
           {
              return compareIntValues( l._numberInt(), r._numberInt() ) ;
           }
           else if ( NumberLong == r.type() )
           {
              return compareLongValues( l._numberInt(), r._numberLong() ) ;
           }
           return compareNumberValues( l, r ) ;
        case NumberLong:
           if ( NumberInt == r.type() )
           {
              return compareLongValues( l._numberLong(), r._numberInt() ) ;
           }
           else if ( NumberLong == r.type() )
           {
              return compareLongValues( l._numberLong(), r._numberLong() ) ;
           }
           return compareNumberValues( l, r ) ;
        case NumberDouble:
            return compareNumberValues( l, r ) ;
        case jstOID:
            return memcmp(l.value(), r.value(), 12);
        case Code:
        case Symbol:
        case String:
            /* todo: utf version */
            return strcmp(l.valuestr(), r.valuestr());
        case Object:
        case Array:
            return l.embeddedObject().woCompare( r.embeddedObject() );
        case DBRef: {
            int lsz = l.valuesize();
            int rsz = r.valuesize();
            if ( lsz - rsz != 0 ) return lsz - rsz;
            return memcmp(l.value(), r.value(), lsz);
        }
        case BinData: {
            int lsz = l.objsize(); // our bin data size in bytes,
                                   // not including the subtype byte
            int rsz = r.objsize();
            if ( lsz - rsz != 0 ) return lsz - rsz;
            return memcmp(l.value()+4, r.value()+4, lsz+1);
        }
        case RegEx: {
            int c = strcmp(l.regex(), r.regex());
            if ( c )
                return c;
            return strcmp(l.regexFlags(), r.regexFlags());
        }
        case CodeWScope : {
            f = l.canonicalType() - r.canonicalType();
            if ( f )
                return f;
            f = strcmp( l.codeWScopeCode() , r.codeWScopeCode() );
            if ( f )
                return f;
            f = strcmp( l.codeWScopeScopeData() , r.codeWScopeScopeData() );
            if ( f )
                return f;
            return 0;
        }
        default:
            out() << "compareElementValues: bad type " << (int) l.type()
                  << endl;
            assert(false);
        }
        return -1;
    }

    /* Matcher --------------------------------------*/

// If the element is something like:
//   a : { $gt : 3 }
// we append
//   a : 3
// else we just append the element.
//
    void appendElementHandlingGtLt(BSONObjBuilder& b, const BSONElement& e) {
        if ( e.type() == Object ) {
            BSONElement fe = e.embeddedObject().firstElement();
            const char *fn = fe.fieldName();
            if ( fn[0] == '$' && fn[1] && fn[2] == 't' ) {
                b.appendAs(fe, e.fieldName());
                return;
            }
        }
        b.append(e);
    }

    int getGtLtOp(const BSONElement& e) {
        if ( e.type() != Object )
            return BSONObj::Equality;

        BSONElement fe = e.embeddedObject().firstElement();
        return fe.getGtLtOp();
    }

    /*
    FieldCompareResult compareDottedFieldNames(const string& l, const string& r)
    {
        static int maxLoops = 1024 * 1024;

        size_t lstart = 0;
        size_t rstart = 0;

        for ( int i=0; i<maxLoops; i++ ) {
            if ( lstart >= l.size() ) {
                if ( rstart >= r.size() )
                    return SAME;
                return RIGHT_SUBFIELD;
            }
            if ( rstart >= r.size() )
                return LEFT_SUBFIELD;

            size_t a = l.find( '.' , lstart );
            size_t b = r.find( '.' , rstart );

            size_t lend = a == string::npos ? l.size() : a;
            size_t rend = b == string::npos ? r.size() : b;

            const string& c = l.substr( lstart , lend - lstart );
            const string& d = r.substr( rstart , rend - rstart );

            int x = lexNumCmp( c.c_str(), d.c_str() );

            if ( x < 0 )
                return LEFT_BEFORE;
            if ( x > 0 )
                return RIGHT_BEFORE;

            lstart = lend + 1;
            rstart = rend + 1;
        }

        log() << "compareDottedFieldNames ERROR  l: " << l << " r: " << r
              << "  TOO MANY LOOPS" << endl;
        assert(0);
        return SAME; // will never get here
    }*/
    FieldCompareResult compareDottedFieldNames(const char* l, const char* r)
    {
        static int maxLoops = 1024 * 1024;

        const char *lstart = l ;
        const char *rstart = r ;

        for ( int i = 0 ; i < maxLoops ; i++ )
        {
            if ( '\0' == *lstart )
                return ( *rstart == '\0' ) ? SAME : RIGHT_SUBFIELD ;
            else if ( *rstart == '\0' )
                return LEFT_SUBFIELD ;

            // find the earliest '.' from current position
            const char *lnext = strchr ( lstart, '.' ) ;
            const char *rnext = strchr ( rstart, '.' ) ;

            // do string compare
            int x = lexNumCmp( lstart, rstart, true ) ;
            if ( x < 0 )
                return LEFT_BEFORE ;
            else if ( x > 0 )
                return RIGHT_BEFORE ;

            lstart = lnext ? ( lnext + 1 ) : "" ;
            rstart = rnext ? ( rnext + 1 ) : "" ;
        }

        log() << "compareDottedFieldNames ERROR  l: " << l << " r: " << r
              << "  TOO MANY LOOPS" << endl;
        assert(0);
        return SAME; // will never get here
    }

    /* BSONObj ------------------------------------------------------------*/

    bool BSONObj::_jsCompatibility = false;

    string BSONObj::md5() const
      { return md5::md5simpledigest((const md5_byte_t*)_objdata , objsize() ); }

    string BSONObj::jsonString( JsonStringFormat format, int pretty ) const {

        if ( isEmpty() ) return "{}";

        StringBuilder s;
        s << "{ ";
        BSONObjIterator i(*this);
        BSONElement e = i.next();
        if ( !e.eoo() )
            while ( 1 ) {
                s << e.jsonString( format, true, pretty?pretty+1:0 );
                e = i.next();
                if ( e.eoo() )
                    break;
                s << ",";
                if ( pretty ) {
                    s << '\n';
                    for( int x = 0; x < pretty; x++ )
                        s << "  ";
                }
                else {
                    s << " ";
                }
            }
        s << " }";
        return s.str();
    }

    bool BSONObj::valid() const {
        try {
            BSONObjIterator it( *this );
            while( it.moreWithEOO() ) {
                // both throw exception on failure
                BSONElement e = it.next(true);
                e.validate();

                if (e.eoo()) {
                    if (it.moreWithEOO())
                        return false;
                    return true;
                }
                else if (e.isABSONObj()) {
                    if(!e.embeddedObject().valid())
                        return false;
                }
                else if (e.type() == CodeWScope) {
                    if(!e.codeWScopeObject().valid())
                        return false;
                }
            }
        }
        catch (...) {
        }
        return false;
    }

    int BSONObj::woCompare(const BSONObj& r, const Ordering &o,
      bool considerFieldName) const {
        if ( isEmpty() )
            return r.isEmpty() ? 0 : -1;
        if ( r.isEmpty() )
            return 1;

        BSONObjIterator i(*this);
        BSONObjIterator j(r);
        unsigned mask = 1;
        while ( 1 ) {
            // so far, equal...

            BSONElement l = i.next();
            BSONElement r = j.next();
            if ( l.eoo() )
                return r.eoo() ? 0 : -1;
            if ( r.eoo() )
                return 1;

            int x;
            {
                x = l.woCompare( r, considerFieldName );
                if( o.descending(mask) )
                    x = -x;
            }
            if ( x != 0 )
                return x;
            mask <<= 1;
        }
        return -1;
    }

    /* well ordered compare */
    int BSONObj::woCompare(const BSONObj &r, const BSONObj &idxKey,
                           bool considerFieldName) const {
        if ( isEmpty() )
            return r.isEmpty() ? 0 : -1;
        if ( r.isEmpty() )
            return 1;

        bool ordered = !idxKey.isEmpty();

        BSONObjIterator i(*this);
        BSONObjIterator j(r);
        BSONObjIterator k(idxKey);
        while ( 1 ) {
            // so far, equal...

            BSONElement l = i.next();
            BSONElement r = j.next();
            BSONElement o;
            if ( ordered )
                o = k.next();
            if ( l.eoo() )
                return r.eoo() ? 0 : -1;
            if ( r.eoo() )
                return 1;

            int x;
            /*
                        if( ordered && o.type() == String && strcmp(o.valuestr(), "ascii-proto") == 0 &&
                            l.type() == String && r.type() == String ) {
                            // note: no negative support yet, as this is just sort of a POC
                            x = _stricmp(l.valuestr(), r.valuestr());
                        }
                        else*/ {
                x = l.woCompare( r, considerFieldName );
                if ( ordered && o.number() < 0 )
                    x = -x;
            }
            if ( x != 0 )
                return x;
        }
        return -1;
    }

    static BSONObj staticNullObj()
    {
       BSONObjBuilder b;
       return b.appendNull("").obj();
    }
    //BSONObj staticNull = fromjson( "{'':null}" );
    BSONObj staticNull = staticNullObj();

    static BSONObj staticUndefinedObj ()
    {
       BSONObjBuilder b ;
       return b.appendUndefined("").obj () ;
    }
    BSONObj staticUndefined = staticUndefinedObj () ;

    /* well ordered compare */
    int BSONObj::woSortOrder(const BSONObj& other, const BSONObj& sortKey ,
      bool useDotted ) const {
        if ( isEmpty() )
            return other.isEmpty() ? 0 : -1;
        if ( other.isEmpty() )
            return 1;

        uassert(10060, "woSortOrder needs a non-empty sortKey" ,
          !sortKey.isEmpty());

        BSONObjIterator i(sortKey);
        while ( 1 ) {
            BSONElement f = i.next();
            if ( f.eoo() )
                return 0;

            BSONElement l = useDotted ? getFieldDotted( f.fieldName() )
                                      : getField( f.fieldName() );
            if ( l.eoo() )
                l = staticNull.firstElement();
            BSONElement r = useDotted ? other.getFieldDotted( f.fieldName() )
                                      : other.getField( f.fieldName() );
            if ( r.eoo() )
                r = staticNull.firstElement();

            int x = l.woCompare( r, false );
            if ( f.number() < 0 )
                x = -x;
            if ( x != 0 )
                return x;
        }
        return -1;
    }

    void BSONObj::getFieldsDotted(const StringData& name, BSONElementSet &ret )
      const {
        BSONElement e = getField( name );
        if ( e.eoo() ) {
            const char *p = strchr(name.data(), '.');
            if ( p ) {
                StringData left(name.data(), p-name.data());
                const char* next = p+1;
                BSONElement e = getField( left );

                if (e.type() == Object) {
                    e.embeddedObject().getFieldsDotted(next, ret);
                }
                else if (e.type() == Array) {
                    bool allDigits = false;
                    if ( isdigit( *next ) ) {
                        const char * temp = next + 1;
                        while ( isdigit( *temp ) )
                            temp++;
                        allDigits = (*temp == '.' || *temp == '\0');
                    }
                    if (allDigits) {
                        e.embeddedObject().getFieldsDotted(next, ret);
                    }
                    else {
                        BSONObjIterator i(e.embeddedObject());
                        while ( i.more() ) {
                            BSONElement e2 = i.next();
                            if (e2.type() == Object || e2.type() == Array)
                                e2.embeddedObject().getFieldsDotted(next, ret);
                        }
                    }
                }
                else {
                    // do nothing: no match
                }
            }
        }
        else {
            if (e.type() == Array) {
                BSONObjIterator i(e.embeddedObject());
                while ( i.more() )
                    ret.insert(i.next());
            }
            else {
                ret.insert(e);
            }
        }
    }

    BSONElement BSONObj::getFieldDottedOrArray(const char *&name) const {
        const char *p = strchr(name, '.');

        BSONElement sub;

        if ( p ) {
            sub = getField( StringData(name, p-name) );
            name = p + 1;
        }
        else {
            sub = getField( name );
            name = name + strlen(name);
        }

        if ( sub.eoo() )
            return nullElement;
        else if ( sub.type() == Array || name[0] == '\0')
            return sub;
        else if ( sub.type() == Object )
            return sub.embeddedObject().getFieldDottedOrArray( name );
        else
            return nullElement;
    }

    /**
     sets element field names to empty string
     If a field in pattern is missing, it is omitted from the returned
     object.
     */
    BSONObj BSONObj::extractFieldsUnDotted(BSONObj pattern) const {
        BSONObjBuilder b;
        BSONObjIterator i(pattern);
        while ( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            BSONElement x = getField(e.fieldName());
            if ( !x.eoo() )
                b.appendAs(x, "");
        }
        return b.obj();
    }

    BSONObj BSONObj::extractFields(const BSONObj& pattern , bool fillWithNull )
      const {
        BSONObjBuilder b(32); // scanandorder.h can make a zillion of these,
                              // so we start the allocation very small
        BSONObjIterator i(pattern);
        while ( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            BSONElement x = getFieldDotted(e.fieldName());
            if ( ! x.eoo() )
                b.appendAs( x, e.fieldName() );
            else if ( fillWithNull )
                b.appendNull( e.fieldName() );
        }
        return b.obj();
    }

    BSONObj BSONObj::filterFieldsUndotted(const BSONObj &filter, bool inFilter)
      const {
        BSONObjBuilder b;
        BSONObjIterator i( *this );
        while( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            BSONElement x = filter.getField( e.fieldName() );
            if ( ( x.eoo() && !inFilter ) ||
                    ( !x.eoo() && inFilter ) )
                b.append( e );
        }
        return b.obj();
    }

    BSONElement BSONObj::getFieldUsingIndexNames(const char *fieldName,
      const BSONObj &indexKey) const {
        BSONObjIterator i( indexKey );
        int j = 0;
        while( i.moreWithEOO() ) {
            BSONElement f = i.next();
            if ( f.eoo() )
                return BSONElement();
            if ( strcmp( f.fieldName(), fieldName ) == 0 )
                break;
            ++j;
        }
        BSONObjIterator k( *this );
        while( k.moreWithEOO() ) {
            BSONElement g = k.next();
            if ( g.eoo() )
                return BSONElement();
            if ( j == 0 ) {
                return g;
            }
            --j;
        }
        return BSONElement();
    }

    /* TOOD(jbenet) evaluate whether these belong there:
    int BSONObj::getIntField(const char *name) const {
        BSONElement e = getField(name);
        return e.isNumber() ? (int) e.number() : INT_MIN;
    }

    bool BSONObj::getBoolField(const char *name) const {
        BSONElement e = getField(name);
        return e.type() == Bool ? e.boolean() : false;
    }

    const char * BSONObj::getStringField(const char *name) const {
        BSONElement e = getField(name);
        return e.type() == String ? e.valuestr() : "";
    }
    */

    /* grab names of all the fields in this object */
    int BSONObj::getFieldNames(set<string>& fields) const {
        int n = 0;
        BSONObjIterator i(*this);
        while ( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            fields.insert(e.fieldName());
            n++;
        }
        return n;
    }

    bool BSONObj::hasAllFieldNames(const BSONObj &obj) const {
        if ( obj.isEmpty() )
            return true;
        else if ( isEmpty() )
            return false;

        BSONObjIterator i(obj);
        while ( i.more() ) {
            BSONElement e = i.next();
            if ( !hasField( e.fieldName() ) ) {
                return false;
            }
        }
        return true;
    }

    /* note: addFields always adds _id even if not specified
       returns n added not counting _id unless requested.
    */
    int BSONObj::addFields(BSONObj& from, set<string>& fields) {
        assert( isEmpty() && !isOwned() ); /* partial implementation for now. */

        BSONObjBuilder b;

        int N = fields.size();
        int n = 0;
        BSONObjIterator i(from);
        bool gotId = false;
        while ( i.moreWithEOO() ) {
            BSONElement e = i.next();
            const char *fname = e.fieldName();
            if ( fields.count(fname) ) {
                b.append(e);
                ++n;
                gotId = gotId || strcmp(fname, "_id")==0;
                if ( n == N && gotId )
                    break;
            }
            else if ( strcmp(fname, "_id")==0 ) {
                b.append(e);
                gotId = true;
                if ( n == N && gotId )
                    break;
            }
        }

        if ( n ) {
            int len;
            init( b.decouple(len) );
        }

        return n;
    }

    BSONObj BSONObj::clientReadable() const {
        BSONObjBuilderOption option( true, false );
        BSONObjBuilder b;
        b.appendEx( *this, option );
        return b.obj();
    }

    BSONObj BSONObj::replaceFieldNames( const BSONObj &names ) const {
        BSONObjBuilder b;
        BSONObjIterator i( *this );
        BSONObjIterator j( names );
        BSONElement f = j.moreWithEOO() ? j.next() : BSONObj().firstElement();
        while( i.moreWithEOO() ) {
            BSONElement e = i.next();
            if ( e.eoo() )
                break;
            if ( !f.eoo() ) {
                b.appendAs( e, f.fieldName() );
                f = j.next();
            }
            else {
                b.append( e );
            }
        }
        return b.obj();
    }

    bool BSONObj::okForStorage() const {
        BSONObjIterator i( *this );
        while ( i.more() ) {
            BSONElement e = i.next();
            const char * name = e.fieldName();

            if ( strchr( name , '.' ) ||
                    strchr( name , '$' ) ) {
                return
                    strcmp( name , "$ref" ) == 0 ||
                    strcmp( name , "$id" ) == 0
                    ;
            }

            if ( e.mayEncapsulate() ) {
                switch ( e.type() ) {
                case Object:
                case Array:
                    if ( ! e.embeddedObject().okForStorage() )
                        return false;
                    break;
                case CodeWScope:
                    if ( ! e.codeWScopeObject().okForStorage() )
                        return false;
                    break;
                default:
                  uassert(12579, "unhandled cases in BSONObj okForStorage", 0);
                }

            }
        }
        return true;
    }

    void BSONObj::dump() const {
        out() << hex;
        const char *p = objdata();
        for ( int i = 0; i < objsize(); i++ ) {
            out() << i << '\t' << ( 0xff & ( (unsigned) *p ) );
            if ( *p >= 'A' && *p <= 'z' )
                out() << '\t' << *p;
            out() << endl;
            p++;
        }
    }

    string BSONObj::hexDump() const {
        stringstream ss;
        const char *d = objdata();
        int size = objsize();
        for( int i = 0; i < size; ++i ) {
            ss.width( (streamsize)2 );
            ss.fill( '0' );
            ss << hex << (unsigned)(unsigned char)( d[ i ] ) << dec;
            if ( ( d[ i ] >= '0' && d[ i ] <= '9' ) ||
                 ( d[ i ] >= 'A' && d[ i ] <= 'z' ) )
                ss << '\'' << d[ i ] << '\'';
            if ( i != size - 1 )
                ss << ' ';
        }
        return ss.str();
    }

    void nested2dotted(BSONObjBuilder& b, const BSONObj& obj,
      const string& base) {
        BSONObjIterator it(obj);
        while (it.more()) {
            BSONElement e = it.next();
            if (e.type() == Object) {
                string newbase = base + e.fieldName() + ".";
                nested2dotted(b, e.embeddedObject(), newbase);
            }
            else {
                string newbase = base + e.fieldName();
                b.appendAs(e, newbase);
            }
        }
    }

    void dotted2nested(BSONObjBuilder& b, const BSONObj& obj) {
        //use map to sort fields
        BSONMap sorted = bson2map(obj);
        EmbeddedBuilder eb(&b);
        for(BSONMap::const_iterator it=sorted.begin(); it!=sorted.end(); ++it) {
            eb.appendAs(it->second, it->first);
        }
        eb.done();
    }

    /*-- test things ----------------------------------------------------*/

#pragma pack(1)
    struct MaxKeyData {
        MaxKeyData() {
            totsize=7;
            maxkey=MaxKey;
            name=0;
            eoo=EOO;
        }
        int totsize;
        char maxkey;
        char name;
        char eoo;
    } maxkeydata;
    BSONObj maxKey((const char *) &maxkeydata);

    struct MinKeyData {
        MinKeyData() {
            totsize=7;
            minkey=MinKey;
            name=0;
            eoo=EOO;
        }
        int totsize;
        char minkey;
        char name;
        char eoo;
    } minkeydata;
    BSONObj minKey((const char *) &minkeydata);

    Labeler::Label GT( "$gt" );
    Labeler::Label GTE( "$gte" );
    Labeler::Label LT( "$lt" );
    Labeler::Label LTE( "$lte" );
    Labeler::Label NE( "$ne" );
    Labeler::Label SIZE( "$size" );

#pragma pack()

    void BSONObjBuilder::appendMinForType(const StringData& fieldName, int t) {
        switch ( t ) {
        case MinKey: appendMinKey( fieldName ); return;
        case MaxKey: appendMinKey( fieldName ); return;
        case NumberInt:
        case NumberDouble:
        case NumberLong:
            append( fieldName , - numeric_limits<double>::max() ); return;
        case NumberDecimal:
        {
            bsonDecimal decimal ;
            decimal.setMin() ;
            append( fieldName, decimal ) ;
            return ;
        }
        case jstOID: {
            OID o;
            memset(&o, 0, sizeof(o));
            appendOID( fieldName , &o);
            return;
        }
        case Bool: appendBool( fieldName , false); return;
        case Date:
        {
            appendDate( fieldName , numeric_limits<INT64>::min() ) ;
            return ;
        }
        case jstNULL: appendNull( fieldName ); return;
        case Symbol:
        case String: append( fieldName , "" ); return;
        case Object: append( fieldName , BSONObj() ); return;
        case Array:
            appendArray( fieldName , BSONObj() ); return;
        case BinData:
            appendBinData( fieldName , 0 , Function , (const char *) 0 );
            return;
        case Undefined:
            appendUndefined( fieldName ); return;
        case RegEx: appendRegex( fieldName , "" ); return;
        case DBRef: {
            OID o;
            memset(&o, 0, sizeof(o));
            appendDBRef( fieldName , "" , o );
            return;
        }
        case Code: appendCode( fieldName , "" ); return;
        case CodeWScope: appendCodeWScope( fieldName , "" , BSONObj() ); return;
        case Timestamp:
        {
            OpTime t( numeric_limits<SINT32>::min(), 0 ) ;
            appendTimestamp( fieldName, t.asDate() ) ;
            return ;
        }

        };
        log() << "type not support for appendMinElementForType: " << t << endl;
        uassert(10061, "type not supported for appendMinElementForType", false);
    }

    void BSONObjBuilder::appendMaxForType(const StringData& fieldName, int t) {
        switch ( t ) {
        case MinKey: appendMaxKey( fieldName );  break;
        case MaxKey: appendMaxKey( fieldName ); break;
        case NumberInt:
        case NumberDouble:
        case NumberLong:
            append( fieldName , numeric_limits<double>::max() );
            break;
        case NumberDecimal:
        {
            bsonDecimal decimal ;
            decimal.setMax() ;
            append( fieldName, decimal ) ;
            break ;
        }
        case BinData:
            appendMinForType( fieldName , jstOID );
            break;
        case jstOID: {
            OID o;
            memset(&o, 0xFF, sizeof(o));
            appendOID( fieldName , &o);
            break;
        }
        case Undefined:
        case jstNULL:
        {
           appendMinForType( fieldName, NumberInt ) ;
           break ;
        }
        case Bool:
        {
           appendBool( fieldName, true ) ;
           break;
        }
        case Date:
        {
            appendDate( fieldName , numeric_limits<INT64>::max() ) ;
            break;
        }
        case Symbol:
        case String: append( fieldName , BSONObj() ); break;
        case Code:
        case CodeWScope:
            appendCodeWScope( fieldName , "ZZZ" , BSONObj() ); break;
        case Timestamp:
        {
            OpTime t( numeric_limits<SINT32>::max(), 999999 ) ;
            appendTimestamp( fieldName, t.asDate() ) ;
            break;
        }
        default:
            appendMinForType( fieldName , t + 1 );
        }
    }

    bool BSONObjBuilder::appendDecimal( const StringData& fieldName,
                                        const StringData& strDecimal,
                                        int precision, int scale )
    {
        int rc = 0 ;
        bsonDecimal decimal ;
        rc = decimal.init( precision, scale ) ;
        if ( 0 != rc )
        {
            return false ;
        }

        rc = decimal.fromString( strDecimal.data() ) ;
        if ( 0 != rc )
        {
            return false ;
        }

        append( fieldName, decimal ) ;

        return true ;
    }

    bool BSONObjBuilder::appendDecimal( const StringData& fieldName,
                                        const StringData& strDecimal )
    {
        int rc = 0 ;
        bsonDecimal decimal ;
        rc = decimal.fromString( strDecimal.data() ) ;
        if ( 0 != rc )
        {
            return false ;
        }

        append( fieldName, decimal ) ;

        return true ;
    }

    const string BSONObjBuilder::numStrs[] = {
        "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
        "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
        "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
        "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
        "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
        "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
        "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
        "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
        "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
        "90", "91", "92", "93", "94", "95", "96", "97", "98", "99",
    };

    bool BSONObjBuilder::appendAsNumber( const StringData& fieldName ,
      const StringData& data ) {
        const char* p = data.data() ;
        if ( data.size() == 0 || ( data.size() == 1 && p[0] == '-' ) )
            return false;

        unsigned int pos = 0 ;
        if ( p[0] == '-' )
            pos++;

        bool hasDec = false;

        for ( ; pos < data.size() ; pos++ ) {
            if ( isdigit(p[pos]) )
                continue;

            if ( p[pos] == '.' ) {
                if ( hasDec )
                    return false;
                hasDec = true;
                continue;
            }

            return false;
        }

        if ( hasDec ) {
            double d = atof( p );
            append( fieldName , d );
            return true;
        }

        if ( data.size() < 8 ) {
            append( fieldName , atoi( p ) );
            return true;
        }
        char *pEndPtr = NULL ;
#if defined (_WIN32)
        long long num = _strtoi64 ( p, &pEndPtr, 10 ) ;
#else
        long long num = strtoll ( p, &pEndPtr, 10 ) ;
#endif
        if ( '\0' != *pEndPtr )
           return false ;
        append( fieldName , num );
        return true ;
/*        try {
            long long num = boost::lexical_cast<long long>( p );
            append( fieldName , num );
            return true;
        }
        catch(boost::bad_lexical_cast &) {
            return false;
        }*/

    }

    void BSONObjBuilder::appendKeys( const BSONObj& keyPattern ,
      const BSONObj& values ) {
        BSONObjIterator i(keyPattern);
        BSONObjIterator j(values);

        while ( i.more() && j.more() ) {
            appendAs( j.next() , i.next().fieldName() );
        }

        assert( ! i.more() );
        assert( ! j.more() );
    }

    int BSONElementFieldSorter( const void * a , const void * b ) {
        const char * x = *((const char**)a);
        const char * y = *((const char**)b);
        x++; y++;
        return lexNumCmp( x , y, false ) ;
    }

    BSONObjIteratorSorted::BSONObjIteratorSorted( const BSONObj& o ) {
        _fields = 0;
        _nfields = o.nFields();
        if ( _nfields <= BSONOBJITERSORTED_DFTFIELDS )
        {
           _fields = &_staticFields[0] ;
        }
        else
        {
           _fields = new const char*[_nfields];
        }
        int x = 0;
        BSONObjIterator i( o );
        while ( i.more() && x < _nfields ) {
            _fields[x++] = i.next().rawdata() ;
            assert( _fields[x-1] );
        }
        if ( i.more() )
        {
           // more elements after eoo, we need to throw exception
           // release fields first
           if ( _fields != &_staticFields[0] )
           {
               delete[] _fields;
           }
           _fields = 0;
           massert( 10337, "Invalid BSONObj with more data after EOO",
                    false );
        }
        assert( x == _nfields );
        qsort( _fields , _nfields , sizeof(char*) , BSONElementFieldSorter );
        _cur = 0;
    }

    /** transform a BSON array into a vector of BSONElements.
        we match array # positions with their vector position, and ignore
        any fields with non-numeric field names.
        */
    vector<BSONElement> BSONElement::Array() const {
        chk(bson::Array);
        vector<BSONElement> v;
        BSONObjIterator i(Obj());
        while( i.more() ) {
            BSONElement e = i.next();
            const char *f = e.fieldName();
            try {
                unsigned u = stringToNum(f);
                assert( u < 1000000 );
                if( u >= v.size() )
                    v.resize(u+1);
                v[u] = e;
            }
            catch(unsigned) { }
        }
        return v;
    }

} // namespace bson
