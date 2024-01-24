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

   Source File Name = mthCommon.hpp

   Descriptive Name = Method Common Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/12/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHCOMMON_HPP__
#define MTHCOMMON_HPP__

#include "core.hpp"
#include <vector>
#include "utilString.hpp"
#include "utilStr.hpp"
#include "../bson/bson.h"
#include <map>

using namespace bson ;

namespace engine
{

   #define MTH_OPERATOR_EYECATCHER              '$'

   // basic operation flags
   #define MTH_OPERATION_FLAG_OVERFLOW   0x00000001



   INT32 mthAppendString ( CHAR **ppStr, INT32 &bufLen,
                           INT32 strLen, const CHAR *newStr,
                           INT32 newStrLen, INT32 *pMergedLen = NULL ) ;

   INT32 mthDoubleBufferSize ( CHAR **ppStr, INT32 &bufLen ) ;

   BOOLEAN mthIsZero( const BSONElement &element ) ;

   INT32 mthCheckFieldName( const CHAR *pField, INT32 &dollarNum ) ;

   BOOLEAN mthCheckUnknowDollar( const CHAR *pField,
                                 std::vector<INT64> *dollarList ) ;

   INT32 mthConvertSubElemToNumeric( const CHAR *desc,
                                     INT32 &number ) ;

   BOOLEAN mthIsModValid( const BSONElement &modmEle ) ;

   INT32 mthAbs( const CHAR *name, const BSONElement &in,
                 BSONObjBuilder &outBuilder, INT32 &flag ) ;

   INT32 mthCeiling( const CHAR *name, const BSONElement &in,
                     BSONObjBuilder &outBuilder ) ;

   INT32 mthFloor( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder ) ;

   INT32 mthMod( const CHAR *name, const BSONElement &in,
                 const BSONElement &modm, BSONObjBuilder &outBuilder ) ;

   INT32 mthCast( const CHAR *name, const BSONElement &in,
                  BSONType targetType, BSONObjBuilder &outBuilder ) ;

   INT32 mthSlice( const CHAR *name, const BSONElement &in,
                   INT32 begin, INT32 limit, BSONObjBuilder &outBuilder ) ;

   INT32 mthSubStr( const CHAR *name, const BSONElement &in,
                    INT32 begin, INT32 limit, BSONObjBuilder &outBuilder ) ;

   INT32 mthStrLen( const CHAR *name, const BSONElement &in,
                    BSONObjBuilder &outBuilder ) ;
   INT32 mthStrLenBytes( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder ) ;
   INT32 mthStrLenCP( const CHAR *name, const BSONElement &in,
                      BSONObjBuilder &outBuilder ) ;

   INT32 mthLower( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder ) ;

   INT32 mthUpper( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder ) ;

   BOOLEAN mthIsTrimed( const CHAR *str, INT32 size, INT8 lr ) ;
   INT32 mthTrim( const CHAR *name, const BSONElement &in, INT8 lr,
                  BSONObjBuilder &outBuilder ) ;

   INT32 mthAdd( const CHAR *name, const BSONElement &in,
                 const BSONElement &addend,
                 BSONObjBuilder &outBuilder,
                 INT32 &flag ) ;

   INT32 mthSub( const CHAR *name, const BSONElement &in,
                 const BSONElement &subtrahead,
                 BSONObjBuilder &outBuilder,
                 INT32 &flag ) ;

   INT32 mthMultiply( const CHAR *name, const BSONElement &in,
                      const BSONElement &multiplier,
                      BSONObjBuilder &outBuilder,
                      INT32 &flag ) ;

   INT32 mthDivide( const CHAR *name, const BSONElement &in,
                    const BSONElement &divisor,
                    BSONObjBuilder &outBuilder,
                    INT32 &flag ) ;

   INT32 mthType( const CHAR *name, INT32 outType, const BSONElement &in,
                  BSONObjBuilder &outBuilder ) ;

   INT32 mthSize( const CHAR *name, const BSONElement &in,
                  BSONObjBuilder &outBuilder ) ;

   INT32 mthKeyString( const CHAR *name,
                       INT32 direction,
                       const BSONElement &in,
                       BSONObjBuilder &outBuilder ) ;

   class _mthCastTranslator
   {
   public:
      _mthCastTranslator() ;
      ~_mthCastTranslator() ;

   public:
      INT32 getCastType( const CHAR *typeStr, BSONType &type ) ;

      INT32 getCastStr( BSONType type, string &name ) ;

   private:
      typedef map< string, BSONType >        MTH_CAST_NAME_MAP ;
      MTH_CAST_NAME_MAP _castTransMap ;

      typedef map< BSONType, string >        MTH_CAST_TYPE_MAP ;
      MTH_CAST_TYPE_MAP _castTypeMap ;
   } ;

   _mthCastTranslator *mthGetCastTranslator() ;

   class _mthSliceIterator : public SDBObject
   {
   public:
      _mthSliceIterator( const bson::BSONObj &obj, INT32 begin = 0,
                         INT32 limit = -1 ) ;
      ~_mthSliceIterator() ;

   public:
      BOOLEAN more() ;
      bson::BSONElement next() ;

   private:
      bson::BSONObj _obj ;
      INT32 _where ;
      INT32 _limit ;
      bson::BSONObjIterator _itr ;
   } ;

   typedef class _mthSliceIterator mthSliceIterator ;

   struct mthStrcasecmp
   {
      BOOLEAN operator() ( const CHAR* a, const CHAR* b )
      {
         return ossStrcasecmp( a, b ) < 0 ;
      }
   } ;

   BOOLEAN mthIsNumber1( const bson::BSONElement &ele ) ;
   BOOLEAN mthIsValidLen( INT32 length ) ;

   INT32 mthCheckIfSubFieldIsOp( const BSONElement &ele,
                                 BOOLEAN &subFieldIsOp ) ;
}

#endif //MTHCOMMON_HPP__
