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

   Source File Name = fapMongoDecimal.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2020/06/18  fangjiabin Initial Draft

   Last Changed =

*******************************************************************************/

#include "fapMongoDecimal.hpp"
#if !defined( _ARMLIN64 )
#include "../../thirdparty/intelDecimal/LIBRARY/src/bid_conf.h"
#include "../../thirdparty/intelDecimal/LIBRARY/src/bid_functions.h"
#endif
#include "pd.hpp"

namespace fap
{
#define FAP_MONGO_CLIENT_VERSION_2       2
#define FAP_MONGO_CLIENT_VERSION_3       3
#define FAP_MONGO_CLIENT_SUBVERSION_2    2
#define FAP_MONGO_CLIENT_SUBVERSION_4    4
#define FAP_MONGO_CLIENT_FIXVERSION_12   12

#define FAP_MONGO_PLUS                   "+"
#define FAP_MONGO_MINUS_SIGN             "-"
#define FAP_MONGO_SPECIAL_STR_LEN        4
#define FAP_MONGO_MAX_OR_MIN_STR_LEN     3
#define FAP_MONGO_MAX_STR                "Max"
#define FAP_MONGO_MIN_STR                "Min"
#define FAP_MONGO_INF_STR                "Inf"
#define FAP_MONGO_NAN_STR                "Nan"

#define FAP_MONGO_DECIAML_STR_E_STR      "E"
#define FAP_MONGO_DECIAML_STR_E_CHAR     'E'

// 42 = 1( mantissa sign ) + 34( mantissa ) + 1( 'E' ) +
//      1( exponent sign ) + 4( exponent ) + 1( '\0' )
#define FAP_MONGO_DECIAML_BID_STR_MAX_SIZE   42

// 43 = 1( mantissa sign ) + 35( mantissa ) + 1( 'E' ) +
//      1( exponent sign ) + 4( exponent ) + 1( '\0' )
#define FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE   43

   static INT32 isNeedConvertToScientificNotation( const CHAR* pInputStr,
                                                   BOOLEAN &isNeed )
   {
      INT32  rc = SDB_OK ;
      UINT32 scale = 0 ;
      const CHAR* p = NULL ;
      isNeed = FALSE ;

      SDB_ASSERT( pInputStr != NULL , "Input str can't be NULL!" ) ;

      // srcStr is a string converted by IntelDecimal. Only 'E' is included in
      // the string. So we don't need to deal with the case of 'e'.
      p = ossStrrchr( pInputStr, FAP_MONGO_DECIAML_STR_E_CHAR ) ;
      if ( NULL == p )
      {
         // srcStr may be -15950735424
         goto done ;
      }

      // eg: srcStr = "-15950735424E-1010", p = "E-1010"
      if ( ossStrlen( p ) <= 2 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal exponent: %s, rc: %d", pInputStr, rc ) ;
         goto error ;
      }

      scale = ossAtoi( p + 2 ) ;
      if ( scale > 1000 )
      {
         if ( '+' == *( p + 1 ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid decimal scale: %d. It must be less "
                    "than 1000, rc: %d", scale, rc ) ;
            goto error ;
         }
         else if ( '-' == *( p + 1 ) )
         {
            isNeed = TRUE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid exponent sign: %c, rc: %d",
                    *( p + 1 ), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 convertToScientificNotation( const CHAR* pInputStr,
                                             UINT32 inputStrSize,
                                             CHAR* pOutputStr,
                                             UINT32 outputStrSize )
   {
      // eg:
      // -15950735424E-1010 == -1.5950735424E-1000
      // But in SequoiaDB, -15950735424E-1010 is invalid, because the scale of
      // decimal is more than 1000
      // So we should convert it to -1.5950735424E-1000

      INT32 rc = SDB_OK ;
      CHAR  srcStrBuf[ FAP_MONGO_DECIAML_BID_STR_MAX_SIZE ] = { 0 } ;
      CHAR* p = NULL ;
      CHAR* nextPtr = NULL ;
      UINT32 scale = 0 ;
      UINT32 newScale = 0 ;

      SDB_ASSERT( pInputStr != NULL , "Input str can't be NULL!" ) ;
      SDB_ASSERT( pOutputStr != NULL , "Output str can't be NULL!" ) ;

      // srcStr doesn't include decimal point( like: -15950735424E-1010 )
      // outputStr includes decimal point( like: -1.5950735424E-1000 )
      // so the maximum length of mantissa in srcStr is 34 and
      // the maximum length of mantissa in outputStr is 35
      if ( FAP_MONGO_DECIAML_BID_STR_MAX_SIZE != inputStrSize )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The size of input str is %d, but it must be %d"
                 " rc: %d", inputStrSize, FAP_MONGO_DECIAML_BID_STR_MAX_SIZE,
                 rc ) ;
         goto error ;
      }

      if ( FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE != outputStrSize )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The size of output str is %d, but it must be %d"
                 " rc: %d", inputStrSize,
                 FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE,
                 rc ) ;
         goto error ;
      }

