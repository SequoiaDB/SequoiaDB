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

   Source File Name = rtnPredicate.cpp

   Descriptive Name = Runtime Predicate

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code to generate
   predicate list from user's input.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnPredicate.hpp"
#include "ixm.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include <sstream>
#include "mthCommon.hpp"
#include "optCommon.hpp"
#include "msgDef.hpp"
#include "../bson/util/builder.h"
#include <boost/noncopyable.hpp>

namespace bson
{
   extern BSONObj staticNull ;
   extern BSONObj staticUndefined ;
   extern BSONObj minKey ;
   extern BSONObj maxKey ;
}

using namespace bson ;

namespace engine
{

   /*
      _rtnParamList implement
    */
   _rtnParamList::_rtnParamList ()
   : _paramNum( 0 )
   {
   }

   _rtnParamList::~_rtnParamList ()
   {
      clearParams() ;
   }

   void _rtnParamList::toBSON ( BSONObjBuilder &builder ) const
   {
      BSONArrayBuilder subBuilder(
                  builder.subarrayStart( FIELD_NAME_PARAMETERS ) ) ;
      for ( INT8 i = 0 ; i < _paramNum ; i ++ )
      {
         if ( !_params[ i ]._param.eoo() )
         {
            // WORKAROUND: this should not happened for eoo()
            subBuilder.append( _params[ i ]._param ) ;
         }
      }
      subBuilder.done() ;
   }

   BSONObj _rtnParamList::toBSON () const
   {
      // parameter with one integer is about 29 bytes
      // no need to use default initial size of BSON builder (512)
      BSONObjBuilder builder( 32 ) ;
      toBSON( builder ) ;
      return builder.obj() ;
   }

   string _rtnParamList::toString () const
   {
      return toBSON().toString( FALSE, TRUE ) ;
   }

   INT8 _rtnParamList::addParam ( const BSONElement &param )
   {
      if ( _paramNum < RTN_MAX_PARAM_NUM )
      {
         _params[ _paramNum ]._param = param ;
         _params[ _paramNum ]._doneByPred = FALSE ;
         return ( _paramNum ++ ) ;
      }
      return -1 ;
   }

   void _rtnParamList::setDoneByPred ( INT8 index )
   {
      if ( index >= 0 &&
           index < _paramNum )
      {
         _params[ index ]._doneByPred = TRUE ;
      }
   }

   INT32 _rtnParamList::buildValueSet ( INT8 index )
   {
      SDB_ASSERT( index >= 0 && index < _paramNum, "index is invalid" ) ;
      if ( Array == _params[ index ]._param.type() &&
            NULL == _params[ index ]._valueSet )
      {
         _params[ index ]._valueSet = SDB_OSS_NEW rtnParamValueSet() ;
         if ( NULL == _params[ index ]._valueSet )
         {
            return SDB_OOM ;
         }
         _params[ index ]._valueSet->buildValueSet( _params[ index ]._param ) ;
         return SDB_OK ;
      }
      return SDB_INVALIDARG ;
   }

   /*
      Static minimum and maximum keys in range predicates
    */
   static BSONObj _rtnKeyBuildMinTypeObj ( BSONType type ) ;
   static BSONObj _rtnKeyBuildMaxTypeObj ( BSONType type ) ;

   static BSONObj _rtnKeyMinMinKey = _rtnKeyBuildMinTypeObj( MinKey ) ;
   static BSONObj _rtnKeyMinNumberDouble = _rtnKeyBuildMinTypeObj( NumberDouble ) ;
   static BSONObj _rtnKeyMinString = _rtnKeyBuildMinTypeObj( String ) ;
   static BSONObj _rtnKeyMinObject = _rtnKeyBuildMinTypeObj( Object ) ;
   static BSONObj _rtnKeyMinArray = _rtnKeyBuildMinTypeObj( Array ) ;
   static BSONObj _rtnKeyMinBinData = _rtnKeyBuildMinTypeObj( BinData ) ;
   static BSONObj _rtnKeyMinUndefined = _rtnKeyBuildMinTypeObj( Undefined ) ;
   static BSONObj _rtnKeyMinJstOID = _rtnKeyBuildMinTypeObj( jstOID ) ;
   static BSONObj _rtnKeyMinBool = _rtnKeyBuildMinTypeObj( Bool ) ;
   static BSONObj _rtnKeyMinDate = _rtnKeyBuildMinTypeObj( Date ) ;
   static BSONObj _rtnKeyMinJstNULL = _rtnKeyBuildMinTypeObj( jstNULL ) ;
   static BSONObj _rtnKeyMinRegEx = _rtnKeyBuildMinTypeObj( RegEx ) ;
   static BSONObj _rtnKeyMinDBRef = _rtnKeyBuildMinTypeObj( DBRef ) ;
   static BSONObj _rtnKeyMinCode = _rtnKeyBuildMinTypeObj( Code ) ;
   static BSONObj _rtnKeyMinSymbol = _rtnKeyBuildMinTypeObj( Symbol ) ;
   static BSONObj _rtnKeyMinCodeWScope = _rtnKeyBuildMinTypeObj( CodeWScope ) ;
   static BSONObj _rtnKeyMinNumberInt = _rtnKeyBuildMinTypeObj( NumberInt ) ;
   static BSONObj _rtnKeyMinTimestamp = _rtnKeyBuildMinTypeObj( Timestamp ) ;
   static BSONObj _rtnKeyMinNumberLong = _rtnKeyBuildMinTypeObj( NumberLong ) ;
   static BSONObj _rtnKeyMinNumberDecimal = _rtnKeyBuildMinTypeObj( NumberDecimal ) ;
   static BSONObj _rtnKeyMinMaxKey = _rtnKeyBuildMinTypeObj( MaxKey ) ;

   static BSONObj _rtnKeyMaxMinKey = _rtnKeyBuildMaxTypeObj( MinKey ) ;
   static BSONObj _rtnKeyMaxNumberDouble = _rtnKeyBuildMaxTypeObj( NumberDouble ) ;
   static BSONObj _rtnKeyMaxString = _rtnKeyBuildMaxTypeObj( String ) ;
   static BSONObj _rtnKeyMaxObject = _rtnKeyBuildMaxTypeObj( Object ) ;
   static BSONObj _rtnKeyMaxArray = _rtnKeyBuildMaxTypeObj( Array ) ;
   static BSONObj _rtnKeyMaxBinData = _rtnKeyBuildMaxTypeObj( BinData ) ;
   static BSONObj _rtnKeyMaxUndefined = _rtnKeyBuildMaxTypeObj( Undefined ) ;
   static BSONObj _rtnKeyMaxJstOID = _rtnKeyBuildMaxTypeObj( jstOID ) ;
   static BSONObj _rtnKeyMaxBool = _rtnKeyBuildMaxTypeObj( Bool ) ;
   static BSONObj _rtnKeyMaxDate = _rtnKeyBuildMaxTypeObj( Date ) ;
   static BSONObj _rtnKeyMaxJstNULL = _rtnKeyBuildMaxTypeObj( jstNULL ) ;
   static BSONObj _rtnKeyMaxRegEx = _rtnKeyBuildMaxTypeObj( RegEx ) ;
   static BSONObj _rtnKeyMaxDBRef = _rtnKeyBuildMaxTypeObj( DBRef ) ;
   static BSONObj _rtnKeyMaxCode = _rtnKeyBuildMaxTypeObj( Code ) ;
   static BSONObj _rtnKeyMaxSymbol = _rtnKeyBuildMaxTypeObj( Symbol ) ;
   static BSONObj _rtnKeyMaxCodeWScope = _rtnKeyBuildMaxTypeObj( CodeWScope ) ;
   static BSONObj _rtnKeyMaxNumberInt = _rtnKeyBuildMaxTypeObj( NumberInt ) ;
   static BSONObj _rtnKeyMaxTimestamp = _rtnKeyBuildMaxTypeObj( Timestamp ) ;
   static BSONObj _rtnKeyMaxNumberLong = _rtnKeyBuildMaxTypeObj( NumberLong ) ;
   static BSONObj _rtnKeyMaxNumberDecimal = _rtnKeyBuildMaxTypeObj( NumberDecimal ) ;
   static BSONObj _rtnKeyMaxMaxKey = _rtnKeyBuildMaxTypeObj( MaxKey ) ;

   static BSONObj _rtnKeyBuildMinTypeObj ( BSONType type )
   {
      BSONObjBuilder b ;
      b.appendMinForType( "", type ) ;
      return b.obj() ;
   }

   static BSONObj _rtnKeyBuildMaxTypeObj ( BSONType type )
   {
      BSONObjBuilder b ;
      b.appendMaxForType( "", type ) ;
      return b.obj() ;
   }

   BSONObj rtnKeyGetMinType ( INT32 bsonType )
   {
      switch ( bsonType )
      {
         case MinKey :
            return minKey ;
         case NumberDouble :
            return _rtnKeyMinNumberDouble ;
         case String :
            return _rtnKeyMinString ;
         case Object :
            return _rtnKeyMinObject ;
         case Array :
            return _rtnKeyMinArray ;
         case BinData :
            return _rtnKeyMinBinData ;
         case Undefined :
            return _rtnKeyMinUndefined ;
         case jstOID :
            return _rtnKeyMinJstOID ;
         case Bool :
            return _rtnKeyMinBool ;
         case Date :
            return _rtnKeyMinDate ;
         case jstNULL :
            return _rtnKeyMinJstNULL ;
         case RegEx :
            return _rtnKeyMinRegEx ;
         case DBRef :
            return _rtnKeyMinDBRef ;
         case Code :
            return _rtnKeyMinCode ;
         case Symbol :
            return _rtnKeyMinSymbol ;
         case CodeWScope :
            return _rtnKeyMinCodeWScope ;
         case NumberInt :
            return _rtnKeyMinNumberInt ;
         case Timestamp :
            return _rtnKeyMinTimestamp ;
         case NumberLong :
            return _rtnKeyMinNumberLong ;
         case NumberDecimal :
            return _rtnKeyMinNumberDecimal ;
         case MaxKey :
            return maxKey ;
         default :
            break ;
      }

      return minKey ;
   }

   BSONObj rtnKeyGetMaxType ( INT32 bsonType )
   {
      switch ( bsonType )
      {
         case MinKey :
            return minKey ;
         case NumberDouble :
            return _rtnKeyMaxNumberDouble ;
         case String :
            return _rtnKeyMaxString ;
         case Object :
            return _rtnKeyMaxObject ;
         case Array :
            return _rtnKeyMaxArray ;
         case BinData :
            return _rtnKeyMaxBinData ;
         case Undefined :
            return _rtnKeyMaxUndefined ;
         case jstOID :
            return _rtnKeyMaxJstOID ;
         case Bool :
            return _rtnKeyMaxBool ;
         case Date :
            return _rtnKeyMaxDate ;
         case jstNULL :
            return _rtnKeyMaxJstNULL ;
         case RegEx :
            return _rtnKeyMaxRegEx ;
         case DBRef :
            return _rtnKeyMaxDBRef ;
         case Code :
            return _rtnKeyMaxCode ;
         case Symbol :
            return _rtnKeyMaxSymbol ;
         case CodeWScope :
            return _rtnKeyMaxCodeWScope ;
         case NumberInt :
            return _rtnKeyMaxNumberInt ;
         case Timestamp :
            return _rtnKeyMaxTimestamp ;
         case NumberLong :
            return _rtnKeyMaxNumberLong ;
         case NumberDecimal :
            return _rtnKeyMaxNumberDecimal ;
         case MaxKey :
            return maxKey ;
         default :
            break ;
      }

      return maxKey ;
   }

   BSONObj rtnKeyGetMinForCmp ( INT32 bsonType, BOOLEAN mixCmp )
   {
      if ( mixCmp )
      {
         return minKey ;
      }

      switch ( bsonType )
      {
         case MinKey :
            return _rtnKeyMinMinKey ;

         // Should perform a full range compare
         case MaxKey :
            return minKey ;

         // Decimal should cover Double, Int and Long
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
            return _rtnKeyMinNumberDecimal ;

         // Date should cover Timestamp
         case Date :
         case Timestamp :
            return _rtnKeyMinDate ;

         case String :
            return _rtnKeyMinString ;
         case Object :
            return _rtnKeyMinObject ;
         case Array :
            return _rtnKeyMinArray ;
         case BinData :
            return _rtnKeyMinBinData ;
         case Undefined :
            return _rtnKeyMinUndefined ;
         case jstOID :
            return _rtnKeyMinJstOID ;
         case Bool :
            return _rtnKeyMinBool ;
         case jstNULL :
            return _rtnKeyMinJstNULL ;
         case RegEx :
            return _rtnKeyMinRegEx ;
         case DBRef :
            return _rtnKeyMinDBRef ;
         case Code :
            return _rtnKeyMinCode ;
         case Symbol :
            return _rtnKeyMinSymbol ;
         case CodeWScope :
            return _rtnKeyMinCodeWScope ;
         default :
            break ;
      }

      return minKey ;
   }

