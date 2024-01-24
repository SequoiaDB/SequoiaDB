/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = mthCommon.cpp

   Descriptive Name = Method Common

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains common functions for mth

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/12/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossTypes.h"
#include "mthCommon.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "../util/fromjson.hpp"
#include "utilMath.hpp"
#include "keystring/utilKeyStringBuilder.hpp"

using namespace bson ;

namespace engine
{
   #define MTH_OPERATOR_STR_AND  "$and"
   #define MTH_OPERATOR_STR_OR   "$or"
   #define MTH_OPERATOR_STR_NOT  "$not"

   struct mthCastStr2Type
   {
      CHAR *castStr ;
      BSONType castType ;
   } ;

   static mthCastStr2Type g_cast_str_to_type_array[] =
   {
      //castStr,                      castType
      { "minkey",                     MinKey },
      { "double",                     NumberDouble },
      { "string",                     String },
      { "object",                     Object },
      { "array",                      Array },
      { "bindata",                    BinData },
      { "oid",                        jstOID },
      { "bool",                       Bool },
      { "date",                       Date },
      { "null",                       jstNULL },
      { "int32",                      NumberInt },
      { "timestamp",                  Timestamp },
      { "int64",                      NumberLong },
      { "regex",                      RegEx },
      { "decimal",                    NumberDecimal },
      { "maxkey",                     MaxKey },
   } ;

   static INT32 _mthAbsBasic( const CHAR *name, const BSONElement &in,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthCeilingBasic( const CHAR *name, const BSONElement &in,
                                  BSONObjBuilder &outBuilder ) ;
   static INT32 _mthFloorBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthModBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &modm,
                              BSONObjBuilder &outBuilder ) ;
   static INT32 _mthCastBasic( const CHAR *name, const BSONElement &in,
                               BSONType targetType, BSONObjBuilder &outBuilder ) ;
   static INT32 _mthSubStrBasic( const CHAR *name, const BSONElement &in,
                                 INT32 begin, INT32 limit,
                                 BSONObjBuilder &outBuilder ) ;
   static INT32 _mthStrLenBasic( const CHAR *name, const BSONElement &in,
                                 BSONObjBuilder &outBuilder ) ;