      ossMemcpy( srcStrBuf, pInputStr, FAP_MONGO_DECIAML_BID_STR_MAX_SIZE ) ;

      p = ossStrtok( srcStrBuf, FAP_MONGO_DECIAML_STR_E_STR, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal str: %s, rc: %d", srcStrBuf, rc ) ;
         goto error ;
      }

      if ( '\0' == nextPtr[0] )
      {
         // srcStr may be -15950735424
         ossMemcpy( pOutputStr, pInputStr, FAP_MONGO_DECIAML_BID_STR_MAX_SIZE ) ;
         pOutputStr[FAP_MONGO_DECIAML_BID_STR_MAX_SIZE] = '\0' ;
         goto done ;
      }

      // eg: -15950735424E-1010, p = "-15950735424", nextPtr = "-1010"
      if ( ossStrlen( nextPtr ) <= 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal exponent: %s, rc: %d", pInputStr, rc ) ;
         goto error ;
      }

      scale = ossAtoi( nextPtr + 1 ) ;
      if ( scale <= 1000 )
      {
         ossMemcpy( pOutputStr, pInputStr, FAP_MONGO_DECIAML_BID_STR_MAX_SIZE ) ;
         pOutputStr[FAP_MONGO_DECIAML_BID_STR_MAX_SIZE] = '\0' ;
         goto done ;
      }
      else
      {
         if ( '+' == *nextPtr )
         {
            // newScale = scale + ossStrlen( p+2 ) ;
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid decimal scale: %d. It must be less "
                    "than 1000, rc: %d", scale, rc ) ;
            goto error ;
         }
         else if ( '-' == *nextPtr )
         {
            if ( ossStrlen( p ) < 2 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid decimal mantissa: %s, rc: %d",
                       pInputStr, rc ) ;
               goto error ;
            }
            // eg: -1E-1010, p = -1
            else if ( 2 == ossStrlen( p ) )
            {
               newScale = scale ;
            }
            else
            {
               newScale = scale - ossStrlen( p+2 ) ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid exponent sign: %c, rc: %d",
                    *nextPtr, rc ) ;
            goto error ;
         }
      }

      if ( newScale > 1000 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal scale: %d. It must be less "
                 "than 1000, rc: %d", scale, rc ) ;
         goto error ;
      }

      // '+' or '-' + mantissa + 'E' + '+' or '-' + exponent
      ossSnprintf( pOutputStr, FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE,
                   "%c%c.%sE%c%d",
                   *p, *(p+1), p+2, *nextPtr, newScale ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 isSdbRecordHasDecimal( const BSONObj &sdbRecord,
                                       BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() || Array == ele.type() )
            {
               rc = isSdbRecordHasDecimal( ele.Obj(), hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( hasDecimal )
               {
                  goto done ;
               }
            }
            else if ( NumberDecimal == ele.type() )
            {
               hasDecimal = TRUE ;
               goto done ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when determining whether "
                 "there is deciaml in the record: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   static BOOLEAN hasMongoSpecialStr( const CHAR* pDecimalStr )
   {
      BOOLEAN has = FALSE ;
      INT32   len = ossStrlen( pDecimalStr ) ;

      if ( FAP_MONGO_SPECIAL_STR_LEN == len )
      {
         if ( ( 0 == ossStrcasecmp( pDecimalStr,
                                    FAP_MONGO_PLUS FAP_MONGO_NAN_STR ) ) ||
              ( 0 == ossStrcasecmp( pDecimalStr,
                                    FAP_MONGO_MINUS_SIGN FAP_MONGO_NAN_STR ) ) ||
              ( 0 == ossStrcasecmp( pDecimalStr,
                                    FAP_MONGO_PLUS FAP_MONGO_MIN_STR ) ) ||
              ( 0 == ossStrcasecmp( pDecimalStr,
                                    FAP_MONGO_PLUS FAP_MONGO_MAX_STR ) ) ||
              ( 0 == ossStrcasecmp( pDecimalStr,
                                    FAP_MONGO_PLUS FAP_MONGO_INF_STR ) ) )
         {
            has = TRUE ;
         }
      }

      return has ;
   }

   static INT32 convertSdbMaxAndMin2Inf( string &decimalStr )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( FAP_MONGO_MAX_OR_MIN_STR_LEN == decimalStr.length() )
         {
            if ( 0 == ossStrcasecmp( decimalStr.c_str(), FAP_MONGO_MAX_STR ) )
            {
               decimalStr = FAP_MONGO_PLUS FAP_MONGO_INF_STR ;
            }
            else if ( 0 == ossStrcasecmp( decimalStr.c_str(),
                                          FAP_MONGO_MIN_STR ) )
            {
               decimalStr = FAP_MONGO_MINUS_SIGN FAP_MONGO_INF_STR ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when converting MAX or MIN to "
                 "INF: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

#if defined( _ARMLIN64 )
   // ARM64 doesn't support decimal in fap
   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONObjBuilder &mongoRecordBob,
                                  BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      // if sdbRecord has decimal, we don't need to check.
      if ( !hasDecimal )
      {
         rc = isSdbRecordHasDecimal( sdbRecord, hasDecimal ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( hasDecimal )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONArrayBuilder &mongoRecordBab,
                                  BufBuilder &mongoRecordBb )
   {
      return SDB_OK ;
   }

#else
   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONObjBuilder &mongoRecordBob,
                                  BOOLEAN &hasDecimal )
   {
      INT32 rc = SDB_OK ;

      // if sdbRecord has decimal, we don't need to check.
      if ( !hasDecimal )
      {
         rc = isSdbRecordHasDecimal( sdbRecord, hasDecimal ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // After checking, if sdbRecord doesn't has decimal,
      // we don't need to convert.
      if ( !hasDecimal )
      {
         goto done ;
      }

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob(
                              mongoRecordBob.subobjStart( ele.fieldName() ) ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BufBuilder &subBb = mongoRecordBob.subarrayStart(
                                   ele.fieldName() ) ;
               BSONArrayBuilder subBob( subBb ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, subBb ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( NumberDecimal == ele.type() )
            {
               UINT32 signalingFlags = 0 ;
               string value = ele.numberDecimal().toString() ;
               BID_UINT128 dec128 ;

               rc = convertSdbMaxAndMin2Inf( value ) ;
               if ( rc )
               {
                  goto error ;
               }

               dec128 = bid128_from_string(
                        const_cast< CHAR* >( value.c_str() ), 0,
                        &signalingFlags ) ;

               mongoRecordBob.bb().appendNum(
                                  (CHAR)FAP_MONGO_BSON_DECIMALBID_TYPE ) ;
               mongoRecordBob.bb().appendStr( ele.fieldName() );
               mongoRecordBob.bb().appendBuf( (void*)&dec128, sizeof(dec128) ) ;
            }
            else
            {
               mongoRecordBob.append( ele ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when converting sdb "
                 "decimal to mongo decimal: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbDecimal2MongoDecimal( const BSONObj &sdbRecord,
                                  BSONArrayBuilder &mongoRecordBab,
                                  BufBuilder &mongoRecordBb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( sdbRecord ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BOOLEAN hasDecimal = TRUE ;
               BSONObjBuilder subBob( mongoRecordBab.subobjStart() ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, hasDecimal ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BufBuilder &subBb = mongoRecordBab.subarrayStart() ;
               BSONArrayBuilder subBob( subBb ) ;
               rc = sdbDecimal2MongoDecimal( ele.Obj(), subBob, subBb ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( NumberDecimal == ele.type() )
            {
               UINT32 signalingFlags = 0 ;
               string value = ele.numberDecimal().toString() ;
               BID_UINT128 dec128 ;

               rc = convertSdbMaxAndMin2Inf( value ) ;
               if ( rc )
               {
                  goto error ;
               }

               dec128 = bid128_from_string(
                        const_cast< CHAR* >( value.c_str() ), 0,
                        &signalingFlags ) ;

               mongoRecordBb.appendNum( (CHAR)FAP_MONGO_BSON_DECIMALBID_TYPE ) ;
               mongoRecordBb.appendStr( ele.fieldName() );
               mongoRecordBb.appendBuf( (void*)&dec128, sizeof(dec128) ) ;
            }
            else
            {
               mongoRecordBab.append( ele ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when converting sdb "
                 "decimal to mongo decimal: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                  BSONObjBuilder &sdbMsgObjBob )
   {
      INT32 rc = SDB_OK ;

      try
      {
         mongoBSONObjIterator itr( mongoMsgObj ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob( sdbMsgObjBob.subobjStart(
                                      ele.fieldName() ) ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BSONArrayBuilder subBob( sdbMsgObjBob.subarrayStart(
                                        ele.fieldName() ) ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( FAP_MONGO_BSON_DECIMALBID_TYPE == ele.type() )
            {
#if defined( _ARMLIN64 )
               // ARM64 doesn't support decimal in fap
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "ARM64 doesn't support decimal in fap"
                       ", rc: %d", rc ) ;
               goto error ;
#else
               UINT32 signalingFlags = 0 ;
               BID_UINT128 dec128 ;
               CHAR decimalStr[ FAP_MONGO_DECIAML_BID_STR_MAX_SIZE ] = { 0 } ;
               BOOLEAN appendSucc = FALSE ;

               ossMemcpy( &dec128, ele.value(), FAP_MONGO_DECIAMLOID_SIZE ) ;
               bid128_to_string( decimalStr, dec128, &signalingFlags ) ;

               if ( hasMongoSpecialStr( decimalStr ) )
               {
                  // if the decimalStr is +NaN, +Inf, +Max or
                  // +Min( not case sensitive ), we should
                  // append NaN instead of +NaN, Inf instead of +Inf...
                  appendSucc = sdbMsgObjBob.appendDecimal( ele.fieldName(),
                                                           decimalStr+1 ) ;
               }
               else
               {
                  BOOLEAN isNeed = FALSE ;
                  rc = isNeedConvertToScientificNotation( decimalStr, isNeed ) ;
                  if ( rc )
                  {
                     goto error ;
                  }

                  if ( isNeed )
                  {
                     CHAR decimalStrNew[
                     FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE ] = { 0 } ;

                     rc = convertToScientificNotation( decimalStr,
                          FAP_MONGO_DECIAML_BID_STR_MAX_SIZE,
                          decimalStrNew,
                          FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE ) ;
                     if ( rc )
                     {
                        goto error ;
                     }
                     appendSucc = sdbMsgObjBob.appendDecimal( ele.fieldName(),
                                                              decimalStrNew ) ;
                  }
                  else
                  {
                     appendSucc = sdbMsgObjBob.appendDecimal( ele.fieldName(),
                                                              decimalStr ) ;
                  }
               }
               if ( !appendSucc )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Failed to append decimal, rc: %d", rc ) ;
                  goto error ;
               }
#endif
            }
            else
            {
               sdbMsgObjBob.append( ele ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when converting mongo "
                 "decimal to sdb decimal: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mongoDecimal2SdbDecimal( const BSONObj &mongoMsgObj,
                                  BSONArrayBuilder &sdbMsgObjBab )
   {
      INT32 rc = SDB_OK ;

      try
      {
         mongoBSONObjIterator itr( mongoMsgObj ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object == ele.type() )
            {
               BSONObjBuilder subBob( sdbMsgObjBab.subobjStart() ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( Array == ele.type() )
            {
               BSONArrayBuilder subBob( sdbMsgObjBab.subarrayStart() ) ;
               rc = mongoDecimal2SdbDecimal( ele.Obj(), subBob ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBob.doneFast() ;
            }
            else if ( FAP_MONGO_BSON_DECIMALBID_TYPE == ele.type() )
            {
#if defined( _ARMLIN64 )
               // ARM64 doesn't support decimal in fap
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "ARM64 doesn't support decimal in fap"
                       ", rc: %d", rc ) ;
               goto error ;
#else
               bsonDecimal decimalObj ;
               UINT32 signalingFlags = 0 ;
               BID_UINT128 dec128 ;
               CHAR decimalStr[FAP_MONGO_DECIAML_BID_STR_MAX_SIZE] = { 0 } ;

               ossMemcpy( &dec128, ele.value(), FAP_MONGO_DECIAMLOID_SIZE ) ;
               bid128_to_string( decimalStr, dec128, &signalingFlags ) ;

               if ( hasMongoSpecialStr( decimalStr ) )
               {
                  // if the decimalStr is +NaN, +Inf, +Max or
                  // +Min( not case sensitive ), we should
                  // append NaN instead of +NaN, Inf instead of +Inf...
                  rc = decimalObj.fromString( decimalStr+1 ) ;
               }
               else
               {
                  BOOLEAN isNeed = FALSE ;
                  rc = isNeedConvertToScientificNotation( decimalStr, isNeed ) ;
                  if ( rc )
                  {
                     goto error ;
                  }

                  if ( isNeed )
                  {
                     CHAR decimalStrNew[
                     FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE ] = { 0 } ;

                     rc = convertToScientificNotation( decimalStr,
                          FAP_MONGO_DECIAML_BID_STR_MAX_SIZE,
                          decimalStrNew,
                          FAP_MONGO_DECIMAL_SCIENTIFIC_NOTATION_STR_MAX_SIZE ) ;
                     if ( rc )
                     {
                        goto error ;
                     }
                     rc = decimalObj.fromString( decimalStrNew ) ;
                  }
                  else
                  {
                     rc = decimalObj.fromString( decimalStr ) ;
                  }
               }

               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to convert decimal str to "
                          "decimal obj, rc: %d", rc ) ;
                  goto error ;
               }

               sdbMsgObjBab.append( decimalObj ) ;
#endif
            }
            else
            {
               sdbMsgObjBab.append( ele ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when converting mongo "
                 "decimal to sdb decimal: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // MongoDB only started to support decimal after version 3.4. So if the
   // mongo shell version < 3.4, Java drive version < 3.4 or nodejs version <
   // 2.2.12, we don't need to convert mongodb decimal to sdb decimal
   BOOLEAN mongoIsSupportDecimal( const mongoClientInfo &clientInfo )
   {
      BOOLEAN isSupportDecimal = TRUE ;

      if ( JAVA_DRIVER == clientInfo.type ||
           MONGO_SHELL == clientInfo.type )
      {
         if ( ( clientInfo.version < FAP_MONGO_CLIENT_VERSION_3 ) ||
              ( FAP_MONGO_CLIENT_VERSION_3 == clientInfo.version &&
                clientInfo.subVersion < FAP_MONGO_CLIENT_SUBVERSION_4 ) )
         {
            isSupportDecimal = FALSE ;
         }
      }
      else if ( NODEJS_DRIVER == clientInfo.type )
      {
         if ( ( clientInfo.version < FAP_MONGO_CLIENT_VERSION_2 ) ||
              ( FAP_MONGO_CLIENT_VERSION_2 == clientInfo.version &&
                clientInfo.subVersion < FAP_MONGO_CLIENT_SUBVERSION_2 ) ||
              ( FAP_MONGO_CLIENT_VERSION_2 == clientInfo.version &&
                FAP_MONGO_CLIENT_SUBVERSION_2 == clientInfo.subVersion &&
                clientInfo.fixVersion < FAP_MONGO_CLIENT_FIXVERSION_12 ) )
         {
            isSupportDecimal = FALSE ;
         }
      }

      return isSupportDecimal ;
   }

}