   BSONObj rtnKeyGetMaxForCmp ( INT32 bsonType, BOOLEAN mixCmp )
   {
      if ( mixCmp )
      {
         return maxKey ;
      }

      switch ( bsonType )
      {
         // Should perform a full range compare
         case MinKey :
            return maxKey ;

         case MaxKey :
            return _rtnKeyMaxMaxKey ;

         // Decimal should cover Double, Int and Long
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
            return _rtnKeyMaxNumberDecimal ;

         // Date should cover Timestamp
         case Date :
         case Timestamp :
            return _rtnKeyMaxDate ;

         case String :
            return _rtnKeyMaxString ;
         case Object :
            return _rtnKeyMaxObject ;
         case Array :
            return _rtnKeyMaxArray ;
         case BinData :
            return _rtnKeyMaxBinData ;
         case Undefined :
            return _rtnKeyMaxUndefined ;
         case jstOID :
            return _rtnKeyMaxJstOID ;
         case Bool :
            return _rtnKeyMaxBool ;
         case jstNULL :
            return _rtnKeyMaxJstNULL ;
         case RegEx :
            return _rtnKeyMaxRegEx ;
         case DBRef :
            return _rtnKeyMaxDBRef ;
         case Code :
            return _rtnKeyMaxCode ;
         case Symbol :
            return _rtnKeyMaxSymbol ;
         case CodeWScope :
            return _rtnKeyMaxCodeWScope ;
         default :
            break ;
      }

      return maxKey ;
   }

