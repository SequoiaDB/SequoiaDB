/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ixmKey.cpp

   Descriptive Name = Index Manager Key

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   keys. There are two major types of key. Compact and BSON. For BSON key it
   does not include field name. For compact mode it is grouped with special
   format, and can be safely converted into normal BSON format.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ixmKey.hpp"
#include "../bson/ordering.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"
namespace engine
{
   // each field has a 1 byte prefix, including 8bits:
   // 1          1          1   1   1  1  1  1
   // ---------- ---------- --- --- -----------
   // [NOT USED] [HAS MORE] [x] [y] [canontype]
   enum _ixmCanonicalTypes
   {
      cminkey         = 1   , // minkey
      cundefined      = 2   , // UNDEFINED
      cnull           = 3   , // NULL
      cdouble         = 4   , // DOUBLE
      cstring         = 6   , // STRING
      cbindata        = 7   , // Binary Data
      coid            = 8   , // Object ID
      cfalse          = 10  , // FALSE
      ctrue           = 11  , // TRUE
      cdate           = 12  , // Date
      cmaxkey         = 14  , // Max Key
      cCANONTYPEMASK  = 0xF , // Mask for type
      cY              = 0x10 , // if both cY and cdouble are set, then it's int
      cint            = cY | cdouble ,
      cX              = 0x20 , // if both cX and cdouble set, then it's long
      clong           = cX | cdouble ,
      cHASMORE        = 0x40 , // if this bit is set, then we have more elements
      cNOTUSED        = 0x80
   } ;
   // note that we convert long long/ double and int to cdouble with cX/cY
   // prefix, this is useful when we attempt to compare multiple types of values
   // (ex, double to long long)
   // little additional overhead is needed when converting from long long /int
   // to double, but this makes comparison faster

   const UINT32 BinDataLenMask  = 0xF0 ; // lengths are power of 2 of this val
   const UINT32 BinDataTypeMask = 0x0F; // 0-7 are type
   const INT32  BinDataLenMax   = 32 ; // max 2^32
   // map 32 bits to 16 bits (0xFF to 0xF), use -1 in some slot
   const INT32 BinDataLengthToCode[] =
   {
      0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
      0x80, -1/*9*/, 0x90/*10*/, -1/*11*/, 0xa0/*12*/, -1/*13*/, 0xb0/*14*/,
      -1/*15*/, 0xc0/*16*/, -1, -1, -1, 0xd0/*20*/, -1, -1, -1,
      0xe0/*24*/, -1, -1, -1, -1, -1, -1, -1, 0xf0/*32*/
   } ;
   const INT32 BinDataCodeToLength[] =
   {
      0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 32
   } ;
   INT32 binDataCodeToLength ( INT32 codeByte )
   {
      return BinDataCodeToLength[codeByte >> 4];
   }