   static BOOLEAN _mthIsUTF8StartByte( CHAR charByte ) ;
   static INT32   _mthLengthInUTF8CodePoints( const CHAR* str ) ;
   static INT32   _mthStrlenCP( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;

   static INT32 _mthLowerBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthUpperBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthTrimBasic( const CHAR *name, const BSONElement &in, INT8 lr,
                               BSONObjBuilder &outBuilder ) ;
   static INT32 _mthAddBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &addend,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthSubBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &subtrahead,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthMultiplyBasic( const CHAR *name, const BSONElement &in,
                                   const BSONElement &multiplier,
                                   BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthDivideBasic( const CHAR *name, const BSONElement &in,
                                 const BSONElement &divisor,
                                 BSONObjBuilder &outBuilder, INT32 &flag ) ;

   static INT32 _mthCast( const CHAR *fieldName, const bson::BSONElement &e,
                          BSONType type, BSONObjBuilder &builder ) ;

   static void _getSubStr( const CHAR *src, INT32 srcLen, INT32 begin,
                           INT32 limit, const CHAR *&subStr,
                           INT32 &subStrLen ) ;

   static INT32 _lower( const CHAR *str, UINT32 len, _utilString<> &us ) ;
   static INT32 _upper( const CHAR *str, UINT32 len, _utilString<> &us ) ;

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   static void _ltrim( const CHAR *str, const CHAR *&trimed ) ;
   static INT32 _rtrim( const CHAR *str, INT32 size, _utilString<> &us ) ;
   static INT32 _mthTrim( const CHAR *str, INT32 size, INT8 lr,
                          _utilString<> &us ) ;

   INT32 _mthCast( const CHAR *fieldName, const bson::BSONElement &e,
                   BSONType type, BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( e.type() != type, "should not be same" ) ;
      switch ( type )
      {
      case MinKey :
         builder.appendMinKey( fieldName ) ;
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  (FLOAT64)( ( INT64 )( e.date().millis ) ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l += ( INT64 )( e.timestampInc() / 1000 ) ;
            builder.appendNumber( fieldName, (FLOAT64)l ) ;
         }
         else if ( Bool == e.type() )
         {
            FLOAT64 f = e.Bool() ? 1.0 : 0.0 ;
            builder.appendNumber( fieldName, f ) ;
         }
         else if ( String != e.type() )
         {
            FLOAT64 f = e.numberDouble() ;
            if ( isInf( f ) )
            {
               f = 0.0 ;
            }
            builder.appendNumber( fieldName, f ) ;
         }
         else
         {
            try
            {
               FLOAT64 f = 0.0 ;
               f = boost::lexical_cast<FLOAT64>( e.valuestr () ) ;
               builder.appendNumber( fieldName, f ) ;
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0.0 ) ;
            }
         }
         break ;
      }
      case String :
      {
         if ( NumberInt == e.type() )
         {
            _utilString<UTIL_STRING_INT_LEN+1> us ;
            rc = us.appendINT32( e.numberInt() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int32:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberLong == e.type() )
         {
            _utilString<UTIL_STRING_INT64_LEN+1> us ;
            rc = us.appendINT64( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            _utilString<UTIL_STRING_DOUBLE_LEN+1> us ;
            rc = us.appendDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append float64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            _utilString<> us ;
            bsonDecimal decimal ;
            string value ;

            decimal = e.numberDecimal() ;
            rc = decimal.toStringChecked( value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed toStringChecked,rc=%d",rc ) ;
               goto error ;
            }

            rc      = us.append( value.c_str(), value.length() );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append decimal=%s,rc=%d",
                       value.c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( Date == e.type() )
         {
            CHAR buffer[64] = { 0 };
            time_t timer = (time_t)( ( INT64 )( e.date() ) / 1000 ) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( Timestamp == e.type() )
         {
            Date_t date = e.timestampTime () ;
            unsigned int inc = e.timestampInc () ;
            char buffer[128] = { 0 };
            time_t timer = (time_t)((( INT64 )(date.millis))/1000) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday,
                      psr.tm_hour,
                      psr.tm_min,
                      psr.tm_sec,
                      inc ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( jstOID == e.type() )
         {
            builder.append( fieldName, e.OID().str() ) ;
         }
         else if ( Object == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( FALSE, TRUE ) ) ;
         }
         else if ( Array == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( TRUE, TRUE ) ) ;
         }
         else if ( Bool == e.type() )
         {
            builder.append( fieldName,
                            e.booleanSafe() ?
                            "true" : "false" ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Object :
      {
         if ( String == e.type() )
         {
            BSONObj obj ;
            INT32 r = fromjson( e.valuestr(), obj ) ;
            if ( SDB_OK == r )
            {
               builder.append( fieldName, obj ) ;
            }
            else
            {
               builder.appendNull( fieldName ) ;
            }
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Array :
      case BinData :
      case Undefined :
         builder.appendNull( fieldName ) ;
         break ;
      case jstOID :
      {
         if ( String == e.type() &&
              25 == e.valuestrsize() )
         {
            bson::OID o( e.valuestr() ) ;
            builder.appendOID( fieldName, &o ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Bool :
         builder.appendBool( fieldName, e.trueValue() ) ;
         break ;
      case Date :
      {
         UINT64 tm = 0 ;
         if ( e.isNumber() )
         {
            if ( NumberInt == e.type() )
            {
               Date_t d( e.numberInt() * 1000LL ) ;
               builder.appendDate( fieldName, d ) ;
            }
            else
            {
               BOOLEAN hasAppend = FALSE ;
               if ( NumberDecimal == e.type() )
               {
                  bsonDecimal original = e.Decimal() ;
                  bsonDecimal l_min ;
                  bsonDecimal l_max ;
                  rc = l_min.fromLong( OSS_SINT64_MIN ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  rc = l_max.fromLong( OSS_SINT64_MAX ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  if ( original.compare( l_min ) < 0 ||
                       original.compare( l_max ) > 0 )
                  {
                     builder.appendNull( fieldName ) ;
                     hasAppend = TRUE ;
                  }
               }
               if ( FALSE == hasAppend )
               {
                  Date_t d( e.numberLong() ) ;
                  builder.appendDate( fieldName, d ) ;
               }
            }
         }
         else if ( String == e.type() &&
                   SDB_OK == utilStr2Date( e.valuestr(), tm ))
         {
            builder.appendDate( fieldName, Date_t( tm ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            builder.appendDate( fieldName, e.timestampTime() ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case jstNULL :
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         builder.appendNull( fieldName ) ;
         break ;
      case NumberInt :
      {
         if ( Date == e.type() )
         {
            INT32 sec = 0 ;
            INT64 l   = ( ( INT64 )( e.date().millis ) ) / 1000 ;
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               sec = 0 ;
            }
            else
            {
               sec = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, sec ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT32 sec = 0 ;
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l      += ( INT64 )( ((INT32)(e.timestampInc())) / 1000 ) ;
            l       = l / 1000; // seconds
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               sec = 0 ;
            }
            else
            {
               sec = (INT32)l ;
            }
            builder.appendNumber( fieldName, sec ) ;
         }
         else if ( Bool == e.type() )
         {
            INT32 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberLong == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = e.numberLong() ;
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = 0 ;
            rc = e.numberDecimal().toLong( &l) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
               goto error ;
            }
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT32 i = 0 ;
            double d = e.Double() ;
            if ( d > OSS_SINT32_MAX_D || d < OSS_SINT32_MIN_D )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )d ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberInt() ) ;
         }
         else
         {
            try
            {
               INT32 i = 0 ;
               double v = 0 ;
               v = boost::lexical_cast<double>( e.valuestr () ) ;
               if ( v > OSS_SINT32_MAX_D || v < OSS_SINT32_MIN_D )
               {
                  i = 0 ;
               }
               else
               {
                  i = ( INT32 )v ;
               }
               builder.appendNumber( fieldName, i ) ;
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case Timestamp :
      {
         time_t tm = 0 ;;
         UINT64 usec = 0 ;
         if ( e.isNumber() )
         {
            if ( NumberInt == e.type() )
            {
               INT32 sec = ( INT32 )( e.numberInt() ) ; // take it as seconds
               OpTime t( (unsigned) (sec), 0 ) ;
               builder.appendTimestamp( fieldName, t.asDate() ) ;
            }
            else
            {
               BOOLEAN hasAppend = FALSE ;
               if ( NumberDecimal == e.type() )
               {
                  bsonDecimal original = e.Decimal() ;
                  bsonDecimal l_min ;
                  bsonDecimal l_max ;
                  rc = l_min.fromLong( OSS_SINT64_MIN ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  rc = l_max.fromLong( OSS_SINT64_MAX ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  if ( original.compare( l_min ) < 0 ||
                       original.compare( l_max ) > 0 )
                  {
                     builder.appendNull( fieldName ) ;
                     hasAppend = TRUE ;
                  }
               }
               if ( FALSE == hasAppend )
               {
                  INT64 varLong = ( INT64 )( e.numberLong() ) ;
                  INT64 sec     = varLong / 1000 ;
                  INT64 us      = ( varLong % 1000 ) * 1000 ; // microseconds
                  if ( us < 0 )
                  {
                     // move 1s from sec to us
                     sec--;
                     us += 1000000;
                  }
                  if ( sec > OSS_SINT32_MAX_LL || sec < OSS_SINT32_MIN_LL )
                  {
                     builder.appendNull( fieldName ) ;
                  }
                  else
                  {
                     OpTime t( (unsigned) (sec), (unsigned) (us) ) ;
                     builder.appendTimestamp( fieldName, t.asDate() ) ;
                  }
               }
            }
         }
         else if ( String == e.type() &&
                   SDB_OK == engine::utilStr2TimeT( e.valuestr(),
                                                    tm,
                                                    &usec ))
         {
            OpTime t( (unsigned) (tm) , usec );
            builder.appendTimestamp( fieldName, t.asDate() ) ;
         }
         else if ( Date == e.type() )
         {
            // when date is large than the max value of timestamp,
            // return null
            INT64 sec = ( ( INT64 )( e.date().millis ) ) / 1000 ;
            if ( sec > OSS_SINT32_MAX_LL || sec < OSS_SINT32_MIN_LL )
            {
               builder.appendNull( fieldName ) ;
            }
            else
            {
               OpTime t( (unsigned) (sec), 0 ) ;
               builder.appendTimestamp( fieldName, t.asDate() ) ;
            }
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case NumberLong :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  ( INT64 )( e.date().millis ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l += ( INT64 )( e.timestampInc() / 1000 ) ;
            builder.appendNumber( fieldName, l ) ;
         }
         else if ( Bool == e.type() )
         {
            INT64 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT64 l = 0 ;
            double d = e.Double() ;
            if ( d >= 0 && d < (OSS_SINT64_MAX_D + 1) )
            {
               l = (INT64)d ;
            }
            else if ( d < 0 && d >= OSS_SINT64_MIN_D )
            {
               l = (INT64)d ;
            }
            builder.appendNumber( fieldName, l ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberLong() ) ;
         }
         else
         {
            try
            {
               //if the STRING has "." "e" or "E" use double type
               if ( ossStrchr ( e.valuestr (), '.' ) != NULL ||
                    ossStrchr ( e.valuestr (), 'E' ) != NULL ||
                    ossStrchr ( e.valuestr (), 'e' ) != NULL )
               {
                  double d = 0  ;
                  INT64 l = 0 ;
                  d = boost::lexical_cast<double>( e.valuestr () ) ;
                  if ( d >= 0 && d < (OSS_SINT64_MAX_D + 1) )
                  {
                     l = (INT64)d ;
                  }
                  else if ( d < 0 && d >= OSS_SINT64_MIN_D )
                  {
                     l = (INT64)d ;
                  }
                  builder.appendNumber( fieldName, l ) ;
               }
               else
               {
                  INT64 l = 0 ;
                  l = boost::lexical_cast<INT64>( e.valuestr () ) ;
                  builder.appendNumber( fieldName, l ) ;
               }
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case NumberDecimal :
      {
         if ( Date == e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromLong( ( INT64 )( e.date().millis ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( Timestamp == e.type() )
         {
            bsonDecimal decimal ;
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l      += ( INT64 )( ( (INT32)(e.timestampInc()) ) / 1000 ) ;
            rc = decimal.fromLong( l ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( Bool == e.type() )
         {
            bsonDecimal decimal ;
            INT64 v = e.Bool() ? 1 : 0 ;

            rc = decimal.fromLong( ( INT64 )v ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( NumberLong == e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromLong( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( String != e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else
         {
            bsonDecimal decimal ;
            rc = decimal.fromString( e.String().c_str() ) ;
            if ( SDB_OK != rc && SDB_INVALIDARG != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }

            rc = SDB_OK ;
            // SDB_OK or SDB_INVALIDARG(invalid string return the default value)
            builder.append( fieldName, decimal ) ;
         }
         break ;
      }
      case MaxKey :
         builder.appendMaxKey( fieldName ) ;
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid cast type:%d", type ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _getSubStr( const CHAR *src, INT32 srcLen, INT32 begin,
                    INT32 limit, const CHAR *&subStr, INT32 &subStrLen )
   {
      const CHAR *cpBegin = NULL ;

      if ( srcLen < 0 )
      {
         goto error ;
      }

      if ( 0 <= begin )
      {
         if ( srcLen <= begin )
         {
            goto error ;
         }
         cpBegin = src + begin ;
         subStrLen = srcLen - begin ;
      }
      else
      {
         INT32 beginPos = srcLen + begin ;
         if ( beginPos < 0 )
         {
            goto error ;
         }
         cpBegin = src + beginPos ;
         subStrLen = srcLen - beginPos ;
      }

      if ( 0 <= limit && limit < subStrLen )
      {
         subStrLen = limit ;
      }

      subStr = cpBegin ;
   done:
      return ;
   error:
      subStr    = NULL ;
      subStrLen = -1 ;
      goto done ;
   }

   INT32 _lower( const CHAR *str, UINT32 len, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'A' <= *p &&
              *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _upper( const CHAR *str, UINT32 len, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'a' <= *p &&
              *p <= 'z' )
         {
            rc = us.append( *p - 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _ltrim( const CHAR *str, const CHAR *&trimed )
   {
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( ' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p )
         {
            break ;
         }

         ++p ;
      }

      trimed = p ;
      return ;
   }

   INT32 _rtrim( const CHAR *str, INT32 size, _utilString<> &us )
   {
      INT32 rc  = SDB_OK ;
      INT32 pos = size - 1 ;

      while ( 0 <= pos )
      {
         const CHAR *p = str + pos ;
         if ( ' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p )
         {
            break ;
         }

         --pos ;
      }

      if ( 0 <= pos )
      {
         rc = us.append( str, pos + 1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append string:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthTrim( const CHAR *str, INT32 size, INT8 lr, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      const CHAR *p = str ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         const CHAR *newP = NULL ;
         _ltrim( p, newP ) ;
         p = newP ;
      }

      if ( 0 <= lr )
      {
         rc = _rtrim( p, size - ( p - str ), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         /// necessary to avoid one more copy when
         /// str is like "  abc" ?
         rc = us.append( p, size - ( p - str ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // the function try to append newStr to ppStr.
   // if the buffer is not large enough the function is responsible to allocate
   // a larger one. If failed to allocate larger buffer, this function must
   // maintain the validity of original pointer
   INT32 mthAppendString ( CHAR **ppStr, INT32 &bufLen,
                           INT32 strLen, const CHAR *newStr,
                           INT32 newStrLen, INT32 *pMergedLen )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( ppStr && newStr, "str or newStr can't be NULL" ) ;
      // if user doesn't know the string length, pass 0
      if ( !*ppStr )
      {
         strLen = 0 ;
      }
      else if ( strLen <= 0 )
      {
         strLen = ossStrlen ( *ppStr ) ;
      }
      // if user doesn't know the new string len, pass 0
      if ( newStrLen <= 0 )
      {
         newStrLen = ossStrlen ( newStr ) ;
      }
      // make sure the string len and new string len is less than buffer
      if ( strLen + newStrLen >= bufLen )
      {
         // we need to allocate more memory if exceed buffer
         CHAR *pOldStr = *ppStr ;
         INT32 newSize = ossRoundUpToMultipleX ( strLen + newStrLen,
                                                 SDB_PAGE_SIZE ) ;
         if ( newSize < 0 )
         {
            PD_LOG ( PDERROR, "new buffer overflow, size: %d", newSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
         if ( !*ppStr )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
            rc = SDB_OOM ;
            *ppStr = pOldStr ;
            goto error ;
         }
         bufLen = newSize ;
      }
      // now new buffer is allocated or we already have enough memory, let's do
      // copy
      if ( *ppStr && newStr )
      {
         ossMemcpy ( &(*ppStr)[strLen], newStr, newStrLen ) ;
         (*ppStr)[strLen+newStrLen] = '\0' ;

         if ( pMergedLen )
         {
            *pMergedLen = strLen + newStrLen ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN mthIsZero( const BSONElement &ele )
   {
      if ( ele.type() == NumberDecimal ) {
         bsonDecimal decimal = ele.numberDecimal() ;
         if ( decimal.isZero() ) {
            return TRUE ;
         }
      }
      else if ( ele.type() == NumberDouble ) {
         double d = ele.numberDouble() ;
         if ( d < OSS_EPSILON && d > -OSS_EPSILON ) {
            return TRUE ;
         }
      }
      else {
         long l = ele.numberLong() ;
         if ( 0 == l ) {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDOUBLEBUFFERSIZE, "mthDoubleBufferSize" )
   INT32 mthDoubleBufferSize ( CHAR **ppStr, INT32 &bufLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHDOUBLEBUFFERSIZE ) ;
      SDB_ASSERT ( ppStr, "ppStr can't be NULL" ) ;
      CHAR *pOldStr = *ppStr ;
      INT32 newSize = ossRoundUpToMultipleX ( 2*bufLen,
                                              SDB_PAGE_SIZE ) ;
      if ( newSize < 0 )
      {
         PD_LOG ( PDERROR, "new buffer overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( 0 == newSize )
      {
         newSize = SDB_PAGE_SIZE ;
      }
      *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
      if ( !*ppStr )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
         rc = SDB_OOM ;
         *ppStr = pOldStr ;
         goto error ;
      }
      bufLen = newSize ;

   done :
      PD_TRACE_EXITRC ( SDB__MTHDOUBLEBUFFERSIZE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 mthCheckFieldName( const CHAR *pField, INT32 &dollarNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      dollarNum = 0 ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;
         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }
            ++dollarNum ;
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN mthCheckUnknowDollar( const CHAR *pField,
                                 std::vector<INT64> *dollarList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      BOOLEAN hasUnknowDollar = FALSE ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;

         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }

            if ( dollarList )
            {
               std::vector<INT64>::iterator it = dollarList->begin() ;
               for ( ; it != dollarList->end() ; ++it )
               {
                  if ( number == (((*it)>>32)&0xFFFFFFFF) )
                  {
                     break ;
                  }
               }
               if ( it == dollarList->end() )
               {
                  goto error ;
               }
            }
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return hasUnknowDollar ? FALSE : TRUE ;
   error:
      hasUnknowDollar = TRUE ;
      goto done ;
   }

   INT32 mthConvertSubElemToNumeric( const CHAR *desc,
                                     INT32 &n )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc && '$' == *desc, "must be a $" ) ;
      const CHAR *p = desc ;
      const UINT32 maxLen = 10 ;
      CHAR number[maxLen + 1] = { 0 } ;
      UINT32 numberLen = 0 ;

      if ( *p && '$' == *p && p[1] && '[' == p[1] )
      {
         p += 2 ;
         while ( *p )
         {
            if ( '0' <= *p && *p <= '9' )
            {
               if ( numberLen == maxLen )
               {
                  PD_LOG( PDERROR, "number is too long" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               number[numberLen++] = *p++ ;
            }
            else if ( ']' == *p )
            {
               break ;
            }
            else
            {
               PD_LOG( PDDEBUG, "argument should be a numeric" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         if ( 0 == numberLen || ']' != *p )
         {
            PD_LOG( PDDEBUG, "invalid action in selector:%s", desc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         number[numberLen] = '\0' ;
         n = ossAtoi( number ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "invalid action:%s", desc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
     return rc ;
   error:
     goto done ;
   }

   BOOLEAN mthIsModValid( const BSONElement &modmEle )
   {
      if ( modmEle.type() == NumberDecimal )
      {
         bsonDecimal modmDecimal = modmEle.numberDecimal() ;
         if ( modmDecimal.isZero() )
         {
            return FALSE ;
         }
      }
      else if ( modmEle.type() == NumberDouble )
      {
         FLOAT64 f = modmEle.numberDouble() ;
         if ( fabs( f ) <= OSS_EPSILON )
         {
            return FALSE ;
         }
      }
      else
      {
         INT64 modm = modmEle.numberLong() ;
         if ( 0 == modm )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _mthAbsBasic( const CHAR *name, const BSONElement &in,
                       BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, fabs( in.Double() ) ) ;
      }
      else if ( NumberInt == in.type() )
      {
         INT32 v = in.numberInt() ;
         /// - 2 ^ 31
         if ( OSS_SINT32_MIN != v )
         {
            outBuilder.append( name, 0 <= v ? v : -v ) ;
         }
         else
         {
            outBuilder.append( name, -((INT64)v) ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ;
         }
      }
      else if ( NumberLong == in.type() )
      {
         INT64 v = in.numberLong() ;
         /// return -9223372036854775808 when v is -9223372036854775808
         if ( OSS_SINT64_MIN != v)
         {
            outBuilder.append( name, 0 <= v ? ( INT64 )v : ( INT64 )( -v ) ) ;
         }
         else
         {
            bsonDecimal decResult ;
            rc = decResult.fromString( "9223372036854775808" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromString(\"9223372036854775808\"),rc=%d",
                       rc ) ;
               goto error ;
            }
            outBuilder.append( name, decResult ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ;
         }

      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         decimal = in.numberDecimal() ;
         rc = decimal.abs() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         outBuilder.append( name, decimal ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         /// do nothing.
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthAbs( const CHAR *name, const BSONElement &in,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthAbsBasic( ele.fieldName(), ele, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Abs:rc=%d", rc ) ;


            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthAbsBasic( name, in, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Abs:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCeilingBasic( const CHAR *name, const BSONElement &in,
                           BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( NumberLong == in.type() )
      {
         outBuilder.append( name, ( INT64 )( in.numberLong() ) ) ;
      }
      else if ( NumberInt == in.type() )
      {
         outBuilder.append( name, ( INT32 )( in.numberInt() ) ) ;
      }
      else if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, ( FLOAT64 )ceil( in.numberDouble() ) ) ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         decimal = in.numberDecimal() ;

         rc = decimal.ceil( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         outBuilder.append( name, result ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthCeiling( const CHAR *name, const BSONElement &in,
                     BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthCeilingBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Ceiling:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthCeilingBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Ceiling:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthFloorBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( NumberInt == in.type() )
      {
         outBuilder.append( name, ( INT32 )( in.numberInt() ) ) ;
      }
      else if ( NumberLong == in.type() )
      {
         outBuilder.append( name, ( INT64 )( in.numberLong() ) ) ;
      }
      else if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, ( FLOAT64 )floor( in.numberDouble() ) ) ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;

         decimal = in.numberDecimal() ;
         rc = decimal.floor( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to floor decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthFloor( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthFloorBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Floor:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthFloorBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Floor:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &modm, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         /// do nothing.
      }
      else if ( !in.isNumber() || !modm.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == modm.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = modm.numberDecimal() ;
         rc = decimal.mod( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod decimal:%s mod %s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( FALSE == mthIsModValid( modm ) )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDouble == in.type() &&
                NumberDouble == modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(), modm.numberDouble() ) ;
         outBuilder.append( name, v ) ;
      }
      else if ( NumberDouble != in.type () &&
                NumberDouble == modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberLong(), modm.numberDouble() ) ;
         outBuilder.append( name, v ) ;
      }
      else if ( NumberDouble == in.type () &&
                NumberDouble != modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(), modm.numberLong() ) ;
         outBuilder.append( name, v ) ;
      }
      else
      {
         INT64 v = in.numberLong() % modm.numberLong() ;
         outBuilder.appendNumber( name, v ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthMod( const CHAR *name, const BSONElement &in,
                 const BSONElement &modm, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthModBasic( ele.fieldName(), ele, modm, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Mod:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthModBasic( name, in, modm, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Mod:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastBasic( const CHAR *name, const BSONElement &in,
                        BSONType targetType, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }

      if ( EOO == targetType )
      {
         PD_LOG( PDERROR, "can not cast to eoo" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( in.type() == targetType )
      {
         outBuilder.appendAs( in, name ) ;
      }
      else
      {
         rc = _mthCast( name, in, targetType, outBuilder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", in.toString().c_str(), targetType ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthCast( const CHAR *name, const BSONElement &in,
                  BSONType targetType, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthCastBasic( ele.fieldName(), ele, targetType, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Cast:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthCastBasic( name, in, targetType, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Cast:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubStrBasic( const CHAR *name, const BSONElement &in,
                          INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         const CHAR *outStr = NULL ;
         INT32 outStrLen    = -1 ;
         _getSubStr( in.valuestr(), in.valuestrsize() - 1, begin, limit,
                     outStr, outStrLen ) ;
         if ( NULL == outStr || -1 == outStrLen )
         {
            outBuilder.append( name, "" ) ;
         }
         else
         {
            outBuilder.appendStrWithNoTerminating( name, outStr, outStrLen ) ;
         }
      }

   done:
      return rc ;
   }

   INT32 mthSlice( const CHAR *name, const BSONElement &in,
                   INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( Array == in.type() )
      {
         _mthSliceIterator iter( in.embeddedObject(), begin, limit ) ;

         BSONArrayBuilder sliceBuilder( outBuilder.subarrayStart( name ) ) ;
         while ( iter.more() )
         {
            sliceBuilder.append( iter.next() ) ;
         }

         sliceBuilder.doneFast() ;
      }
      else
      {
         outBuilder.append( in ) ;
      }

   done:
      return rc ;
   }

   INT32 mthSubStr( const CHAR *name, const BSONElement &in,
                    INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthSubStrBasic( ele.fieldName(), ele, begin, limit,
                                  tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to SubStr:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthSubStrBasic( name, in, begin, limit, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to SubStr:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthStrLenBasic( const CHAR *name, const BSONElement &in,
                          BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         outBuilder.append( name, in.valuestrsize() - 1 ) ;
      }

   done:
      return rc ;
   }

   // UTF-8 encoding rules:
   // single byte: the highest bit is 0
   // multibyte: start from the highest bit, N consecutive bits are all 1. The
   // rest of the bytes all start with 10. N represents the number of encoded
   // bytes
   BOOLEAN _mthIsUTF8StartByte( CHAR charByte )
   {
      return ( charByte & 0xc0 ) != 0x80 ? TRUE : FALSE ;
   }

   INT32 _mthLengthInUTF8CodePoints( const CHAR* str )
   {
      UINT32 i = 0 ;
      INT32 length = 0 ;

      if ( NULL == str )
      {
         goto done ;
      }

      while ( str[i] != '\0' )
      {
         if ( _mthIsUTF8StartByte( str[i] ) )
         {
            length++ ;
         }
         ++i ;
      }

   done:
      return length ;
   }

   INT32 _mthStrlenCP( const CHAR *name, const BSONElement &in,
                       BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         outBuilder.append( name,
                            _mthLengthInUTF8CodePoints ( in.valuestrsafe() ) ) ;
      }

   done:
      return rc ;
   }

   INT32 mthStrLenBytes( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthStrLenBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to StrLen:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthStrLenBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to StrLen:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthStrLen( const CHAR *name, const BSONElement &in,
                    BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      rc = mthStrLenBytes( name, in, outBuilder ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to strlen:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthStrLenCP( const CHAR *name, const BSONElement &in,
                      BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthStrlenCP( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to strlenCP:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthStrlenCP( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to strlenCP:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthLowerBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _lower( in.valuestr(), in.valuestrsize(), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthLower( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthLowerBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Lower:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthLowerBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Lower:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthUpperBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _upper( in.valuestr(), in.valuestrsize(), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthUpper( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthUpperBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Upper:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthUpperBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Upper:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN mthIsTrimed( const CHAR *str, INT32 size, INT8 lr )
   {
      BOOLEAN rc = TRUE ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         if ( ' ' == *str || '\t' == *str || '\n' == *str || '\r' == *str )
         {
            rc = FALSE ;
            goto done ;
         }
      }

      if ( 0 <= lr )
      {
         if ( ' ' == *( str + strLen - 1 ) || '\t' == *( str + strLen - 1 ) ||
              '\n' == *( str + strLen - 1 ) || '\r' == *( str + strLen - 1 ) )
         {
            rc = FALSE ;
            goto done ;
         }
      }

   done:
      return rc ;
   }

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   INT32 _mthTrimBasic( const CHAR *name, const BSONElement &in, INT8 lr,
                        BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( mthIsTrimed( in.valuestr(), in.valuestrsize() - 1, lr ) )
      {
         outBuilder.appendAs( in, name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _mthTrim( in.valuestr(), in.valuestrsize() - 1, lr, us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   INT32 mthTrim( const CHAR *name, const BSONElement &in, INT8 lr,
                  BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthTrimBasic( ele.fieldName(), ele, lr, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to add trim:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthTrimBasic( name, in, lr, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to  trim:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthAddBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &addend,
                       BSONObjBuilder &outBuilder,
                       INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == addend.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimalE   = in.numberDecimal() ;
         decimalArg = addend.numberDecimal() ;
         rc = decimalE.add( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                    decimalE.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == addend.type() )
      {
         FLOAT64 f = addend.numberDouble() + in.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == addend.type() )
      {
         INT64 arg1 = addend.numberLong() ;
         INT64 arg2 = in.numberLong() ;
         INT64 i = arg1 + arg2 ;
         if ( utilAddIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = addend.numberDecimal() ;
            rc = decimalE.add( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            // keep int if possible
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = addend.numberInt() ;
         INT32 arg2 = in.numberInt() ;
         INT32 i32 = arg1 + arg2 ;
         INT64 i64 = (INT64)arg1 + (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthAdd( const CHAR *name, const BSONElement &in,
                 const BSONElement &addend,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthAddBasic( ele.fieldName(), ele, addend, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to add:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthAddBasic( name, in, addend, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to add:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &subtrahead,
                       BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == subtrahead.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimalE   = in.numberDecimal() ;
         decimalArg = subtrahead.numberDecimal() ;
         rc = decimalE.sub( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub decimal:%s-%s,rc=%d",
                    decimalE.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == subtrahead.type() )
      {
         FLOAT64 f = in.numberDouble() - subtrahead.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == subtrahead.type() )
      {
         INT64 arg1 = in.numberLong() ;
         INT64 arg2 = subtrahead.numberLong() ;
         INT64 i = arg1 - arg2 ;
         if ( utilSubIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = subtrahead.numberDecimal() ;
            rc = decimalE.sub( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sub decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = in.numberInt() ;
         INT32 arg2 = subtrahead.numberInt() ;
         INT32 i32 = arg1 - arg2 ;
         INT64 i64 = (INT64)arg1 - (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthSub( const CHAR *name, const BSONElement &in,
                 const BSONElement &subtrahead,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthSubBasic( ele.fieldName(), ele, subtrahead, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to subtract:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthSubBasic( name, in, subtrahead, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to subtract:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMultiplyBasic( const CHAR *name, const BSONElement &in,
                            const BSONElement &multiplier,
                            BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == multiplier.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = multiplier.numberDecimal() ;
         rc = decimal.mul( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mul decimal:%s*%s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == multiplier.type() )
      {
         FLOAT64 f = multiplier.numberDouble() * in.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == multiplier.type() )
      {
         INT64 arg1 = in.numberLong() ;
         INT64 arg2 = multiplier.numberLong() ;
         INT64 i = arg1 * arg2 ;
         if ( utilMulIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = multiplier.numberDecimal() ;
            rc = decimalE.mul( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sub decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            // keep int if possible
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = in.numberInt() ;
         INT32 arg2 = multiplier.numberInt() ;
         INT32 i32 = arg1 * arg2 ;
         INT64 i64 = (INT64)arg1 * (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthMultiply( const CHAR *name, const BSONElement &in,
                      const BSONElement &multiplier,
                      BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthMultiplyBasic( ele.fieldName(), ele, multiplier,
                                    tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Multiply:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthMultiplyBasic( name, in, multiplier, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Multiply:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthDivideBasic( const CHAR *name, const BSONElement &in,
                          const BSONElement &divisor,
                          BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == divisor.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = divisor.numberDecimal() ;
         rc = decimal.div( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to div decimal:%s/%s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == divisor.type() )
      {
         FLOAT64 r = divisor.numberDouble() ;
         if ( fabs(r) < OSS_EPSILON )
         {
            PD_LOG( PDERROR, "invalid argument:%f", r ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         outBuilder.appendNumber( name, in.numberDouble() / r ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == divisor.type() )
      {
         INT64 divide = in.numberLong() ;
         INT64 r = divisor.numberLong() ;
         INT64 result ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ; /// should not happen. so use sdb_sys.
            goto error ;
         }
         if ( !utilDivIsOverflow( divide, r ) )
         {
            result = divide / r ;
            if ( NumberInt == in.type() && utilCanConvertToINT32( result ) )
            {
               // keep int if possible
               outBuilder.append( name, (INT32)result ) ;
            }
            else
            {
               outBuilder.append( name, result ) ;
            }
         }
         else
         {
            //overflow
            bsonDecimal decResult ;
            rc = decResult.fromString( "9223372036854775808" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to div decimal:%lld/%lld,rc=%d",
                       divide, r, rc ) ;
               goto error ;
            }
            outBuilder.append( name, decResult ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }

      }
      else
      {
         INT32 divide = in.numberInt() ;
         INT32 r = divisor.numberInt() ;
         INT32 result ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ; /// should not happen. so use sdb_sys.
            goto error ;
         }
         if ( -1 == r )
         {
            if ( divide != (INT32)OSS_SINT32_MIN )
            {
               result = -divide ;
               outBuilder.append( name, result ) ;
            }
            else
            {
               INT64 result64 = 2147483648 ;
               outBuilder.append( name, result64 ) ;
               flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
            }
         }
         else
         {
            result = divide / r ;
            outBuilder.append( name, result ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthDivide( const CHAR *name, const BSONElement &in,
                    const BSONElement &divisor,
                    BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthDivideBasic( ele.fieldName(), ele, divisor, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Divide:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthDivideBasic( name, in, divisor, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Divide:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthType( const CHAR *name, INT32 outType, const BSONElement &in,
                  BSONObjBuilder &outBuilder )
   {
      if ( !in.eoo() )
      {
         BSONType type = in.type() ;
         if ( 1 == outType )
         {
            outBuilder.append( name, type ) ;
         }
         else
         {
            string typeName = "" ;
            mthGetCastTranslator()->getCastStr( type, typeName ) ;
            outBuilder.append( name, typeName ) ;
         }
      }

      return SDB_OK ;
   }

   INT32 mthSize( const CHAR *name, const BSONElement &in,
                  BSONObjBuilder &outBuilder )
   {
      if ( in.eoo() )
      {
         goto done ;
      }

      if ( in.type() == Array || in.type() == Object )
      {
         outBuilder.append( name, in.embeddedObject().nFields() ) ;
      }
      else
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return SDB_OK ;
   }

   INT32 mthKeyString( const CHAR *name,
                       INT32 direction,
                       const BSONElement &in,
                       BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      keystring::keyStringBuilder builder ;
      rc = builder.buildForKeyStringFunc( in, direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append bson element, rc: %d", rc ) ;
      {
         utilSlice keySlice( builder.getShallowKeyString().getKeySlice() ) ;
         outBuilder.appendBinData( name,
                                   keySlice.getSize(),
                                   BinDataGeneral,
                                   keySlice.getData() ) ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   _mthCastTranslator::_mthCastTranslator()
   {
      INT32 i   = 0 ;
      INT32 len = 0 ;

      len = sizeof( g_cast_str_to_type_array) / sizeof( mthCastStr2Type ) ;
      for ( i = 0 ; i < len ; i++ )
      {
         mthCastStr2Type *ptype = &g_cast_str_to_type_array[i] ;
         _castTransMap[ ptype->castStr ] = ptype->castType ;
         _castTypeMap[ ptype->castType ] = ptype->castStr ;
      }
   }

   _mthCastTranslator::~_mthCastTranslator()
   {
      _castTransMap.clear() ;
      _castTypeMap.clear() ;
   }

   INT32 _mthCastTranslator::getCastType( const CHAR *typeStr, BSONType &type )
   {
      MTH_CAST_NAME_MAP::iterator iter ;

      INT32 rc = SDB_OK ;
      _utilString<20> us ;
      const CHAR *p = typeStr ;
      while ( '\0' != *p )
      {
         if ( 'A' <= *p && *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "append str failed:str=%s,rc=%d",
                       typeStr, rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = us.append( *p ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "append str failed:str=%s,rc=%d",
                       typeStr, rc ) ;
               goto error ;
            }
         }

         ++p ;
      }

      iter = _castTransMap.find( us.str() ) ;
      if ( iter == _castTransMap.end() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "unknown type:typeStr=%s", typeStr ) ;
         goto error ;
      }

      type = iter->second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastTranslator::getCastStr( BSONType type, string &name )
   {
      MTH_CAST_TYPE_MAP::iterator iter ;
      iter = _castTypeMap.find( type ) ;
      if ( iter == _castTypeMap.end() )
      {
         name = "Unknown Type" ;
      }
      else
      {
         name = iter->second ;
      }

      return SDB_OK ;
   }

   _mthCastTranslator *mthGetCastTranslator()
   {
      static _mthCastTranslator translator ;

      return &translator ;
   }

   _mthSliceIterator::_mthSliceIterator( const bson::BSONObj &obj, INT32 begin,
                                         INT32 limit )
   :_obj( obj ), _where( 0 ), _limit( limit ), _itr( _obj )
   {
      INT32 total = obj.nFields() ;
      _where = begin < 0 ? begin + total : begin ;
      if ( _where < 0 )
      {
         _where = 0 ;
      }

      while ( 0 != _where )
      {
         if ( _itr.more() )
         {
            _itr.next() ;
            --_where ;
         }
         else
         {
            _limit = 0 ;
            break ;
         }
      }
   }

   _mthSliceIterator::~_mthSliceIterator()
   {
   }

   BOOLEAN _mthSliceIterator::more()
   {
      return _limit != 0 && _itr.more() ;
   }

   bson::BSONElement _mthSliceIterator::next()
   {
      if ( more() )
      {
         if ( 0 < _limit )
         {
            --_limit ;
         }

         return _itr.next() ;
      }
      else
      {
         return BSONElement() ;
      }
   }

   BOOLEAN mthIsNumber1( const bson::BSONElement &ele )
   {
      if ( ele.isNumber() )
      {
         if ( ele.numberInt() == 1 )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   //substr[begin, len]/slice[begin, len]  len=-1 means unlimit len
   BOOLEAN mthIsValidLen( INT32 length )
   {
      return TRUE ;
   }

   INT32 mthCheckIfSubFieldIsOp( const BSONElement &ele, BOOLEAN &subFieldIsOp )
   {
      INT32 rc = SDB_OK ;
      subFieldIsOp = FALSE ;

      try
      {
         BSONObjIterator itr( ele.embeddedObject() ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            const CHAR *fieldName = e.fieldName() ;

            if ( MTH_OPERATOR_EYECATCHER != fieldName[0] ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_AND ) ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_OR ) ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_NOT ) )
            {
               continue ;
            }
            else
            {
               subFieldIsOp = TRUE ;
               break ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "Check if the subfield name is an operator name"
                  "exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