   // this class is only used in rtnPredicate::operator|=
   class startStopKeyUnionBuilder : boost::noncopyable
   {
   public :
      startStopKeyUnionBuilder()
      {
         _init = FALSE ;
      }
      void next ( const rtnStartStopKey &n )
      {
         if ( !_init )
         {
            _tail = n ;
            _init = TRUE ;
            return ;
         }
         _merge(n) ;
      }
      void done()
      {
         if ( _init )
            _result.push_back ( _tail ) ;
      }
      const RTN_SSKEY_LIST &result() const
      {
         return _result ;
      }
   private:
      // if start/stopKey n doesn't joint with existing _tail, it will push
      // _tail to _result and let _tail=n and return FALSE
      // otherwise it will return TRUE
      void _merge ( const rtnStartStopKey &n )
      {
         // compare tail.stop and n.start, make sure they are joint
         INT32 cmp = _tail._stopKey._bound.woCompare ( n._startKey._bound,
                                                       FALSE ) ;
         if ((cmp<0) || (cmp==0 && !_tail._stopKey._inclusive &&
                                   !n._startKey._inclusive ))
         {
            _result.push_back ( _tail ) ;
            _tail = n ;
            return ;
         }
         // make sure n is not part of tail by comparing tail.stopKey and
         // n.stopKey. If n.stopKey is smaller or equal to tail.stopKey, that
         // means we don't have to extend
         cmp = _tail._stopKey._bound.woCompare ( n._stopKey._bound, FALSE ) ;
         if ( (cmp<0) || (cmp==0 && !_tail._stopKey._inclusive &&
                                    n._stopKey._inclusive ))
         {
            _tail._stopKey = n._stopKey ;
            // Have different major types, set to default
            if ( _tail._majorType != n._majorType )
            {
               _tail._majorType = RTN_KEY_MAJOR_DEFAULT ;
            }
         }
      }
      BOOLEAN _init ;
      rtnStartStopKey _tail ;
      RTN_SSKEY_LIST _result ;
   } ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_TOSTRING, "rtnStartStopKey::toString" )
   string rtnStartStopKey::toString() const
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_TOSTRING ) ;
      StringBuilder buf ;
      buf << ( _startKey._inclusive ? "[":"(") << " " ;
      buf << _startKey._bound.toString( FALSE ) ;
      buf << ", " ;
      buf << _stopKey._bound.toString( FALSE ) ;
      buf << " " << (_stopKey._inclusive?"]":")") ;
      PD_TRACE_EXIT ( SDB_RTNSSKEY_TOSTRING ) ;
      return buf.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_RESET, "rtnStartStopKey::reset" )
   void rtnStartStopKey::reset ()
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_RESET ) ;
      _startKey._bound = bson::minKey.firstElement() ;
      _startKey._inclusive = FALSE ;
      _stopKey._bound = bson::maxKey.firstElement() ;
      _stopKey._inclusive = FALSE ;
      PD_TRACE_EXIT ( SDB_RTNSSKEY_RESET ) ;
   }

   #define RTN_START_STOP_KEY_START "a"
   #define RTN_START_STOP_KEY_STOP  "o"
   #define RTN_START_STOP_KEY_INCL  "i"

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_FROMBSON, "rtnStartStopKey::fromBson" )
   BOOLEAN rtnStartStopKey::fromBson ( BSONObj &ob )
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_FROMBSON ) ;
      BOOLEAN ret = TRUE ;
      try
      {
         BSONElement startBE = ob.getField ( RTN_START_STOP_KEY_START ) ;
         BSONElement stopBE  = ob.getField ( RTN_START_STOP_KEY_STOP ) ;
         if ( startBE.type() != Object )
         {
            ret = FALSE ;
            goto done ;
         }
         if ( stopBE.type() != Object )
         {
            ret = FALSE ;
            goto done ;
         }
         BSONObj startKey = startBE.embeddedObject() ;
         _startKey._bound = startKey.firstElement () ;
         _startKey._inclusive = startKey.hasElement ( RTN_START_STOP_KEY_INCL );
         BSONObj stopKey = stopBE.embeddedObject() ;
         _stopKey._bound = stopKey.firstElement () ;
         _stopKey._inclusive = stopKey.hasElement ( RTN_START_STOP_KEY_INCL ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to extract rtnStartSTopKey from %s, %s",
                  ob.toString().c_str(), e.what() ) ;
         ret = FALSE ;
         goto done ;
      }
   done :
      PD_TRACE1 ( SDB_RTNSSKEY_FROMBSON, PD_PACK_INT ( ret ) ) ;
      PD_TRACE_EXIT ( SDB_RTNSSKEY_FROMBSON ) ;
      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_TOBSON, "rtnStartStopKey::toBson" )
   BSONObj rtnStartStopKey::toBson() const
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_TOBSON ) ;

      BSONObjBuilder ob( 128 ) ;

      BSONObjBuilder startOB( ob.subobjStart( RTN_START_STOP_KEY_START ) ) ;
      startOB.append ( _startKey._bound ) ;
      if ( _startKey._inclusive )
      {
         startOB.append ( RTN_START_STOP_KEY_INCL, TRUE ) ;
      }
      startOB.done() ;

      BSONObjBuilder stopOB( ob.subobjStart( RTN_START_STOP_KEY_STOP ) ) ;
      stopOB.append ( _stopKey._bound ) ;
      if ( _stopKey._inclusive )
      {
         stopOB.append ( RTN_START_STOP_KEY_INCL, TRUE ) ;
      }
      stopOB.done() ;

      PD_TRACE_EXIT ( SDB_RTNSSKEY_TOBSON ) ;
      return ob.obj () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNKEYCOMPARE, "rtnKeyCompare" )
   INT32 rtnKeyCompare ( const BSONElement &l, const BSONElement &r )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNKEYCOMPARE ) ;
      INT32 f = l.canonicalType() - r.canonicalType() ;
      if ( f != 0 )
      {
         rc = f ;
         goto done ;
      }
      rc = compareElementValues ( l, r ) ;
   done :
      PD_TRACE_EXITRC ( SDB_RTNKEYCOMPARE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_COMPARE1, "rtnStartStopKey::compare" )
   RTN_SSK_VALUE_POS rtnStartStopKey::compare ( BSONElement &ele,
                                                INT32 dir ) const
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_COMPARE1 ) ;
      INT32 posStart = 0 ;
      INT32 posStop  = 0 ;
      RTN_SSK_VALUE_POS pos ;
      SDB_ASSERT ( isValid (), "this object is not valid" ) ;

      if ( dir >= 0 )
      {
         posStart = rtnKeyCompare ( ele, _startKey._bound ) ;
      }
      else
      {
         posStart = rtnKeyCompare ( ele, _stopKey._bound ) ;
      }
      // case:
      //       e
      //           { range ?
      if ( posStart < 0 )
      {
         pos = RTN_SSK_VALUE_POS_LT ;
      }
      // case :
      //      e
      //      { range ?
      else if ( posStart == 0 )
      {
         if ( dir >= 0 )
         {
            pos = _startKey._inclusive ?
                    RTN_SSK_VALUE_POS_LET : RTN_SSK_VALUE_POS_LT ;
         }
         else
         {
            pos = _stopKey._inclusive ?
                    RTN_SSK_VALUE_POS_LET : RTN_SSK_VALUE_POS_LT ;
         }
      }
      // case :
      //       e
      //     { range ?
      if ( dir >= 0 )
      {
         posStop  = rtnKeyCompare ( ele, _stopKey._bound ) ;
      }
      else
      {
         posStop = rtnKeyCompare ( ele, _startKey._bound ) ;
      }
      // case :
      //       e
      //    { range }
      if ( posStart > 0 && posStop < 0 )
      {
         pos = RTN_SSK_VALUE_POS_WITHIN ;
      }
      // case :
      //           e
      //   { range }
      else if ( posStop == 0 )
      {
         if ( dir >= 0 )
         {
            pos = _stopKey._inclusive ?
                     RTN_SSK_VALUE_POS_GET : RTN_SSK_VALUE_POS_GT ;
         }
         else
         {
            pos = _startKey._inclusive ?
                     RTN_SSK_VALUE_POS_GET : RTN_SSK_VALUE_POS_GT ;
         }
      }
      // case :
      //             e
      //   { range }
      else
      {
         pos = RTN_SSK_VALUE_POS_GT ;
      }
      if ( dir < 0 )
      {
         pos = (RTN_SSK_VALUE_POS)(((INT32)pos) * -1) ;
      }

      PD_TRACE_EXIT ( SDB_RTNSSKEY_COMPARE1 ) ;
      return pos ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSSKEY_COMPARE2, "rtnStartStopKey::compare" )
   RTN_SSK_RANGE_POS rtnStartStopKey::compare ( rtnStartStopKey &key,
                                                INT32 dir ) const
   {
      PD_TRACE_ENTRY ( SDB_RTNSSKEY_COMPARE2 ) ;
      RTN_SSK_VALUE_POS posStart ;
      RTN_SSK_VALUE_POS posStop ;
      RTN_SSK_RANGE_POS pos = RTN_SSK_RANGE_POS_RT ;
      SDB_ASSERT ( key.isValid (), "key is not valid!" ) ;

      if ( dir >= 0 )
      {
         posStart = compare ( key._startKey._bound, 1 ) ;
         posStop = compare ( key._stopKey._bound, 1 ) ;
      }
      else
      {
         posStart = compare ( key._stopKey._bound, -1 ) ;
         posStop = compare ( key._startKey._bound, -1 ) ;
      }
      if ( RTN_SSK_VALUE_POS_LT == posStart )
      {
         // key._startKey is less than edge, in this case we may have less than,
         // left edge, left
         // intersect or contain case
         if ( RTN_SSK_VALUE_POS_LT == posStop )
         {
            // both key's start/stop less than current range
            // case :
            //      [ key ]
            //              [ this ]
            pos = RTN_SSK_RANGE_POS_LT ;
         }
         else if ( RTN_SSK_VALUE_POS_LET == posStop )
         {
            // case :
            //     [ key ]
            //           [ this ]
            // or :
            //     [ key )
            //           [ this ]
            if ( dir >= 0 )
            {
               pos = key._stopKey._inclusive ?
                   RTN_SSK_RANGE_POS_LET : RTN_SSK_RANGE_POS_LT ;
            }
            else
            {
               pos = key._startKey._inclusive ?
                   RTN_SSK_RANGE_POS_LET : RTN_SSK_RANGE_POS_LT ;
            }
         }
         else if ( RTN_SSK_VALUE_POS_WITHIN == posStop )
         {
            // case :
            //    [ key ]
            //       [ this ]
            pos = RTN_SSK_RANGE_POS_LI ;
         }
         // case :
         //    [    key  ]
         //       [ this ]
         // or :
         //    [   key     ]
         //       [ this ]
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_LET == posStart )
      {
         // key._startKey is at edge, in this case we may have exact edge case
         // or contain case
         if ( RTN_SSK_VALUE_POS_LET == posStop )
         {
            // case :
            //    k
            //    [ this ]
            pos = RTN_SSK_RANGE_POS_LET ;
         }
         // case :
         //    [ key ]
         //    [  this  ]
         // or :
         //    [ key  ]
         //    [ this ]
         // or :
         //    [ key          ]
         //    [ this ]
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_WITHIN == posStart )
      {
         // key._startKey is within range, in this case we may have contain
         // case or right intersect
         if ( RTN_SSK_VALUE_POS_GT == posStop )
         {
            // case :
            //    [   key   ]
            //  [ this ]
            pos = RTN_SSK_RANGE_POS_RI ;
         }
         // case :
         //     [ key ]
         //   [ this  ]
         // or :
         //     [ key ]
         //   [ this   ]
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_GET == posStart )
      {
         // key._startKey is at right edge, in this case we must be right edge
         // case :
         //          [ key ]
         //   [ this ]
         //
         // or :
         //         k
         //  [ this ]
         // or :
         //         ( key ]
         //  [ this ]
         if ( dir >= 0 )
         {
            pos = key._startKey._inclusive ?
                   RTN_SSK_RANGE_POS_RET : RTN_SSK_RANGE_POS_RT ;
         }
         else
         {
            pos = key._stopKey._inclusive ?
                   RTN_SSK_RANGE_POS_RET : RTN_SSK_RANGE_POS_RT ;
         }
      }
      // in this case we must be right than
      // case :
      //                  [ key ]
      //       [ this ]
      PD_TRACE_EXIT ( SDB_RTNSSKEY_COMPARE2 ) ;
      return pos ;
   }

   // Check if regex contains unescaped pipe '|' character
   // NOTE: this may return false positives. e.g. pipe characters inside
   // \Q...\E sequence, etc.
   static BOOLEAN regexHasUnescapedPipe ( const CHAR * regex )
   {
      BOOLEAN isEscape = FALSE ;
      for ( ; *regex != '\0' ; regex ++ )
      {
         if ( *regex == '|' && !isEscape )
         {
            return TRUE ;
         }
         else if ( *regex == '\\' && !isEscape )
         {
            isEscape = TRUE ;
         }
         else if ( isEscape )
         {
            isEscape = FALSE ;
         }
      }
      return FALSE ;
   }

   // input: regex for regular expression string
   // input: flags for re flag
   // output: whether it's purePrefix (start with ^)
   // return regular expression string
   // PD_TRACE_DECLARE_FUNCTION ( SDB_SIMAPLEREGEX1, "simpleRegex" )
   string simpleRegex ( const CHAR* regex,
                        const CHAR* flags,
                        BOOLEAN *purePrefix )
   {
      PD_TRACE_ENTRY ( SDB_SIMAPLEREGEX1 ) ;
      // by default return empty string
      BOOLEAN extended = FALSE ;
      BOOLEAN multilineOK = FALSE ;
      string r = "";
      stringstream ss ;

      if ( purePrefix )
      {
         *purePrefix = FALSE;
      }

      if ( regex[0] == '\\' && regex[1] == 'A')
      {
         multilineOK = TRUE ;
         regex += 2;
      }
      else if (regex[0] == '^')
      {
          multilineOK = FALSE ;
          regex += 1;
      }
      else
      {
          // only able to start with "\\A" or "^"
          // otherwise return ""
          goto done ;
      }

      while (*flags)
      {
         switch (*(flags++))
         {
         case 'm': // multiline
            if ( multilineOK )
            {
               continue;
            }
            else
            {
               goto done ;
            }
         case 'x': // extended
            extended = TRUE;
            break;
         case 's':
            continue;
         default:
            goto done ; // can't use index
         }
      }

      while(*regex)
      {
         CHAR c = *(regex++);
         if ( c == '*' || c == '?' )
         {
            // These are the only two symbols that make the last char
            // optional
            r = ss.str();
            r = r.substr( 0 , r.size() - 1 );
            goto done ; //breaking here fails with /^a?/
         }
         else if (c == '|')
         {
            // whole match so far is optional. Nothing we can do here.
            r = string();
            goto done ;
         }
         else if (c == '\\')
         {
            c = *(regex++);
            if (c == 'Q')
            {
               // \Q...\E quotes everything inside
               while (*regex)
               {
                  c = (*regex++);
                  if (c == '\\' && (*regex == 'E'))
                  {
                     regex++; //skip the 'E'
                     break; // go back to start of outer loop
                  }
                  else
                  {
                     ss << c; // character should match itself
                  }
               }
            }
            else if ( c == 'n' )
            {
               ss << "\n" ;
            }
            else if ( c == 'r' )
            {
               ss << "\r" ;
            }
            else if ( c == 't' )
            {
               ss << "\t" ;
            }
            else if ((c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '\0'))
            {
               // don't know what to do with these
               r = ss.str();
               break;
            }
            else
            {
               // slash followed by non-alphanumeric represents the
               // following char
               ss << c;
            }
         } // else if (c == '\\')
         else if (strchr("^$.[()+{", c))
         {
            // list of "metacharacters" from man pcrepattern
            r = ss.str();
            break;
         }
         else if (extended && c == '#')
         {
            // comment
            r = ss.str();
            break;
         }
         else if (extended && isspace(c))
         {
            continue;
         }
         else
         {
            // self-matching char
            ss << c;
         }
      }

      if ( r.empty() && *regex == 0 )
      {
         r = ss.str();
         if ( purePrefix )
         {
            *purePrefix = !r.empty();
         }
      }
      else if ( !r.empty() )
      {
         // Check renaming string against '|'
         // A regex with '|' is not considered as a simple regex
         if ( regexHasUnescapedPipe( regex ) )
         {
            // Containing '|' could not be simplified
            r = string() ;
         }
      }

   done :
      PD_TRACE1 ( SDB_SIMAPLEREGEX1, PD_PACK_STRING ( r.c_str() ) ) ;
      PD_TRACE_EXIT ( SDB_SIMAPLEREGEX1 ) ;
      return r;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SIMAPLEREGEX2, "simpleRegex" )
   inline string simpleRegex(const BSONElement& e)
   {
      PD_TRACE_ENTRY ( SDB_SIMAPLEREGEX2 ) ;
      string r = "" ;
      switch(e.type())
      {
      case RegEx:
         r = simpleRegex(e.regex(), e.regexFlags(), NULL);
         break ;
      case Object:
      {
         BSONObj o = e.embeddedObject();
         r = simpleRegex(o["$regex"].valuestrsafe(),
                            o["$options"].valuestrsafe(),
                            NULL);
         break ;
      }
      default:
         break ;
      }
      PD_TRACE1 ( SDB_SIMAPLEREGEX2, PD_PACK_STRING ( r.c_str() ) ) ;
      PD_TRACE_EXIT ( SDB_SIMAPLEREGEX2 ) ;
      return r ;
   }

   string simpleRegexEnd( string regex )
   {
      ++regex[ regex.length() - 1 ];
      return regex;
   }


   // without inclusive when _bound check matches.
   // otherwise if this is max bound of stop keys, we want to return the one
   // with inclusive when _bound check matches
   rtnKeyBoundary maxKeyBound ( const rtnKeyBoundary &l,
                                const rtnKeyBoundary &r,
                                BOOLEAN startKey )
   {
      INT32 result = l._bound.woCompare ( r._bound, FALSE ) ;
      if ( result < 0  || (result == 0 &&
              (startKey?(!r._inclusive):(r._inclusive)) ) )
         return r ;
      return l ;
   }
   // if we want to find the min bound of stop keys, we want to return the ones
   // without inclusive when _bound check matches.
   // otherwise if this is min bound of start keys, we want to return the one
   // with inclusive when -bound check matches
   rtnKeyBoundary minKeyBound ( const rtnKeyBoundary &l,
                                const rtnKeyBoundary &r,
                                BOOLEAN stopKey )
   {
      INT32 result = l._bound.woCompare ( r._bound, FALSE ) ;
      if ( result > 0  || (result == 0 &&
              (stopKey?(!r._inclusive):(r._inclusive)) ))
         return r ;
      return l ;
   }
   // return TRUE when l and r are intersect, and result is set to the intersect
   // of l and r
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREDOVERLAP, "predicatesOverlap" )
   BOOLEAN predicatesOverlap ( const rtnStartStopKey &l,
                               const rtnStartStopKey &r,
                               rtnStartStopKey &result )
   {
      PD_TRACE_ENTRY ( SDB_PREDOVERLAP ) ;

      result._startKey = maxKeyBound ( l._startKey, r._startKey, TRUE ) ;
      result._stopKey = minKeyBound ( l._stopKey, r._stopKey, TRUE ) ;

      // left and right have the same major type, set to the result
      if ( l._majorType == r._majorType )
      {
         result._majorType = l._majorType ;
      }

      PD_TRACE_EXIT ( SDB_PREDOVERLAP ) ;
      return result.isValid() ;
   }

   void rtnPredicate::finishOperation ( const RTN_SSKEY_LIST &newkeys,
                                        const rtnPredicate &other )
   {
      _startStopKeys = newkeys ;
      for ( VEC_OBJ_DATA::const_iterator i = other._objData.begin() ;
            i != other._objData.end(); i++ )
      {
         _objData.push_back(*i) ;
      }
   }

   // intersection operation for two keysets
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_OPEQU, "rtnPredicate::operator&=" )
   const rtnPredicate &rtnPredicate::operator&= ( const rtnPredicate &right )
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPEQU ) ;
      RTN_SSKEY_LIST newKeySet ;
      RTN_SSKEY_LIST::const_iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::const_iterator j = right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end() )
      {
         rtnStartStopKey overlap ;
         if ( predicatesOverlap ( *i, *j, overlap ) )
         {
            newKeySet.push_back ( overlap ) ;
         }
         if ( i->_stopKey == minKeyBound ( i->_stopKey, j->_stopKey, TRUE ) )
         {
            ++i ;
         }
         else
         {
            ++j ;
         }
      }
      finishOperation ( newKeySet, right ) ;

      // Accumulate saved cpu costs
      _savedCPUCost += right._savedCPUCost ;

      PD_TRACE_EXIT ( SDB_RTNPRED_OPEQU ) ;
      return *this ;
   }

   const rtnPredicate &rtnPredicate::operator=(const rtnPredicate &right)
   {
      finishOperation(right._startStopKeys, right) ;
      _paramIndex = right._paramIndex ;
      _fuzzyIndex = right._fuzzyIndex ;
      _savedCPUCost = right._savedCPUCost ;
      _isInitialized = right._isInitialized ;
      return *this ;
   }

   // union operation for two keysets
   // PD_TRACE_DECLARE_FUNCTION (SDB_RTNPRED_OPOREQ, "rtnPredicate::operator|=" )
   const rtnPredicate &rtnPredicate::operator|=(const rtnPredicate &right)
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPOREQ ) ;
      startStopKeyUnionBuilder b ;
      RTN_SSKEY_LIST::const_iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::const_iterator j = right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end() )
      {
         INT32 cmp = i->_startKey._bound.woCompare ( j->_startKey._bound,
                                                     FALSE ) ;
         if ( cmp < 0 || (cmp==0 && i->_startKey._inclusive ))
         {
            b.next(*i++) ;
         }
         else
         {
            b.next(*j++) ;
         }
      }
      while ( i!= _startStopKeys.end() )
      {
         b.next(*i++) ;
      }
      while ( j!= right._startStopKeys.end())
      {
         b.next(*j++) ;
      }
      b.done () ;
      finishOperation ( b.result(), right ) ;

      // Accumulate saved cpu costs
      _savedCPUCost += right._savedCPUCost ;

      PD_TRACE_EXIT ( SDB_RTNPRED_OPOREQ ) ;
      return *this ;
   }
   // exclude operation for two keysets
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_OPMINUSEQ, "rtnPredicate::operator-=" )
   const rtnPredicate &rtnPredicate::operator-= ( const rtnPredicate &right )
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPMINUSEQ ) ;
      RTN_SSKEY_LIST newKeySet ;
      RTN_SSKEY_LIST::iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::const_iterator j = right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end())
      {
         // compare start key for i and j
         INT32 cmp = i->_startKey._bound.woCompare ( j->_startKey._bound,
                                                     FALSE ) ;
         if ( cmp < 0 || ( cmp==0 && i->_startKey._inclusive &&
                                    !j->_startKey._inclusive ) )
         {
            // if i.startkey < j.startkey
            // 4 conditions may happen
            // condition 1:
            // i: [start stop]
            // j:                [start stop]
            // condition 2:
            // i: [start stop]
            // j:            [start stop]
            // condition 3:
            // i: [start stop]
            // j:     [start stop]
            // condition 4:
            // i: [start          stop]
            // j:     [start stop]
            // let's compare the stopkey
            INT32 cmp1 = i->_stopKey._bound.woCompare ( j->_startKey._bound,
                                                        FALSE ) ;
            newKeySet.push_back (*i) ;
            if ( cmp1 < 0 )
            {
               // condition 1:
               // i: [start stop]
               // j:                [start stop]
               // if i.stopkey < j.startkey, that means i is all out of j
               ++i ;
            }
            else if ( cmp1 == 0 )
            {
               // condition 2:
               // i: [start stop]
               // j:            [start stop]
               // if i.stopkey == j.stopkey, let's see if we only want to
               // take out inclusive
               if ( newKeySet.back()._stopKey._inclusive &&
                    j->_startKey._inclusive )
               {
                  newKeySet.back()._stopKey._inclusive = FALSE ;
               }
               ++i ;
            }
            else
            {
               // condition 3 and 4
               // i and j are intersects
               // set i's stop key to j's start key
               newKeySet.back()._stopKey = j->_startKey ;
               // revrese inclusive
               newKeySet.back()._stopKey.reverseInclusive() ;
               // check i's stop key and j's stop key
               INT32 cmp2 = i->_stopKey._bound.woCompare (
                               j->_stopKey._bound, FALSE ) ;
               if ( cmp2 < 0 || ( cmp2 == 0 && (
                         !i->_stopKey._inclusive ||
                          j->_stopKey._inclusive ) ) )
               {
                  // condition 3:
                  // i: [start stop]
                  // j:     [start stop]
                  // if i's stop key is less than j's, we are done
                  ++i ;
               }
               else
               {
                  // condition 4:
                  // i: [start          stop]
                  // j:     [start stop]
                  // let's compare the stopkey
                  // otherwise j is breaking up i in the middle
                  // let's update i and use it in the next round, and move to
                  // next j
                  i->_startKey = j->_stopKey ;
                  i->_startKey.reverseInclusive() ;
                  ++j ;
               }
            }
         }
         else
         {
            // this code path is hit when i's start is equal or greater than
            // j's start
            // 3 conditions may happen
            // condition 1:
            // i:               [start stop]
            // j: [start stop]
            // condition 2:
            // i:    [start stop]
            // j: [start stop]
            // condition 3:
            // i:    [start stop]
            // j: [start           stop]
            INT32 cmp1 = i->_startKey._bound.woCompare ( j->_stopKey._bound,
                                                         FALSE ) ;
            if ( cmp1 > 0 || (cmp1 == 0 && (!i->_stopKey._inclusive ||
                                            !j->_stopKey._inclusive )))
            {
               // condition 1:
               // i:               [start stop]
               // j: [start stop]
               // simply skip j
               ++j ;
            }
            else
            {
               INT32 cmp2 = i->_stopKey._bound.woCompare (
                                                        j->_stopKey._bound,
                                                        FALSE ) ;
               if ( cmp2 < 0 ||
                    (cmp2 == 0 && ( !i->_stopKey._inclusive ||
                                     j->_stopKey._inclusive )))
               {
                  // condition 3:
                  // i:    [start stop]
                  // j: [start           stop]
                  // simply skip i ;
                  ++i ;
               }
               else
               {
                  // condition 2:
                  // i:    [start stop]
                  // j: [start stop]
                  i->_startKey = j->_stopKey ;
                  i->_startKey.reverseInclusive() ;
                  ++j ;
               }
            }
         }
      } // while ( i != _startStopKeys.end() && j !=
      // for any leftover i
      while ( i != _startStopKeys.end() )
      {
         newKeySet.push_back (*i) ;
         ++i ;
      }
      finishOperation ( newKeySet, right ) ;
      PD_TRACE_EXIT ( SDB_RTNPRED_OPMINUSEQ ) ;
      return *this ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_RTNPRED, "rtnPredicate::rtnPredicate" )
   rtnPredicate::rtnPredicate ( const BSONElement &e, INT32 opType,
                                BOOLEAN isNot, BOOLEAN mixCmp,
                                INT8 paramIndex, INT8 fuzzyIndex,
                                UINT32 savedCPUCost )
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_RTNPRED ) ;

      INT32 rc = SDB_OK ;

      _isInitialized = FALSE ;

      _equalFlag = -1 ;
      _allEqualFlag = -1 ;

      _evaluated = FALSE ;
      _allRange = TRUE ;
      _selectivity = OPT_PRED_DEFAULT_SELECTIVITY ;

      _paramIndex = paramIndex ;
      _fuzzyIndex = fuzzyIndex ;
      _savedCPUCost = savedCPUCost ;

      if ( e.eoo() )
      {
         rc = _initFullRange() ;
      }
      else
      {
         switch ( opType )
         {
            case BSONObj::opIN :
               rc = _initIN( e, isNot, mixCmp, FALSE ) ;
               break ;
            case BSONObj::opREGEX :
               rc = _initRegEx( e, isNot ) ;
               break ;
            case BSONObj::Equality :
               rc = _initET( e, isNot ) ;
               break ;
            case BSONObj::NE :
               rc = _initNE( e, isNot ) ;
               break ;
            case BSONObj::LT :
               rc = _initLT( e, isNot, FALSE, mixCmp ) ;
               break ;
            case BSONObj::LTE :
               rc = _initLT( e, isNot, TRUE, mixCmp ) ;
               break ;
            case BSONObj::GT :
               rc = _initGT( e, isNot, FALSE, mixCmp ) ;
               break ;
            case BSONObj::GTE :
               rc = _initGT( e, isNot, TRUE, mixCmp ) ;
               break ;
            case BSONObj::opALL :
               rc = _initALL( e, isNot ) ;
               break ;
            case BSONObj::opMOD :
               rc = _initMOD( e, isNot ) ;
               break ;
            case BSONObj::opEXISTS :
               rc = _initEXISTS( e, isNot ) ;
               break ;
            case BSONObj::opISNULL :
               rc = _initISNULL( e, isNot ) ;
               break ;
            default :
               break ;
         }
      }

      if ( SDB_OK == rc && _startStopKeys.empty() )
      {
         rc = _initFullRange () ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to create predicate, rc: %d", rc ) ;

      _isInitialized = TRUE ;

   done :
      PD_TRACE_EXIT( SDB_RTNPRED_RTNPRED ) ;
      return ;
   error :
      _isInitialized = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_REVERSE, "rtnPredicate::reverse" )
   void rtnPredicate::reverse ( rtnPredicate &result ) const
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_REVERSE ) ;
      result._objData = _objData ;
      result._startStopKeys.clear() ;
      for ( RTN_SSKEY_LIST::const_reverse_iterator i = _startStopKeys.rbegin() ;
            i != _startStopKeys.rend();
            ++ i )
      {
         rtnStartStopKey temp ;
         temp._startKey = i->_stopKey ;
         temp._stopKey = i->_startKey ;
         temp._majorType = i->_majorType ;
         result._startStopKeys.push_back ( temp ) ;
      }
      PD_TRACE_EXIT ( SDB_RTNPRED_REVERSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_TOSTRING, "rtnPredicate::toString" )
   ossPoolString rtnPredicate::toString() const
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_TOSTRING ) ;
      StringBuilder buf ;
      buf << "{ " ;
      for ( RTN_SSKEY_LIST::const_iterator i = _startStopKeys.begin() ;
            i != _startStopKeys.end() ;
            ++ i )
      {
         buf << i->toString() << " " ;
      }
      buf << " }" ;
      PD_TRACE_EXIT ( SDB_RTNPRED_TOSTRING ) ;
      return buf.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_BINDPARAM, "rtnPredicate::bindParameters" )
   INT32 rtnPredicate::bindParameters ( rtnParamList &parameters,
                                        BOOLEAN &hasBind,
                                        BOOLEAN markDone )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNPRED_BINDPARAM ) ;

      hasBind = FALSE ;

      if ( _paramIndex >= 0 )
      {
         const RTN_ELEMENT_SET * valueSet = parameters.getValueSet( _paramIndex ) ;
         if ( NULL != valueSet )
         {
            // Bind for value set
            _startStopKeys.clear() ;
            try
            {
               _bindValueSet( valueSet ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_OOM ;
               goto error ;
            }
         }
         else
         {
            // Bind the start and stop key pairs with specified parameters
            BSONElement param = parameters.getParam( _paramIndex ) ;
            for ( RTN_SSKEY_LIST::iterator iter = _startStopKeys.begin() ;
                  iter != _startStopKeys.end() ;
                  ++ iter )
            {
               if ( iter->_startKey._parameterized )
               {
                  iter->_startKey._bound = param ;
                  if ( _fuzzyIndex >= 0 )
                  {
                     iter->_startKey._inclusive =
                           parameters.getParam( _fuzzyIndex ).booleanSafe() ;
                  }
               }
               if ( iter->_stopKey._parameterized )
               {
                  iter->_stopKey._bound = param ;
                  if ( _fuzzyIndex >= 0 )
                  {
                     iter->_stopKey._inclusive =
                           parameters.getParam( _fuzzyIndex ).booleanSafe() ;
                  }
               }
            }
         }

         // Mark the parameter matching is done by predicates
         if ( markDone )
         {
            parameters.setDoneByPred( _paramIndex ) ;
         }
         hasBind = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNPRED_BINDPARAM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITIN, "rtnPredicate::_initIN" )
   INT32 rtnPredicate::_initIN ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN mixCmp, BOOLEAN expandRegex )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITIN ) ;

      PD_CHECK( Array == e.type(), SDB_INVALIDARG, error, PDERROR,
                "Invalid $in expression" ) ;

      if ( !isNot )
      {
         // for IN statement without isNot or if the element type is array
         // and we want equality match {c1:{$et:[1,2,3]}}
         RTN_ELEMENT_SET valSet ;

         RTN_PREDICATE_LIST regexes ;
         BSONObjIterator i ( e.embeddedObject() ) ;

         try
         {
            // if e is an empty array. just add it to vals.(this will be add to
            // the _startStopKeys
            if ( !i.more() )
            {
               valSet.insert( e ) ;
            }
            // for each element in the array
            while ( i.more() )
            {
               BSONElement ie = i.next() ;

               // make sure we don't have embedded object with ELEM_MATCH operation
               if ( ie.type() == Object &&
                    ie.embeddedObject().firstElement().getGtLtOp() ==
                          BSONObj::opELEM_MATCH )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "$eleMatch is not allowed within $in" ) ;
                  goto done ;
               }

               // for regular expression match, let's create a new rtnPredicate
               if ( expandRegex &&
                    ( ie.type() == RegEx ||
                      ( e.type() == Object &&
                        !e.embeddedObject()[ "$regex" ].eoo() ) ) )
               {
                  regexes.push_back ( rtnPredicate ( ie, BSONObj::opREGEX, FALSE,
                                                     mixCmp ) ) ;
                  PD_CHECK( regexes.back().isInit(),
                            SDB_INVALIDARG, error, PDERROR,
                            "Failed to create regex predicate" ) ;
               }

               valSet.insert( ie ) ;
            }

            // after going through all elements, let's push all in $in into
            // start/stopkey list
            for ( RTN_ELEMENT_SET::iterator iterSet = valSet.begin() ;
                  iterSet != valSet.end() ;
                  ++ iterSet )
            {
               // we don't need to set parameterized flag, it will rebuild
               // during binding phase
               _startStopKeys.push_back( rtnStartStopKey( *iterSet ) ) ;
            }

            // and then union with regular expression
            for ( RTN_PREDICATE_LIST::const_iterator i = regexes.begin();
                  i!=regexes.end(); i++ )
            {
               *this |= *i ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITREGEX, "rtnPredicate::_initRegEx" )
   INT32 rtnPredicate::_initRegEx ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITREGEX ) ;

      // 1. RegEx { $regex:'xx', $options:'xx }
      // 2. Object { $regex: 'xx' }
      // 3. Object { $options: 'xx', $regex: 'xx' }
      PD_CHECK( e.type() == RegEx ||
                ( e.type() == Object && !e.embeddedObject()[ "$regex" ].eoo() ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid regular expression operator" ) ;

      if ( !isNot )
      {
         try
         {
            // let's try to generate regex string if it's simple
            string r = simpleRegex( e ) ;

            if ( r.size() )
            {
               // yes we can have a simple regex
               _startStopKeys.push_back( rtnStartStopKey() ) ;
               rtnStartStopKey &keyPair = _startStopKeys.back() ;
               keyPair._startKey._bound = addObj( BSON( "" << r ) ).firstElement() ;
               keyPair._startKey._inclusive = TRUE ;
               keyPair._stopKey._bound =
                     addObj( BSON( "" << simpleRegexEnd( r ) ) ).firstElement() ;
               keyPair._stopKey._inclusive = FALSE ;
               keyPair._majorType = String ;
            }
            else
            {
               // RegEx is restricted to String and Symbol
               // Note: String and Symbol are the same canonical types currently
               _initTypeRange( String, TRUE ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITREGEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITET, "rtnPredicate::_initET" )
   INT32 rtnPredicate::_initET ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITET ) ;

      if ( isNot )
      {
         // $not:$et -> $ne
         rc = _initNE( e, FALSE ) ;
      }
      else if ( Array == e.type() )
      {
         // do nothing
         // Note: $et with array could generate predicate if the array is
         // expanded recursively, but now it is only expanded for one level
         // rc = _initALL( e, FALSE ) ;
      }
      else
      {
         try
         {
            _startStopKeys.push_back ( rtnStartStopKey( e ) ) ;
            if ( -1 != _paramIndex )
            {
               rtnStartStopKey &keyPair = _startStopKeys.back() ;
               keyPair._startKey._parameterized = TRUE ;
               keyPair._stopKey._parameterized = TRUE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
         }
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITET, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITNE, "rtnPredicate::_initNE" )
   INT32 rtnPredicate::_initNE ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITNE ) ;

      if ( e.type() == Array )
      {
         // Operator with array should not generate predicate
         goto done ;
      }

      if ( isNot )
      {
         // $not:ne -> $et
         rc = _initET( e, FALSE ) ;
      }
      else
      {
         // [ $minKey, e ) and ( e, $maxKey ]
         _initLT( e, FALSE, FALSE, TRUE ) ;
         _initGT( e, FALSE, FALSE, TRUE ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITNE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITLT, "rtnPredicate::_initLT" )
   INT32 rtnPredicate::_initLT ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN inclusive, BOOLEAN mixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITLT ) ;

      if ( e.type() == Array )
      {
         // Operator with array should not generate predicate
         goto done ;
      }

      if ( isNot )
      {
         // $not:$lte -> $gt
         // $not:$lt -> $gte
         rc = _initGT( e, FALSE, !inclusive, mixCmp ) ;
      }
      else
      {
         BSONElement startKey ;
         BOOLEAN startInclusive = TRUE ;

         if ( mixCmp || e.type() == MaxKey )
         {
            if ( e.canonicalType() <=
                 staticUndefined.firstElement().canonicalType() )
            {
               // Stop key is smaller than $undefined
               startKey = minKey.firstElement() ;
               startInclusive = TRUE ;
            }
            else
            {
               // Add a special range [ $minKey, $undefined )
               _initMinRange( TRUE ) ;

               // We start from $undefined to exclude undefined fields
               startKey = staticUndefined.firstElement() ;
               startInclusive = FALSE ;
            }
         }
         else
         {
            startKey = rtnKeyGetMinForCmp( e.type(), FALSE ).firstElement() ;

            // If the start key is not the same canonical type as given key,
            // it should not be included, otherwise it should be included
            startInclusive = ( startKey.canonicalType() == e.canonicalType() ) ;
         }

         try
         {
            _startStopKeys.push_back( rtnStartStopKey() ) ;
            rtnStartStopKey &keyPair = _startStopKeys.back() ;

            keyPair._startKey._bound = startKey ;
            keyPair._startKey._inclusive = startInclusive ;

            keyPair._stopKey._bound = e ;
            keyPair._stopKey._inclusive = inclusive ;

            keyPair._majorType = e.type() ;

            if ( -1 != _paramIndex )
            {
               keyPair._stopKey._parameterized = TRUE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITLT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITGT, "rtnPredicate::_initGT" )
   INT32 rtnPredicate::_initGT ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN inclusive, BOOLEAN mixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITGT ) ;

      if ( e.type() == Array )
      {
         // Operator with array should not generate predicate
         goto done ;
      }

      if ( isNot )
      {
         // $not:$gte -> $lt
         // $not:$gt -> $lte
         rc = _initLT( e, FALSE, !inclusive, mixCmp ) ;
      }
      else
      {
         BOOLEAN minRangeAdded = FALSE ;

         if ( e.canonicalType() <
              staticUndefined.firstElement().canonicalType() )
         {
            // Add a special range [ $minKey, $undefined )
            _initMinRange( inclusive ) ;
            minRangeAdded = TRUE ;
         }

         try
         {
            _startStopKeys.push_back( rtnStartStopKey() ) ;
            rtnStartStopKey &keyPair = _startStopKeys.back() ;

            if ( minRangeAdded )
            {
               // Then we start from $undefined
               keyPair._startKey._bound = staticUndefined.firstElement() ;
               keyPair._startKey._inclusive = FALSE ;
            }
            else
            {
               keyPair._startKey._bound = e ;
               keyPair._startKey._inclusive = inclusive ;
            }

            BSONElement stopKey =
                  rtnKeyGetMaxForCmp( e.type(), mixCmp ).firstElement() ;
            // 1. If it is mixCmp, stop key is $maxKey which should be included
            // 2. If it is $gt:$minKey or $gte:$minKey, stop key is also $maxKey
            //    which should be included
            // 3. If the stop key is not the same canonical type as given key,
            //    it should not be included, otherwise it should be included
            BOOLEAN stopInclusive =
                  ( ( mixCmp || e.type() == MinKey ) ? TRUE :
                    ( stopKey.canonicalType() == e.canonicalType() ) ) ;
            keyPair._stopKey._bound = stopKey ;
            keyPair._stopKey._inclusive = stopInclusive ;

            keyPair._majorType = e.type() ;

            if ( -1 != _paramIndex )
            {
               keyPair._startKey._parameterized = TRUE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITGT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITALL, "rtnPredicate::_initALL" )
   INT32 rtnPredicate::_initALL ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITALL ) ;

      PD_CHECK( e.type() == Array, SDB_INVALIDARG, error, PDERROR,
                "Must be array type for opALL" ) ;

      if ( !isNot )
      {
         // For index, get one in the array to do the index match
         BSONObjIterator i ( e.embeddedObject()) ;
         while ( i.more() )
         {
            BSONElement x = i.next() ;
            if ( x.type() == Object &&
                 x.embeddedObject().firstElement().getGtLtOp() ==
                 BSONObj::opELEM_MATCH )
            {
               continue ;
            }

            try
            {
               _startStopKeys.push_back( rtnStartStopKey( x ) ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            break ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITALL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITMOD, "rtnPredicate::_initMOD" )
   INT32 rtnPredicate::_initMOD ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITMOD ) ;

      if ( !isNot )
      {
         // [ minDecimal, maxDecimal ]
         rc = _initTypeRange( NumberDecimal, TRUE ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITMOD, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITEXISTS, "rtnPredicate::_initEXISTS" )
   INT32 rtnPredicate::_initEXISTS ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITEXISTS ) ;

      BOOLEAN existsSpec = e.trueValue() ;

      if ( isNot )
      {
         existsSpec = !existsSpec ;
      }

      if ( !existsSpec )
      {
         try
         {
            // [ $undefined, $undefined ]
            _startStopKeys.push_back (
                  rtnStartStopKey( staticUndefined.firstElement() ) ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         // generate [ $minKey, undefined )
         rc = _initLT( staticUndefined.firstElement(), FALSE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize with "
                      "[ $minKey, undefined ), rc: %d", rc ) ;

         // generate ( undefined, $maxKey ]
         rc = _initGT( staticUndefined.firstElement(), FALSE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize with "
                      "( undefined, $maxKey ], rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNPRED__INITEXISTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITISNULL, "rtnPredicate::_initISNULL" )
   INT32 rtnPredicate::_initISNULL ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITISNULL ) ;

      BOOLEAN existsSpec = !e.trueValue() ;

      if ( isNot )
      {
         existsSpec = !existsSpec ;
      }

      if ( !existsSpec )
      {
         try
         {
            // [ $undefined, $undefined ]
            _startStopKeys.push_back (
                  rtnStartStopKey( staticUndefined.firstElement() ) ) ;

            // [ null, null ]
            _startStopKeys.push_back (
                  rtnStartStopKey( staticNull.firstElement() ) ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         // generate [ $minKey, undefined )
         rc = _initLT( staticUndefined.firstElement(), FALSE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize with "
                      "[ $minKey, undefined ), rc: %d", rc ) ;

         // generate ( null, $maxKey ]
         rc = _initGT( staticNull.firstElement(), FALSE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize with "
                      "( null, $maxKey ], rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNPRED__INITISNULL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITFULLRANGE, "rtnPredicate::_initFullRange" )
   INT32 rtnPredicate::_initFullRange ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITFULLRANGE ) ;

      try
      {
         _startStopKeys.push_back ( rtnStartStopKey() ) ;
         rtnStartStopKey &keyPair = _startStopKeys.back() ;

         keyPair._startKey._bound = minKey.firstElement() ;
         keyPair._startKey._inclusive = TRUE ;

         keyPair._stopKey._bound = maxKey.firstElement() ;
         keyPair._stopKey._inclusive = TRUE ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITFULLRANGE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITTYPERANGE, "rtnPredicate::_initTypeRange" )
   INT32 rtnPredicate::_initTypeRange ( BSONType type, BOOLEAN forCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITTYPERANGE ) ;

      try
      {
         _startStopKeys.push_back ( rtnStartStopKey() ) ;
         rtnStartStopKey &keyPair = _startStopKeys.back() ;

         BSONElement startKey = forCmp ?
                                rtnKeyGetMinForCmp( type, FALSE ).firstElement() :
                                rtnKeyGetMinType( type ).firstElement() ;
         BOOLEAN startInclusive =
               ( getBSONCanonicalType( type ) == startKey.canonicalType() ) ;
         keyPair._startKey._bound = startKey ;
         keyPair._startKey._inclusive = startInclusive ;

         BSONElement stopKey = forCmp ?
                               rtnKeyGetMaxForCmp( type, FALSE ).firstElement() :
                               rtnKeyGetMaxType( type ).firstElement() ;
         BOOLEAN stopInclusive =
               ( getBSONCanonicalType( type ) == stopKey.canonicalType() ) ;
         keyPair._stopKey._bound = stopKey ;
         keyPair._stopKey._inclusive = stopInclusive ;

         keyPair._majorType = type ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITTYPERANGE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITMINRANGE, "rtnPredicate::_initMinRange" )
   INT32 rtnPredicate::_initMinRange ( BOOLEAN startIncluded )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITMINRANGE ) ;

      try
      {
         _startStopKeys.push_back ( rtnStartStopKey() ) ;
         rtnStartStopKey &keyPair = _startStopKeys.back() ;

         keyPair._startKey._bound = minKey.firstElement() ;
         keyPair._startKey._inclusive = startIncluded ;

         keyPair._stopKey._bound = staticUndefined.firstElement() ;
         keyPair._stopKey._inclusive = FALSE ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITMINRANGE, rc ) ;
      return rc ;
   }

   ossPoolString _rtnPredicateSet::toString() const
   {
      StringBuilder buf ;
      buf << "[ " ;
      RTN_PREDICATE_MAP::const_iterator it = _predicates.begin() ;
      while ( it != _predicates.end() )
      {
         buf << it->first.c_str() << ":" << it->second.toString() << " " ;
         ++it ;
      }
      buf << " ]" ;
      return buf.poolStr() ;
   }

   BSONObj _rtnPredicateSet::toBson() const
   {
      BSONObjBuilder builder ;
      RTN_PREDICATE_MAP::const_iterator it = _predicates.begin() ;
      for ( ; it != _predicates.end(); ++it )
      {
         BSONArrayBuilder sub( builder.subarrayStart( it->first.c_str() ) ) ;
         const RTN_SSKEY_LIST &range = it->second._startStopKeys ;
         RTN_SSKEY_LIST::const_iterator ssItr = range.begin() ;
         for ( ; ssItr != range.end(); ++ssItr )
         {
            sub << ssItr->toBson() ;
         }
         sub.doneFast() ;
      }
      return builder.obj() ;
   }

   static const rtnPredicate &_rtnGetGenericPredicate ()
   {
      static rtnPredicate genericPredicate( BSONObj().firstElement(), 0,
                                            FALSE, TRUE ) ;
      return genericPredicate ;
   }

   static RTN_PREDICATE_LIST _rtnGetGenericPredicateList ()
   {
      RTN_PREDICATE_LIST predicateList ;
      predicateList.push_back( _rtnGetGenericPredicate() ) ;
      return predicateList ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED_PRED, "_rtnPredicateSet::predicate" )
   const rtnPredicate &_rtnPredicateSet::predicate (const CHAR *fieldName) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPRED_PRED ) ;
      const rtnPredicate *pRet = NULL ;
      RTN_PREDICATE_MAP::const_iterator f = _predicates.find(fieldName);
      if ( _predicates.end() == f )
      {
         // we assign rtnPredicate object to a static pointer
         // this memory is not released until process terminate
         pRet = &( _rtnGetGenericPredicate() ) ;
      }
      else
      {
         pRet = &(f->second) ;
      }
      PD_TRACE_EXIT ( SDB__RTNPRED_PRED ) ;
      return *pRet ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED_PARAMPRED, "_rtnPredicateSet::paramPredicate" )
   const RTN_PREDICATE_LIST &_rtnPredicateSet::paramPredicate (const CHAR *fieldName) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPRED_PARAMPRED ) ;

      static RTN_PREDICATE_LIST genericPredList = _rtnGetGenericPredicateList() ;

      const RTN_PREDICATE_LIST *pRet = NULL ;
      RTN_PARAM_PREDICATE_MAP::const_iterator f = _paramPredicates.find( fieldName );
      if ( _paramPredicates.end() == f )
      {
         // we assign rtnPredicate object to a static pointer
         // this memory is not released until process terminate
         pRet = &genericPredList ;
      }
      else
      {
         pRet = &(f->second) ;
      }
      PD_TRACE_EXIT ( SDB__RTNPRED_PARAMPRED ) ;
      return *pRet ;
   }

   INT32 _rtnPredicateSet::addPredicate ( const CHAR * fieldName,
                                          const BSONElement & e,
                                          INT32 opType,
                                          BOOLEAN isNot,
                                          BOOLEAN mixCmp )
   {
      return addParamPredicate( fieldName, e, opType, isNot, mixCmp, FALSE,
                                -1, -1, 0 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDSET_ADDPARAMPRED, "_rtnPredicateSet::addParamPredicate" )
   INT32 _rtnPredicateSet::addParamPredicate ( const CHAR * fieldName,
                                               const BSONElement & e,
                                               INT32 opType,
                                               BOOLEAN isNot,
                                               BOOLEAN mixCmp,
                                               BOOLEAN addToParam,
                                               INT8 paramIndex,
                                               INT8 fuzzyIndex,
                                               UINT32 savedCPUCost )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPREDSET_ADDPARAMPRED ) ;

      std::pair< RTN_PREDICATE_MAP::iterator, BOOLEAN > ret ;
      rtnPredicate pred ( e, opType, isNot, mixCmp, paramIndex, fuzzyIndex,
                          savedCPUCost ) ;
      PD_CHECK( pred.isInit(), SDB_INVALIDARG, error, PDERROR,
                "Failed to init predicate %s: %s", fieldName,
                e.toString().c_str()) ;

      try
      {
         ret = _predicates.insert( make_pair( fieldName, pred ) ) ;
         if ( !(ret.second) )
         {
            ret.first->second &= pred ;
         }

         if ( addToParam )
         {
            RTN_PARAM_PREDICATE_MAP::iterator iter ;
            iter = _paramPredicates.find( fieldName ) ;
            if ( iter == _paramPredicates.end() )
            {
               RTN_PREDICATE_LIST &predList = _paramPredicates[ fieldName ] ;
               predList.push_back( pred ) ;
            }
            else
            {
               iter->second.push_back( pred ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPREDSET_ADDPARAMPRED, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDSET_ADDANDPREDSET, "_rtnPredicateSet::addAndPredicates" )
   INT32 _rtnPredicateSet::addAndPredicates( const _rtnPredicateSet &otherSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPREDSET_ADDANDPREDSET ) ;

      try
      {
         // merge predicates
         for ( RTN_PREDICATE_MAP_CIT iter = otherSet._predicates.begin() ;
               iter != otherSet._predicates.end() ;
               ++ iter )
         {
            pair< RTN_PREDICATE_MAP_IT, BOOLEAN > ret ;
            ret = _predicates.insert( make_pair( iter->first, iter->second ) ) ;
            if ( !ret.second )
            {
               ret.first->second &= iter->second ;
            }
         }

         // merge parameter predicates
         for ( RTN_PARAM_PREDICATE_MAP_CIT otherIter =
                                 otherSet._paramPredicates.begin() ;
               otherIter != otherSet._paramPredicates.end() ;
               ++ otherIter )
         {
            for ( RTN_PREDICATE_LIST_CIT iter = otherIter->second.begin() ;
                  iter != otherIter->second.end() ;
                  ++ iter )
            {
               _paramPredicates[ otherIter->first ].push_back( *iter ) ;
            }
         }
      }
      catch( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add predicates by $and, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNPREDSET_ADDANDPREDSET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDSET_ADDORPREDSET, "_rtnPredicateSet::addOrPredicates" )
   INT32 _rtnPredicateSet::addOrPredicates( const _rtnPredicateSet &otherSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPREDSET_ADDORPREDSET ) ;

      // not support merge predicates with parameters yet
      PD_CHECK( 0 == getParamSize(), SDB_INVALIDARG, error, PDERROR,
                "Failed to merge predicates by $or, "
                "current set has parameter fields" ) ;
      PD_CHECK( 0 == otherSet.getParamSize(), SDB_INVALIDARG, error, PDERROR,
                "Failed to merge predicates by $or, "
                "other set has parameter fields" ) ;

      if ( 0 == getSize() )
      {
         // current is empty, do nothing
      }
      else if ( 1 != getSize() || 1 != otherSet.getSize() )
      {
         // can not generate predicates
         // downgrade to empty
         clear() ;
      }
      else
      {
         RTN_PREDICATE_MAP_IT curIter = _predicates.begin() ;
         RTN_PREDICATE_MAP_CIT otherIter = otherSet._predicates.begin() ;

         SDB_ASSERT( curIter != _predicates.end(),
                     "predicate from current set is invalid" ) ;
         SDB_ASSERT( otherIter != otherSet._predicates.end(),
                     "predicate from other set is invalid" ) ;

         if ( curIter->first != otherIter->first )
         {
            // have different name
            // downgrade to empty
            clear() ;
         }
         else
         {
            // same fields, we can merge predicates
            curIter->second |= otherIter->second ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNPREDSET_ADDORPREDSET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   _rtnPredicateList::_rtnPredicateList ()
   {
      _initialized = FALSE ;
      _fixedPredList = FALSE ;
      _direction = 1 ;
   }

   _rtnPredicateList::~_rtnPredicateList ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_CLEAR, "_rtnPredicateList::clear" )
   void _rtnPredicateList::clear ()
   {
      _predicates.clear() ;
      _keyPattern = BSONObj() ;
      _direction = 1 ;
      _initialized = FALSE ;
      _fixedPredList = FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_INIT, "_rtnPredicateList::initialize" )
   INT32 _rtnPredicateList::initialize ( const rtnPredicateSet &predSet,
                                         const BSONObj &keyPattern,
                                         INT32 direction,
                                         UINT32 &addedLevel )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPREDLIST_INIT ) ;

      SDB_ASSERT( !_initialized, "Should not be initialized" ) ;

      // Set direction and key pattern first
      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;

      addedLevel = 0 ;
      BSONObjIterator i( _keyPattern ) ;

      /// make sure space
      try
      {
         _predicates.reserve( keyPattern.nFields() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      while ( i.more() )
      {
         BSONElement e = i.next() ;
         const rtnPredicate &pred = predSet.predicate ( e.fieldName() ) ;
         if ( pred.isEmpty() )
         {
            rc = _addEmptyPredicate() ;
            if ( rc )
            {
               goto error ;
            }
            break ;
         }
         rc = _addPredicate( e, direction, pred ) ;
         if ( rc )
         {
            goto error ;
         }
         addedLevel ++ ;
      }

      _fixedPredList = TRUE ;
      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_INIT_PARAM, "_rtnPredicateList::initialize" )
   INT32 _rtnPredicateList::initialize ( const rtnPredicateSet &predSet,
                                         const BSONObj &keyPattern,
                                         INT32 direction,
                                         rtnParamList &parameters,
                                         RTN_PARAM_PREDICATE_LIST &paramPredList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_INIT_PARAM ) ;

      SDB_ASSERT( !_initialized, "Should not be initialized" ) ;

      BOOLEAN containParamPred = FALSE ;
      BOOLEAN needAddPred = TRUE ;

      // Set direction and key pattern first
      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;
      BSONObjIterator i( _keyPattern ) ;

      /// make sure space
      try
      {
         _predicates.reserve( keyPattern.nFields() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      while ( i.more() )
      {
         BSONElement e = i.next() ;
         const RTN_PREDICATE_LIST &predicates =
                                 predSet.paramPredicate( e.fieldName() ) ;
         if ( predicates.empty() )
         {
            rc = _addEmptyPredicate() ;
            if ( rc )
            {
               goto error ;
            }
            continue ;
         }
         else
         {
            // Construct the parameterized predicates
            // 1. merge non-parameterized predicates into one predicate
            // 2. save the parameterized predicates in the list for the similar
            //    queries in the future
            rtnPredicate pred, nonParamPred ;
            BOOLEAN containNonParamPred = FALSE ;
            BOOLEAN nonParamPredEmpty = FALSE ;
            BOOLEAN paramPredEmpty = FALSE ;
            BOOLEAN hasBind = FALSE ;

            RTN_PREDICATE_LIST paramList ;
            RTN_PREDICATE_LIST::const_iterator iter = predicates.begin () ;
            if ( iter != predicates.end() )
            {
               pred = (*iter) ;
               rc = pred.bindParameters( parameters, hasBind ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( !hasBind )
               {
                  nonParamPred = pred ;
                  containNonParamPred = TRUE ;
               }
               else
               {
                  try
                  {
                     paramList.push_back( pred ) ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                     rc = SDB_OOM ;
                     goto error ;
                  }
               }
               ++ iter ;
               for ( ;
                     iter != predicates.end() && !nonParamPredEmpty ;
                     ++ iter )
               {
                  rtnPredicate currentPred = (*iter) ;
                  rc = currentPred.bindParameters( parameters, hasBind ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
                  if ( !hasBind )
                  {
                     // Merge non-parameterized predicates
                     if ( containNonParamPred )
                     {
                        nonParamPred &= currentPred ;
                        if ( nonParamPred.isEmpty() )
                        {
                           nonParamPredEmpty = TRUE ;
                        }
                     }
                     else
                     {
                        nonParamPred = currentPred ;
                        containNonParamPred = TRUE ;
                     }
                  }
                  else
                  {
                     // The predicate is parameterized, save for the future
                     // queries
                     try
                     {
                        paramList.push_back( currentPred ) ;
                     }
                     catch( std::exception &e )
                     {
                        PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                        rc = SDB_OOM ;
                        goto error ;
                     }
                  }

                  // Stop merging predicates when it is already empty
                  if ( !paramPredEmpty )
                  {
                     pred &= currentPred ;
                     if ( pred.isEmpty() )
                     {
                        paramPredEmpty = TRUE ;
                     }
                  }
               }

               try
               {
                  if ( containNonParamPred )
                  {
                     if ( nonParamPredEmpty )
                     {
                        // Non-parameterized predicates are already empty
                        // No need to consider parameterized predicates
                        paramList.clear() ;
                        paramList.push_back( nonParamPred ) ;
                     }
                     else
                     {
                        if ( !paramList.empty() )
                        {
                           containParamPred = TRUE ;
                        }
                        paramList.push_back( nonParamPred ) ;
                     }
                  }
                  else if ( !paramList.empty() )
                  {
                     containParamPred = TRUE ;
                  }

                  paramPredList.push_back( paramList ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
            }

            if ( needAddPred && pred.isEmpty() )
            {
               // The current generated predicate is empty
               // No need for generate predicate list, but still need to
               // generate parameterized predicate list if the empty predicate
               // is not generated by non-parameterized predicates
               rc = _addEmptyPredicate() ;
               if ( rc )
               {
                  goto error ;
               }
               needAddPred = FALSE ;
               if ( nonParamPredEmpty )
               {
                  break ;
               }
            }
            else if ( needAddPred )
            {
               rc = _addPredicate( e, direction, pred ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
      }

      // Check if contain parameterized predicates
      // If not, mark the predicate list is fixed
      if ( !containParamPred )
      {
         _fixedPredList = TRUE ;
      }
      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT_PARAM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_INIT_PARAMLIST, "_rtnPredicateList::initialize" )
   INT32 _rtnPredicateList::initialize ( const RTN_PARAM_PREDICATE_LIST &paramPredList,
                                         const BSONObj &keyPattern,
                                         INT32 direction,
                                         rtnParamList &parameters )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_INIT_PARAMLIST ) ;

      // Set direction and key pattern first
      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;

      RTN_PARAM_PREDICATE_LIST::const_iterator iter = paramPredList.begin() ;
      BSONObjIterator i( _keyPattern ) ;

      /// make sure space
      try
      {
         _predicates.reserve( keyPattern.nFields() ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( iter == paramPredList.end() )
         {
            rc = _addEmptyPredicate() ;
            if ( rc )
            {
               goto error ;
            }
            break ;
         }
         else
         {
            rtnPredicate pred ;
            BOOLEAN hasBind = FALSE ;

            RTN_PREDICATE_LIST::const_iterator predIter = iter->begin () ;
            if ( predIter != iter->end() )
            {
               // Build the predicates with parameters
               pred = (*predIter) ;
               rc = pred.bindParameters( parameters, hasBind ) ;
               if ( rc )
               {
                  goto error ;
               }
               ++ predIter ;
               for ( ; predIter != iter->end() ; ++ predIter )
               {
                  rtnPredicate currentPred = (*predIter) ;
                  rc = currentPred.bindParameters( parameters, hasBind ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
                  pred &= currentPred ;
                  if ( pred.isEmpty() )
                  {
                     // The predicate is empty already
                     // No need to process other parameters
                     break ;
                  }
               }
            }

            if ( pred.isEmpty() )
            {
               rc = _addEmptyPredicate() ;
               if ( rc )
               {
                  goto error ;
               }
               break ;
            }
            else
            {
               rc = _addPredicate( e, direction, pred ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
         ++ iter ;
      }

      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT_PARAMLIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST__ADDPRED, "_rtnPredicateList::_addPredicate" )
   INT32 _rtnPredicateList::_addPredicate ( const BSONElement &e,
                                            INT32 direction,
                                            const rtnPredicate &pred )
   {
      PD_TRACE_ENTRY( SDB__RTNPREDLIST__ADDPRED ) ;

      INT32 rc = SDB_OK ;
      // num is the number defined in index {c1:1}
      INT32 num = (INT32)e.number() ;

      // if index is defined as forward and direction is forward, then we
      // have forward
      // if index is defined as forward and direction is backward, then we
      // scan backward
      // if index is defined as backward and direction is forward, then we
      // scan backward
      // if index is defined as backward and direction is backward, then we
      // scan forward
      BOOLEAN forward = ((num>=0?1:-1)*(direction>=0?1:-1)>0) ;

      try
      {
         if ( forward )
         {
            _predicates.push_back ( pred ) ;
         }
         else
         {
            // if we want to scan backward, we need to reverse the predicate
            // i.e. startKey to stopKey, stopKey to startKey
            // first we push an empty predicate into list, then reverse the
            // existing predicate to replace it
            _predicates.push_back ( rtnPredicate () ) ;
            pred.reverse ( _predicates.back() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      PD_TRACE_EXITRC( SDB__RTNPREDLIST__ADDPRED, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST__ADDEMPPRED, "_rtnPredicateList::_addEmptyPredicate" )
   INT32 _rtnPredicateList::_addEmptyPredicate ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNPREDLIST__ADDEMPPRED ) ;

      static rtnStartStopKey s_emptyStartStopKey( 0 ) ;

      try
      {
         rtnPredicate emptyPred ;
         emptyPred._startStopKeys.clear() ;
         emptyPred._startStopKeys.push_back( s_emptyStartStopKey ) ;

         _predicates.push_back( emptyPred ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }

      PD_TRACE_EXITRC( SDB__RTNPREDLIST__ADDEMPPRED, rc ) ;
      return rc ;
   }

   // get the start key of first start/stopkey in each predicate column
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_STARTKEY, "_rtnPredicateList::startKey" )
   BSONObj _rtnPredicateList::startKey() const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_STARTKEY ) ;
      BSONObjBuilder b ;
      for ( RTN_PREDICATE_LIST::const_iterator i = _predicates.begin() ;
            i != _predicates.end(); i++ )
      {
         const rtnStartStopKey &key = i->_startStopKeys.front() ;
         b.appendAs ( key._startKey._bound, "" ) ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_STARTKEY ) ;
      return b.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_ENDKEY, "_rtnPredicateList::endKey" )
   BSONObj _rtnPredicateList::endKey() const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_ENDKEY ) ;
      BSONObjBuilder b ;
      for ( RTN_PREDICATE_LIST::const_iterator i = _predicates.begin() ;
            i != _predicates.end(); i++ )
      {
         const rtnStartStopKey &key = i->_startStopKeys.back() ;
         b.appendAs ( key._stopKey._bound, "" ) ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_ENDKEY ) ;
      return b.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_OBJ, "_rtnPredicateList::obj" )
   BSONObj _rtnPredicateList::obj( BOOLEAN needAbbrev ) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_OBJ ) ;
      BSONObjBuilder b ;
      // always client readable for predicate bounds
      BSONObjBuilderOption option( true, needAbbrev ) ;
      for ( INT32 i = 0; i<(INT32)_predicates.size(); i++ )
      {
         BSONArrayBuilder a ( b.subarrayStart("") ) ;
         for ( RTN_SSKEY_LIST::const_iterator j =
                                    _predicates[i]._startStopKeys.begin() ;
               j != _predicates[i]._startStopKeys.end() ;
               ++ j )
         {
            BSONArrayBuilder subArray( a.subarrayStart() ) ;
            subArray.appendEx( j->_startKey._bound, option ) ;
            subArray.appendEx( j->_stopKey._bound, option ) ;
            subArray.doneFast() ;
         }
         a.doneFast() ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_OBJ ) ;
      return b.obj() ;
   }
   string _rtnPredicateList::toString() const
   {
      return obj( FALSE ).toString(false, false) ;
   }

   BSONObj _rtnPredicateList::getBound( BOOLEAN needAbbrev ) const
   {
      BSONObjBuilder builder ;
      BSONObj predicate = obj( needAbbrev ) ;
      BSONObjIterator i( predicate ) ;
      BSONObjIterator j( _keyPattern ) ;
      while ( i.more() && j.more() )
      {
         BSONElement field = j.next() ;
         BSONElement bound = i.next() ;
         builder.appendAs( bound, field.fieldName() ) ;
      }
      return builder.obj() ;
   }

   BOOLEAN _rtnPredicateList::isAllRange() const
   {
      for ( RTN_PREDICATE_LIST::const_iterator iter = _predicates.begin() ;
            iter != _predicates.end() ;
            ++ iter )
      {
         if ( !iter->isGeneric() )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   // whether an element matches the i'th column
   // even result means the element is contained within a valid range
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_MATLOWELE, "_rtnPredicateList::matchingLowElement" )
   INT32 _rtnPredicateList::matchingLowElement ( const BSONElement &e, INT32 i,
                                                BOOLEAN direction,
                                                BOOLEAN &lowEquality ) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_MATLOWELE ) ;
      lowEquality = FALSE ;
      INT32 l = -1 ;
      // since each element got start and stop key, so we use *2 to double the
      // scan range
      INT32 h = _predicates[i]._startStopKeys.size() * 2 ;
      INT32 m = 0 ;
      // loop until l=h-1, in one start/stop key scenario, we start from l=-1,
      // h=2
      // in matching case, l should be 0,2,4,6,8, etc... which indicate the
      // matching range between l and h
      // when not match, l should be 1,3,5,7, etc... which indicate the
      // unmatching range between previous stopKey and next startKey
      while ( l + 1 < h )
      {
         m = ( l + h ) / 2 ;
         BSONElement toCmp ;
         BOOLEAN toCmpInclusive ;
         const rtnStartStopKey &startstopkey=_predicates[i]._startStopKeys[m/2];
         // even number means startKey
         if ( 0 == m%2 )
         {
            toCmp = startstopkey._startKey._bound ;
            toCmpInclusive = startstopkey._startKey._inclusive ;
         }
         else
         {
            toCmp = startstopkey._stopKey._bound ;
            toCmpInclusive = startstopkey._stopKey._inclusive ;
         }
         // compare the input and key
         INT32 result = toCmp.woCompare ( e, FALSE ) ;
         // for backward scan we reverse the result
         if ( !direction )
            result = -result ;
         // key smaller than input, scan up
         if ( result < 0 )
            l = m ;
         // key larger than iniput, scan down
         else if ( result > 0 )
            h = m ;
         // if we get exact match
         else
         {
            // match the start key
            if ( 0 == m%2 )
               lowEquality = TRUE ;
            // if we got startKey match and it's inclusive, then we are okay
            // (return even number, match case)
            // but if it's not inclusive, we should return the one before it (
            // return odd number, means out of range)
            // if we got stopKey match and it's inclusive, we should return the
            // one before it (return even number, means match)
            // otherwise we dont' change ( return odd number, out of range )
            INT32 ret = m ;
            if ((0 == m%2 && !toCmpInclusive) ||
                (1 == m%2 && toCmpInclusive))
            {
               --ret ;
            }
            PD_TRACE1 ( SDB__RTNPREDLIST_MATLOWELE, PD_PACK_INT ( ret ) ) ;
            PD_TRACE_EXIT ( SDB__RTNPREDLIST_MATLOWELE ) ;
            return ret ;
         }
      }
      PD_TRACE1 ( SDB__RTNPREDLIST_MATLOWELE, PD_PACK_INT ( l ) ) ;
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_MATLOWELE ) ;
      return l ;
   }
   BOOLEAN _rtnPredicateList::matchesElement ( const BSONElement &e, INT32 i,
                                               BOOLEAN direction ) const
   {
      BOOLEAN dummy ;
      return ( 0 == matchingLowElement ( e, i, direction, dummy )%2 ) ;
   }

   // whether a given index key matches the predicate
   // the key should have same amount of elements than predicate size
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_MATKEY, "_rtnPredicateList::matchesKey" )
   BOOLEAN _rtnPredicateList::matchesKey ( const BSONObj &key ) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_MATKEY ) ;
      BOOLEAN ret = TRUE ;
      BSONObjIterator j ( key ) ;
      BSONObjIterator k ( _keyPattern ) ;
      // for each column in predicate list
      for ( INT32 l = 0; l < (INT32)_predicates.size(); ++l )
      {
         // find out scan direction
         INT32 number = (INT32)k.next().number() ;
         BOOLEAN forward = (number>=0?1:-1)*(_direction>=0?1:-1)>0 ;
         // if not matches, then return false
         if ( !matchesElement ( j.next(), l, forward))
         {
            ret = FALSE ;
            break ;
         }
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_MATKEY ) ;
      return ret ;
   }

   _rtnPredicateListIterator::_rtnPredicateListIterator ( const rtnPredicateList
                                                          &predList )
   :
   _predList(predList),
   _cmp ( _predList._predicates.size(), NULL ),
   _inc ( _predList._predicates.size(), FALSE ),
   _currentKey ( _predList._predicates.size(), -1 ),
   _prevKey ( _predList._predicates.size(), -1 )
   {
     reset() ;
     _after = FALSE ;
   }

   INT32 _rtnPredicateListIterator::advancePast ( INT32 i )
   {
      _after = TRUE ;
      return i ;
   }
   INT32 _rtnPredicateListIterator::advancePastZeroed ( INT32 i )
   {
      for ( INT32 j = i; j < (INT32)_currentKey.size(); ++j )
      {
         // no need to reset _cmp and _inc since we pass _after = TRUE in this
         // function, the ixm component will not compare cmp and inc
         _currentKey[j] = 0 ;
      }
      _after = TRUE ;
      return i ;
   }

   // set the i'th field to next start key and reset all other following fields
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_ADVTOLOBOU, "_rtnPredicateListIterator::advanceToLowerBound" )
   INT32 _rtnPredicateListIterator::advanceToLowerBound( INT32 i )
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_ADVTOLOBOU ) ;
      _cmp[i] = &_predList._predicates[i]._startStopKeys[_currentKey[i]
                                                        ]._startKey._bound ;
      _inc[i] = _predList._predicates[i]._startStopKeys[_currentKey[i]
                                                       ]._startKey._inclusive ;
      // reset all other following fields
      for ( INT32 j = i+1; j < (INT32)_currentKey.size(); ++j )
      {
         _cmp[j] =
            &_predList._predicates[j]._startStopKeys.front()._startKey._bound ;
         _inc[j] =
          _predList._predicates[j]._startStopKeys.front()._startKey._inclusive ;
         _currentKey[j] = 0 ;
      }
      _after = FALSE ;
      PD_TRACE_EXIT ( SDB__RTNPREDLISTITE_ADVTOLOBOU ) ;
      return i ;
   }
   // this function is only called when the keyIdx-1'th key is equal predicate
   // MATCH means the currElt matches the currentKey[keyIdx]'s range
   // LESS means currElt is less than the range
   // GREATER means currElt is greater than the range
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_VALCURSSKEY, "_rtnPredicateListIterator::validateCurrentStartStopKey" )
   rtnPredicateCompareResult
     _rtnPredicateListIterator::validateCurrentStartStopKey ( INT32 keyIdx,
                                                const BSONElement &currElt,
                                                BOOLEAN reverse,
                                                BOOLEAN &hitUpperInclusive )
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_VALCURSSKEY ) ;
      rtnPredicateCompareResult re = MATCH ;
      hitUpperInclusive = FALSE ;
      const rtnStartStopKey &key =_predList._predicates[keyIdx]._startStopKeys [
                                         _currentKey[keyIdx]] ;
      INT32 upperMatch = key._stopKey._bound.woCompare ( currElt, FALSE ) ;
      if ( reverse )
         upperMatch = -upperMatch ;
      // element matches stop key and it's inclusive
      if ( upperMatch == 0 && key._stopKey._inclusive )
      {
         hitUpperInclusive = TRUE ;
         // it's a match with stop key
         re = MATCH ;
         goto done ;
      }
      // upperMatch < 0 means the stopKey is smaller than currElt, which means
      // currElt is greater than the range
      if ( upperMatch <= 0 )
      {
         // if it's larger than stopkey, or same but not inclusive
         re = GREATER ;
         goto done ;
      }
      {
         INT32 lowerMatch = key._startKey._bound.woCompare ( currElt, FALSE ) ;
         if ( reverse )
            lowerMatch = -lowerMatch ;
         // element matches start key
         if ( lowerMatch == 0 && key._startKey._inclusive )
         {
            re = MATCH ;
            goto done ;
         }
         // lowerMatch > 0 means the startKey is gerater than currElt, which means
         // currElt is smaller than the range
         if ( lowerMatch >= 0 )
         {
            // if it's less than startkey, or same but not inclusive
            re = LESS ;
         }
      }
   done :
      PD_TRACE_EXIT ( SDB__RTNPREDLISTITE_VALCURSSKEY ) ;
      return re ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_RESET, "_rtnPredicateListIterator::reset" )
   void _rtnPredicateListIterator::reset ()
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_RESET ) ;
      for ( INT32 i = 0; i < (INT32)_currentKey.size(); ++i )
      {
         _cmp[i] =
            &_predList._predicates[i]._startStopKeys.front()._startKey._bound ;
         _inc[i] =
           _predList._predicates[i]._startStopKeys.front()._startKey._inclusive;
         _currentKey[i] = 0 ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLISTITE_RESET ) ;
   }

   INT32 _rtnPredicateListIterator::syncState(
                                      const _rtnPredicateListIterator *source )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      if ( _predList._predicates.size()
           != source->_predList._predicates.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Predlist's size must be same. size: %d, "
                 "sourceSize: %d, rc: %d", _predList._predicates.size(),
                 source->_predList._predicates.size(), rc ) ;
         goto error ;
      }

      for ( i = 0; i < (INT32)_cmp.size(); ++i )
      {
         _cmp[i] = source->_cmp[i] ;
      }

      for ( i = 0; i < (INT32)_inc.size(); ++i )
      {
         _inc[i] = source->_inc[i] ;
      }

      for ( i = 0; i < (INT32)_currentKey.size(); ++i )
      {
         _currentKey[i] = source->_currentKey[i] ;
      }

      for ( i = 0; i < (INT32)_prevKey.size(); ++i )
      {
         _prevKey[i] = source->_prevKey[i] ;
      }

      _after = source->_after ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // Description:
   //   Based on the current index key object, advance the predicate list
   //   iterator if needed
   // Input:
   //   curr: current index key object
   // Return:
   //   -1: selected
   //   -2: hit end of iterator
   //  >=0: the key is not selected
   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_ADVANCE, "_rtnPredicateListIterator::advance" )
   INT32 _rtnPredicateListIterator::advance ( const BSONObj &curr )
   {
      INT32 rc = -1 ;
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_ADVANCE ) ;
      // iterator for input key to match
      BSONObjIterator j ( curr ) ;
      // get index key pattern for direction
      BSONObjIterator o ( _predList._keyPattern ) ;
      // this variable indicates the last field that haven't hit end of the
      // start/stop key list. This is useful when we hit end of the current
      // range, so that we only start from the first field that haven't hit the
      // end (all previous fields remains the same since they can't further
      // grow)
      INT32 latestNonEndPoint = -1 ;
      // for each of the key field
      for ( INT32 i = 0; i < (INT32)_currentKey.size(); ++i )
      {
         _prevKey[i] = _currentKey[i] ;
         // everytime when we search for the best match, we should do binary
         // search to find the best match place
         // one exception is that i-1'th field is equal predicate, and don't
         // change the predicate index which means
         // the next followed field must be in order
         if ( i > 0 && ( _prevKey[ i-1 ] != _currentKey[ i-1 ] ||
              !_predList._predicates[i-1]._startStopKeys[_currentKey[i-1]
                                                        ].isEquality() ) )
         {
            _currentKey[i] = -1 ;
         }
         BSONElement oo = o.next() ;
         // if index defined forward, and direction is forward, reverse = FALSE
         // if index defined forward, and direction is backward, reverse = TRUE
         // if index defined backward, and direction is forward, reverse = TRUE
         // if index defined backward, and direction is backward, reverse=FALSE
         BOOLEAN reverse = ((oo.number()<0)^(_predList._direction<0)) ;
         // now get the i'th field in the key element
         BSONElement jj = j.next() ;
         // this condition is only hit when the previous field is NOT equal
         if ( -1 == _currentKey[i] )
         {
   retry:
            BOOLEAN lowEquality ;
            // compare the key element with predicate
            INT32 l = _predList.matchingLowElement ( jj, i, !reverse,
                                                     lowEquality ) ;
            if ( 0 == l%2 )
            {
               // if we have a match, let's set the current key for i'th column
               // to the one we found
               _currentKey[i] = l/2 ;
               // let's record the last non-end point so that we can do reset
               // from it
               if ( ((INT32)_predList._predicates[i]._startStopKeys.size() >
                        _currentKey[i]+1) ||
                     (_predList._predicates[i]._startStopKeys.back(
                      )._stopKey._bound.woCompare ( jj, FALSE ) != 0) )
               {
                  // this means we are not at the end point
                  // or we are at the end range but didn't hit stopKey
                  latestNonEndPoint = i ;
               }
               // then let's try next field when this field is matched
               continue ;
            }
            else
            {
               // otherwise we are out of range, first let's see if we are at
               // the end
               if ( l ==
                 (INT32)_predList._predicates[i]._startStopKeys.size()*2-1 )
               {
                  // if we hit end of the range, and there's no non-end point,
                  // we don't have any room to further advance
                  if ( -1 == latestNonEndPoint )
                  {
                     rc = -2 ;
                     goto done ;
                  }
                  // otherwise let's reset all currentKey from lastNonEndPoint
                  rc = advancePastZeroed ( latestNonEndPoint + 1 ) ;
                  goto done ;
               }
               // if we are not at the end, let's move to nearest start/stop key
               // range based on the input
               _currentKey[i] = (l+1)/2 ;
               // if we are at the non-inclusive startkey, let's return the next
               // field
               if ( lowEquality )
               {
                  rc = advancePastZeroed ( i+1 ) ;
                  goto done ;
               }
               // otherwise let's move advance to next start/stop key range (
               // note _currentKey[i] = (l+1)/2 in few statement before)
               rc = advanceToLowerBound(i) ;
               goto done ;
            }
         } // if ( -1 == _currentKey[i] )
         // when getting here that means _currentKey[i] != -1
         // this is possible only when the i-1'th field was equal compare
         // eq variable represetns whether a key hits stopKey for a given range
         BOOLEAN eq = FALSE ;
         while ( _currentKey[i] <
                 (INT32)_predList._predicates[i]._startStopKeys.size())
         {
            // since the previous field is equality, we know it's safe to call
            // validateCurrentStartStopKey
            rtnPredicateCompareResult compareResult =
               validateCurrentStartStopKey ( i, jj, reverse, eq ) ;
            // if the result shows jj is greater than the current range, let's
            // move to next range, and compare again
            // we also need to increment _currentKey if the current processed
            // range same as previous range when compareResult shows Less.
            // Otherwise we might loop in same range forever in some situation
            // Why this will work?
            // compareResult = GREATER means the current key in record is
            // greater than the current range
            // comopareResult = MATCH means the current key in record is within
            // the current range
            // compareResult = LESS means the current key in record is smaller
            // than the current range
            if ( GREATER == compareResult )
            {
               _currentKey[i]++ ;
               continue ;
            }
            // jump out the loop if we get a match
            else if ( MATCH == compareResult )
            {
               break ;
            }
            // if jj is less than the current range, let's return the current
            // field id as well as setting _cmp/_inc
            else
            {
               /// has increased the start-stop key, so need to relocated
               if ( _prevKey[i] != _currentKey[i] )
               {
                  rc = advanceToLowerBound(i) ;
               }
               else
               {
                  /// means the pre keys has changed, so need to binary find
                  /// again. ex: { a:1, b:1, c:5 } ==> { a:2, b:1, c:0 }, cur
                  /// is c, range is:[5,5]
                  goto retry ;
               }
               goto done ;
            }
         }
         // when we get here, either we have a match or we hit end of
         // startstopkeyset
         INT32 diff = _predList._predicates[i]._startStopKeys.size() -
                      _currentKey[i] ;
         if ( diff > 1 || ( !eq && diff == 1 ) )
         {
            // if we don't hit the last key, or we are not hitting the stopKey,
            // let's set latest non end point
            // if we hit stop key at the last predicate, we don't want to set
            // this variable then
            latestNonEndPoint = i ;
         }
         // otherwise it means we hit the end if we run out of the loop
         else if ( diff == 0 )
         {
            // if we hit end of the range, and there's no non-end point,
            // we don't have any room to further advance
            if ( -1 == latestNonEndPoint )
            {
               rc = -2 ;
               goto done ;
            }
            // otherwise let's reset all currentKey from lastNonEndPoint
            rc = advancePastZeroed ( latestNonEndPoint + 1 ) ;
            goto done ;
         }
         // when (eq && diff==1), that means we hit stopKey for the last
         // predicate, which we want to continue run the next field and don't
         // set latestNonEndPoint
      }
      // we have a match if we hit here, means all fields got matched
   done :
      PD_TRACE_EXITRC ( SDB__RTNPREDLISTITE_ADVANCE, rc ) ;
      return rc ;
   }

}