   // convert from BSON to index key
   _ixmKeyOwned::_ixmKeyOwned ( const BSONObj & obj )
   {
      BSONObj::iterator i(obj) ;
      UINT8 bits = 0 ;
      while ( TRUE )
      {
         BSONElement e = i.next() ;
         // if there's still more elements, let's set HASMORE bit
         if ( i.more() )
            bits |= cHASMORE ;
         switch ( e.type() )
         {
         // one byte for minkey
         case MinKey:
            _b.appendUChar ( cminkey|bits ) ;
            break ;
         // one byte for null
         case jstNULL:
             _b.appendUChar(cnull|bits);
             break;
         // one byte for undefined
         case Undefined:
             _b.appendUChar(cundefined|bits);
             break;
         // one byte for maxkey
         case MaxKey:
             _b.appendUChar(cmaxkey|bits);
             break;
         // one byte for true/false
         case Bool:
             _b.appendUChar( (e.boolean()?ctrue:cfalse) | bits );
             break;
         // oid type byte + oid data
         case jstOID:
             _b.appendUChar(coid|bits);
             _b.appendBuf(&e.__oid(), sizeof(OID));
             break;
         // binary data, 1 byte type + 1 byte code + x bytes data
         // or call traditional() function if the type is not supported
         case BinData:
         {
            INT32 t = e.binDataType () ;
            // 0-7 and 0x80 to 0x87 are supported by Key
            if ( (t&0x78) == 0 && t!=ByteArrayDeprecated )
            {
               INT32 len ;
               const CHAR *d = e.binData(len) ;
               if ( len <= BinDataLenMax )
               {
                  INT32 code = BinDataLengthToCode[len] ;
                  if ( code >=0 )
                  {
                     if ( t >= 128 )
                        t = (t-128) | 0x08 ;
                     if ( (code & t) == 0 )
                     {
                        _b.appendUChar ( cbindata|bits ) ;
                        _b.appendUChar ( code | t ) ;
                        _b.appendBuf ( d, len ) ;
                        break ;
                     }
                  }
               }
            }
            // if the bin data is not supported format, we just make traditional
            // object and return
            _traditional ( obj ) ;
            return ;
         }
         // date type, 1 byte type + date struct
         case Date :
            _b.appendUChar ( cdate|bits ) ;
            _b.appendStruct ( e.date() ) ;
            break ;
         // string type, 1 byte type + 1 byte size + string
         // if the size > 255 then we store traditional
         case String :
         {
            _b.appendUChar ( cstring|bits ) ;
            // note we don't store the terminating null to save space
            UINT32 len = (UINT32)e.valuestrsize() -1 ;
            if ( len > 255 )
            {
               _traditional ( obj ) ;
               return ;
            }
            _b.appendUChar((UINT8)len) ;
            _b.appendBuf ( e.valuestr(), len ) ;
            break ;
         }
         // 32 bit integer type
         case NumberInt :
            _b.appendUChar ( cint|bits ) ;
            // since int is represent by cY + cdouble, so we store as double
            _b.appendNum ( (FLOAT64)e._numberInt() ) ;
            break ;
         // 64 bit long long type
         case NumberLong :
         {
            SINT64 n = e._numberLong() ;
            // double integer bits
            SINT64 m = 2LL << 52 ;
            if ( n>= m || n<= -m )
            {
               // this big number can't be converted to double without lossing
               // digits
               _traditional ( obj ) ;
               return ;
            }
            _b.appendUChar ( clong | bits ) ;
            _b.appendNum ( (FLOAT64)n ) ;
            break ;
         }
         // double type
         case NumberDouble :
         {
            FLOAT64 d = e._numberDouble() ;
            if ( isNaN (d) )
            {
               // NaN type can't be represented in compact mode
               _traditional ( obj ) ;
               return ;
            }
            _b.appendUChar ( cdouble | bits ) ;
            _b.appendNum(d) ;
            break ;
         }
         // other types using traditional
         default :
            _traditional ( obj ) ;
            return ;
         }
         if ( !i.more() )
            break ;
         bits = 0 ;
      }
      _keyData = (const UINT8*) _b.buf() ;
      SDB_ASSERT ( _b.len() == dataSize(),
                   "data size must be same as builder" ) ;
      SDB_ASSERT ( (*_keyData & cNOTUSED)==0,
                   "data type is invalid" ) ;
   }
   // convert from existing key to owned key
   _ixmKeyOwned::_ixmKeyOwned ( const _ixmKey &r )
   {
      _b.appendBuf ( r.data(), r.dataSize() ) ;
      _keyData = (const UINT8*) _b.buf() ;
      SDB_ASSERT ( _b.len() == dataSize(),
                   "builder length must be same as data length" ) ;
      SDB_ASSERT ( (*_keyData & cNOTUSED) == 0,
                   "Flag is not correct" ) ;
   }

   // check whether there is a null field or undefined field
   BOOLEAN _ixmKey::hasNullOrUndefined() const
   {
      BOOLEAN foundOut = FALSE ;

      if ( _keyData == 0 )
      {
         goto done ;
      }

      if ( !isCompactFormat() )
      {
         // if it's native bson type, we have to go through each element and
         // check whether if it's undefined or null
         BSONObjIterator it ( _bson() ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            if ( Undefined == ele.type() || jstNULL == ele.type() )
            {
               foundOut = TRUE ;
               break ;
            }
         }
         goto done ;
      }
      else
      {
         // it is compact format, we need to loop through all types and
         // check whether any field is cundefined or cnull.
         const UINT8 *p = _keyData ;
         while ( TRUE )
         {
            UINT8 bits = *p++ ;
            switch ( bits & cCANONTYPEMASK )
            {
               case cnull:
               case cundefined:
                  foundOut = TRUE ;
                  break ;
               case cminkey:
               case cmaxkey:
               case cfalse:
               case ctrue:
                  break ;
               case cstring:
               {
                  UINT8 len = *p++ ;
                  p += len ;
                  break ;
               }
               case cbindata:
               {
                  INT32 len = binDataCodeToLength( *p++ ) ;
                  p += len ;
               }
               case coid:
                  p += sizeof( OID ) ;
                  break ;
               case cdate:
                  p += sizeof( Date_t ) ;
                  break ;
               case cdouble:
                  p += sizeof( FLOAT64 ) ;
                  break ;
               default:
                  PD_LOG( PDERROR, "Invalid key is accessed" ) ;
                  throw pdGeneralException( "Invalid Key is accessed" ) ;
            }

            if ( foundOut || 0 == ( bits & cHASMORE ) )
            {
               break ;
            }
         }
      }

   done:
      return foundOut ;
   }

   // check whether all keys are undefined type
   BOOLEAN _ixmKey::isUndefined() const
   {
      if ( _keyData == 0 )
         return FALSE ;
      if ( !isCompactFormat() )
      {
         // if it's native bson type, we have to go through each element and
         // check whether if it's Undefined, if any element doesn't match, we
         // return FALSE
         BSONObjIterator it ( _bson() ) ;
         while ( it.more() )
         {
            if ( it.next().type() != Undefined )
               return FALSE ;
         }
      }
      else
      {
         // loop through all types and check whether we got cundefined for all
         // fields
         const UINT8 *p = _keyData ;
         while ( TRUE )
         {
            UINT8 bits = *p++ ;
            if ( cundefined != ( bits & 0x3F ) )
               return FALSE ;
            // break when we don't have any more fields
            if ( (bits & cHASMORE) == 0 )
               break ;
         }
      }
      return TRUE ;
   }

   void _ixmKey::_toBson(BSONObjBuilder &b) const
   {
      const UINT8 *p = _keyData ;
      while ( TRUE )
      {
         UINT8 bits = *p++ ;
         // type and cX and cY
         switch ( bits & 0x3F )
         {
         case cminkey: b.appendMinKey("") ;      break ;
         case cnull:   b.appendNull("") ;        break ;
         case cundefined: b.appendUndefined("") ; break ;
         case cfalse:  b.appendBool("", FALSE) ; break ;
         case ctrue:   b.appendBool("", TRUE ) ; break ;
         case cmaxkey: b.appendMaxKey("") ;      break ;
         case cstring:
         {
            UINT8 len = *p++ ;
            BufBuilder &bb = b.bb() ;
            bb.appendNum((CHAR)String) ; // field type
            bb.appendUChar(0) ;          // field name
            bb.appendNum(len+1) ;        // string len
            bb.appendBuf(p, len) ;       // string text
            bb.appendUChar(0) ;          // '\0'
            p += len ;
            break ;
         }
         case coid:
            b.appendOID ( "", (OID*) p ) ;
            p += sizeof(OID) ;
            break ;
         case cbindata:
         {
            SINT32 len = binDataCodeToLength(*p) ;
            SINT32 subtype = (*p) & BinDataTypeMask ;
            if ( subtype & 0x08 )
            {
               subtype = (subtype & 0x07)|0x80 ;
            }
            b.appendBinData ( "", len, (BinDataType)subtype, ++p ) ;
            p += len ;
            break ;
         }
         case cdate:
            b.appendDate("", (Date_t&) *p) ;
            p += sizeof(Date_t) ;
            break ;
         case cdouble:
            b.append("", (FLOAT64&) *p) ;
            p += sizeof(FLOAT64) ;
            break ;
         case cint:
            b.append("", static_cast<SINT32>((reinterpret_cast<const
                     PackedDouble&>(*p)).d)) ;
            p+=sizeof(FLOAT64) ;
            break ;
         case clong:
            b.append("", static_cast<SINT64>((reinterpret_cast<const
                     PackedDouble&>(*p)).d)) ;
            p+=sizeof(FLOAT64) ;
            break ;
         default:
            pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                    "Invalid Key is accessed" ) ;
            throw pdGeneralException( "Invalid Key is accessed" ) ;
         }
         if ( (bits & cHASMORE) == 0 )
            break ;
      }
   }

   // convert from key to bson
   BSONObj _ixmKey::toBson(BufBuilder *builder) const
   {
      if ( _keyData == 0 )
         return BSONObj() ;
      if ( !isCompactFormat() )
         return _bson() ;

      if ( builder )
      {
         BSONObjBuilder b(*builder) ;
         _toBson(b);
         return BSONObj(b.done());
      }
      else
      {
         BSONObjBuilder b(512) ;
         _toBson(b);
         return BSONObj(b.obj());
      }
   }

   // convert a regular BSON object to key
   // this is done by adding a single 0xff at first byte (using b.appendUChar)
   // and then append obj.objdata()
   void _ixmKeyOwned::_traditional(const BSONObj &obj)
   {
      _b.reset() ;
      _b.appendUChar ( IsBSON ) ;
      _b.appendBuf ( obj.objdata(), obj.objsize() ) ;
      _keyData = (const UINT8 *)_b.buf() ;
   }
   // compare of two compact buffer
   static INT32 compare(const UINT8 *&l, const UINT8 *&r)
   {
      // left type
      UINT32 lt = (*l & cCANONTYPEMASK);
      // right type
      UINT32 rt = (*r & cCANONTYPEMASK);
      UINT32 x = lt - rt;
      // if types are different, return the diff
      if( x )
         return x;
      // both left and right move to next byte
      l++; r++;
      // now left and right are the same type
      switch( lt )
      {
      // all int/double/float types are converting to double, so this is
      // actually not limited to double type
      case cdouble:
      {
         FLOAT64 L = (reinterpret_cast< const PackedDouble* >(l))->d;
         FLOAT64 R = (reinterpret_cast< const PackedDouble* >(r))->d;
         if( L < R )
            return -1;
         if( L != R )
            return 1;
         l += sizeof(FLOAT64); r += sizeof(FLOAT64);
         break;
      }
      case cstring:
      {
         INT32 lsz = *l;
         INT32 rsz = *r;
         INT32 common = OSS_MIN(lsz, rsz);
         l++; r++; // skip the size byte
         // use memcmp as we (will) allow zeros in UTF8 strings
         INT32 res = ossMemcmp(l, r, common);
         if( res )
            return res;
         // longer string is the greater one
         INT32 diff = lsz-rsz;
         if( diff )
            return diff;
         l += lsz; r += lsz;
         break;
      }
      case cbindata:
      {
         INT32 L = *l;
         INT32 R = *r;
         INT32 llen = binDataCodeToLength(L);
         INT32 diff = L-R; // checks length and subtype simultaneously
         if( diff )
         {
            // unfortunately nibbles are backwards to do subtype and len
            // in one check (could bit swap...)
            INT32 rlen = binDataCodeToLength(R);
            if( llen != rlen )
               return llen - rlen;
            return diff;
         }
         // same length, same type
         l++; r++;
         INT32 res = ossMemcmp(l, r, llen);
         if( res )
            return res;
         l += llen; r += llen;
         break;
      }
      case cdate:
      {
         INT64 L = *((INT64 *) l);
         INT64 R = *((INT64 *) r);
         if( L < R )
            return -1;
         if( L > R )
            return 1;
         l += sizeof(INT64); r += sizeof(INT64);
         break;
      }
      case coid:
      {
         INT32 res = ossMemcmp(l, r, sizeof(OID));
         if( res )
            return res;
         l += sizeof(OID); r += sizeof(OID);
         break;
      }
      default:
         // all the others are a match -- e.g. null == null
         ;
      }
      return 0;
   }

   INT32 _ixmKey::_compareHybrid ( const _ixmKey &r, const Ordering &order) const
   {
      BSONObj L = toBson();
      BSONObj R = r.toBson();
      return L.woCompare(R, order, /*considerfieldname*/false);
   }
   // well ordered compare
   INT32 _ixmKey::woCompare ( const _ixmKey &right, const Ordering &o ) const
   {
      const UINT8 *l = _keyData;
      const UINT8 *r = right._keyData;
      UINT32 mask = 1;
      CHAR lval = 0 ;
      CHAR rval = 0 ;
      INT32 x = 0 ;
      INT32 y = 0 ;
      // if any side are native BSON, let's use compareHybrid
      if( (*l|*r) == IsBSON )
         return _compareHybrid(right, o);

      while( TRUE )
      {
         // lvar and rval need to be assigned BEFORE compare, since compare is
         // going to change l and r value
         lval = *l;
         rval = *r;
         x = compare(l, r); // updates l and r pointers
         if ( x )
            return o.descending(mask)?-x:x ;
         // l and r are the same, let's compare if there's more
         y = (INT32)(lval & cHASMORE) ;
         x = y - ((INT32)(rval & cHASMORE));
         // return if one side got more and the other side doesn't
         if ( x )
            return x ;
         // if getMore are the same for both side, let's see if lside got more,
         // if not let's break
         if ( !y )
            break ;
         mask <<= 1;
     }

     return 0;
   }
   // well ordered equal
   BOOLEAN _ixmKey::woEqual ( const _ixmKey &right ) const
   {
      const UINT8 *l = _keyData;
      const UINT8 *r = right._keyData;
      if( (*l|*r) == IsBSON )
      {
         return toBson().equal(right.toBson()) ;
      }
      while( TRUE )
      {
         CHAR lval = *l ;
         CHAR rval = *r ;
         // they can't be equal if the number of elements are not the same
         // or type are not the same
         if( ( lval & ( cCANONTYPEMASK|cHASMORE ) ) !=
             ( rval & ( cCANONTYPEMASK|cHASMORE )))
            return FALSE ;
         l++; r++;
         // now the type are the same, let's see the value
         switch( lval & cCANONTYPEMASK )
         {
         // compare first 4 bytes, then use cdate to compare next 8 bytes
         case coid:
            if( *((UINT32*) l) != *((UINT32*) r) )
               return FALSE ;
            l += sizeof(UINT32); r += sizeof(UINT32);
         case cdate:
            if( *((UINT64*) l) != *((UINT64*) r) )
               return FALSE ;
            l += sizeof(UINT64); r += sizeof(UINT64);
            break;
         case cdouble:
            if( (reinterpret_cast< const PackedDouble* > (l))->d !=
                (reinterpret_cast< const PackedDouble* > (r))->d )
               return FALSE ;
            l += sizeof(FLOAT64); r += sizeof(FLOAT64);
            break;
         case cstring:
         {
            if( *l != *r )
               return FALSE ; // not same length
            UINT32 sz = ((UINT32) *l) + 1 ;
            if( ossMemcmp(l, r, sz) )
               return FALSE ;
            l += sz; r += sz;
            break ;
         }
         case cbindata:
         {
            if( *l != *r )
               return FALSE ; // len or subtype mismatch
            INT32 len = binDataCodeToLength(*l) + 1 ;
            if( memcmp(l, r, len) )
               return FALSE ;
            l += len; r += len;
            break;
         }
         // for undef, min/max keys, null, false/true, they don't have real
         // value, so as long as the types are the same, they are considered
         // as equal
         case cminkey:
         case cundefined:
         case cnull:
         case cfalse:
         case ctrue:
         case cmaxkey:
            break;
         default:
            pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                    "Invalid type compare" ) ;
            throw pdGeneralException ( "Invalid type compare" ) ;
         }
         if( (lval&cHASMORE) == 0 )
            break ;
     }
     return TRUE ;
   }
   static UINT32 sizes[] =
   {
      0 ,
      1 , //cminkey=1,
      1 , //cnull=2,
      1 , //cundefined=3,
      9 , //cdouble=4,
      0 ,
      0 , //cstring=6,
      0 ,
      13 , //coid=8,
      0 ,
      1 , //cfalse=10,
      1 , //ctrue=11,
      9 , //cdate=12,
      0 ,
      1 , //cmaxkey=14,
      0
   };

   OSS_INLINE static UINT32 sizeOfElement(const UINT8 *p)
   {
      UINT32 type = *p & cCANONTYPEMASK;
      UINT32 sz = sizes[type];
      if( sz == 0 )
      {
         // string is variable length
         // 1 byte type + 1 byte length + data
         if( cstring == type )
         {
            sz = ((UINT32) p[1]) + 2 ;
         }
         else if ( cbindata == type )
         {
            sz = binDataCodeToLength(p[1]) + 2 ;
         }
      }
      return sz;
   }

   // get the size of key
   INT32 _ixmKey::dataSize() const
   {
      const UINT8 *p = _keyData;
      if( !isCompactFormat() )
      {
         // bson length + 1 byte type
         return _bson().objsize() + 1 ;
      }
      BOOLEAN more ;
      do
      {
         more = ( *p & cHASMORE )!=0 ;
         p += sizeOfElement(p) ;
      } while ( more ) ;
      return p - _keyData ;
   }
}
