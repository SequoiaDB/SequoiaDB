/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
   }

   void _rtnParamList::toBSON ( BSONObjBuilder &builder ) const
   {
      BSONArrayBuilder subBuilder(
                  builder.subarrayStart( FIELD_NAME_PARAMETERS ) ) ;
      for ( INT8 i = 0 ; i < _paramNum ; i ++ )
      {
         subBuilder.append( _params[ i ]._param ) ;
      }
      subBuilder.done() ;
   }

   BSONObj _rtnParamList::toBSON () const
   {
      BSONObjBuilder builder ;
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

         case MaxKey :
            return minKey ;

         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
            return _rtnKeyMinNumberDecimal ;

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
         case MinKey :
            return maxKey ;

         case MaxKey :
            return _rtnKeyMaxMaxKey ;

         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
            return _rtnKeyMaxNumberDecimal ;

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
      void _merge ( const rtnStartStopKey &n )
      {
         INT32 cmp = _tail._stopKey._bound.woCompare ( n._startKey._bound,
                                                       FALSE ) ;
         if ((cmp<0) || (cmp==0 && !_tail._stopKey._inclusive &&
                                   !n._startKey._inclusive ))
         {
            _result.push_back ( _tail ) ;
            _tail = n ;
            return ;
         }
         cmp = _tail._stopKey._bound.woCompare ( n._stopKey._bound, FALSE ) ;
         if ( (cmp<0) || (cmp==0 && !_tail._stopKey._inclusive &&
                                    !n._stopKey._inclusive ))
         {
            _tail._stopKey = n._stopKey ;
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
      BSONObjBuilder ob ;
      {
         BSONObjBuilder startOB ;
         startOB.append ( _startKey._bound ) ;
         if ( _startKey._inclusive )
         {
            startOB.append ( RTN_START_STOP_KEY_INCL, TRUE ) ;
         }
         ob.append ( RTN_START_STOP_KEY_START,
                     startOB.obj () ) ;
      }
      {
         BSONObjBuilder stopOB ;
         stopOB.append ( _stopKey._bound ) ;
         if ( _stopKey._inclusive )
         {
            stopOB.append ( RTN_START_STOP_KEY_INCL, TRUE ) ;
         }
         ob.append ( RTN_START_STOP_KEY_STOP,
                     stopOB.obj () ) ;
      }
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
         posStart = rtnKeyCompare ( ele, _startKey._bound ) ;
      else
         posStart = rtnKeyCompare ( ele, _stopKey._bound ) ;
      if ( posStart < 0 )
      {
         pos = RTN_SSK_VALUE_POS_LT ;
      }
      else if ( posStart == 0 )
      {
         if ( dir >= 0 )
            pos = _startKey._inclusive?
                    RTN_SSK_VALUE_POS_LET:RTN_SSK_VALUE_POS_LT ;
         else
            pos = _stopKey._inclusive?
                    RTN_SSK_VALUE_POS_LET:RTN_SSK_VALUE_POS_LT ;
      }
      if ( dir >= 0 )
         posStop  = rtnKeyCompare ( ele, _stopKey._bound ) ;
      else
         posStop = rtnKeyCompare ( ele, _startKey._bound ) ;
      if ( posStart > 0 && posStop < 0 )
      {
         pos = RTN_SSK_VALUE_POS_WITHIN ;
      }
      else if ( posStop == 0 )
      {
         if ( dir >= 0 )
            pos = _stopKey._inclusive?
                     RTN_SSK_VALUE_POS_GET:RTN_SSK_VALUE_POS_GT ;
         else
            pos = _startKey._inclusive?
                     RTN_SSK_VALUE_POS_GET:RTN_SSK_VALUE_POS_GT ;
      }
      else
      {
         pos = RTN_SSK_VALUE_POS_GT ;
      }
      if ( dir < 0 )
         pos = (RTN_SSK_VALUE_POS)(((INT32)pos) * -1) ;
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
         if ( RTN_SSK_VALUE_POS_LT == posStop )
         {
            pos = RTN_SSK_RANGE_POS_LT ;
         }
         else if ( RTN_SSK_VALUE_POS_LET == posStop )
         {
            if ( dir >= 0 )
               pos = key._stopKey._inclusive?
                   RTN_SSK_RANGE_POS_LET:RTN_SSK_RANGE_POS_LT ;
            else
               pos = key._startKey._inclusive?
                   RTN_SSK_RANGE_POS_LET:RTN_SSK_RANGE_POS_LT ;
         }
         else if ( RTN_SSK_VALUE_POS_WITHIN == posStop )
         {
            pos = RTN_SSK_RANGE_POS_LI ;
         }
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_LET == posStart )
      {
         if ( RTN_SSK_VALUE_POS_LET == posStop )
         {
            pos = RTN_SSK_RANGE_POS_LET ;
         }
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_WITHIN == posStart )
      {
         if ( RTN_SSK_VALUE_POS_GT == posStop )
         {
            pos = RTN_SSK_RANGE_POS_RI ;
         }
         pos = RTN_SSK_RANGE_POS_CONTAIN ;
      }
      else if ( RTN_SSK_VALUE_POS_GET == posStart )
      {
         if ( dir >= 0 )
            pos = key._startKey._inclusive?
                   RTN_SSK_RANGE_POS_RET:RTN_SSK_RANGE_POS_RT ;
         else
            pos = key._stopKey._inclusive?
                   RTN_SSK_RANGE_POS_RET:RTN_SSK_RANGE_POS_RT ;
      }
      PD_TRACE_EXIT ( SDB_RTNSSKEY_COMPARE2 ) ;
      return pos ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_SIMAPLEREGEX1, "simpleRegex" )
   string simpleRegex ( const CHAR* regex,
                        const CHAR* flags,
                        BOOLEAN *purePrefix )
   {
      PD_TRACE_ENTRY ( SDB_SIMAPLEREGEX1 ) ;
      BOOLEAN extended = FALSE ;
      string r = "";
      stringstream ss;
      if (purePrefix)
         *purePrefix = false;

      BOOLEAN multilineOK;
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
          goto done ;
      }

      while (*flags)
      {
         switch (*(flags++))
         {
         case 'm': // multiline
            if (multilineOK)
               continue;
            else
               goto done ;
         case 'x': // extended
            extended = true;
            break;
         case 's':
            continue;
         default:
            goto done ; // cant use index
         }
      }

      while(*regex)
      {
         CHAR c = *(regex++);
         if ( c == '*' || c == '?' )
         {
            r = ss.str();
            r = r.substr( 0 , r.size() - 1 );
            goto done ; //breaking here fails with /^a?/
         }
         else if (c == '|')
         {
            r = string();
            goto done ;
         }
         else if (c == '\\')
         {
            c = *(regex++);
            if (c == 'Q')
            {
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
            else if ((c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '0') ||
                     (c == '\0'))
            {
               r = ss.str();
               break;
            }
            else
            {
               ss << c;
            }
         } // else if (c == '\\')
         else if (strchr("^$.[()+{", c))
         {
            r = ss.str();
            break;
         }
         else if (extended && c == '#')
         {
            r = ss.str();
            break;
         }
         else if (extended && isspace(c))
         {
            continue;
         }
         else
         {
            ss << c;
         }
      }

      if ( r.empty() && *regex == 0 )
      {
         r = ss.str();
         if (purePrefix)
            *purePrefix = !r.empty();
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
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PREDOVERLAP, "predicatesOverlap" )
   BOOLEAN predicatesOverlap ( rtnStartStopKey &l,
                               rtnStartStopKey &r,
                               rtnStartStopKey &result )
   {
      PD_TRACE_ENTRY ( SDB_PREDOVERLAP ) ;

      result._startKey = maxKeyBound ( l._startKey, r._startKey, TRUE ) ;
      result._stopKey = minKeyBound ( l._stopKey, r._stopKey, TRUE ) ;

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
      for ( vector<BSONObj>::const_iterator i = other._objData.begin() ;
            i != other._objData.end(); i++ )
      {
         _objData.push_back(*i) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_OPEQU, "rtnPredicate::operator&=" )
   const rtnPredicate &rtnPredicate::operator&=
                      (rtnPredicate &right)
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPEQU ) ;
      RTN_SSKEY_LIST newKeySet ;
      RTN_SSKEY_LIST::iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::iterator j = right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end())
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
      PD_TRACE_EXIT ( SDB_RTNPRED_OPEQU ) ;
      return *this ;
   }

   const rtnPredicate &rtnPredicate::operator=
                      (const rtnPredicate &right)
   {
      finishOperation(right._startStopKeys, right) ;
      _paramIndex = right._paramIndex ;
      _fuzzyIndex = right._fuzzyIndex ;
      _isInitialized = right._isInitialized ;
      return *this ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB_RTNPRED_OPOREQ, "rtnPredicate::operator|=" )
   const rtnPredicate &rtnPredicate::operator|=
                      (const rtnPredicate &right)
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPOREQ ) ;
      startStopKeyUnionBuilder b ;
      RTN_SSKEY_LIST::const_iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::const_iterator j =
                        right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end())
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
      PD_TRACE_EXIT ( SDB_RTNPRED_OPOREQ ) ;
      return *this ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_OPMINUSEQ, "rtnPredicate::operator-=" )
   const rtnPredicate &rtnPredicate::operator-=
                      (const rtnPredicate &right)
   {
      PD_TRACE_ENTRY ( SDB_RTNPRED_OPMINUSEQ ) ;
      RTN_SSKEY_LIST newKeySet ;
      RTN_SSKEY_LIST::iterator i = _startStopKeys.begin() ;
      RTN_SSKEY_LIST::const_iterator j =
                        right._startStopKeys.begin() ;
      while ( i != _startStopKeys.end() && j != right._startStopKeys.end())
      {
         INT32 cmp = i->_startKey._bound.woCompare ( j->_startKey._bound,
                                                     FALSE ) ;
         if ( cmp < 0 || ( cmp==0 && i->_startKey._inclusive &&
                                    !j->_startKey._inclusive ) )
         {
            INT32 cmp1 = i->_stopKey._bound.woCompare ( j->_startKey._bound,
                                                        FALSE ) ;
            newKeySet.push_back (*i) ;
            if ( cmp1 < 0 )
            {
               ++i ;
            }
            else if ( cmp1 == 0 )
            {
               if ( newKeySet.back()._stopKey._inclusive &&
                    j->_startKey._inclusive )
               {
                  newKeySet.back()._stopKey._inclusive = FALSE ;
               }
               ++i ;
            }
            else
            {
               newKeySet.back()._stopKey = j->_startKey ;
               newKeySet.back()._stopKey.flipInclusive() ;
               INT32 cmp2 = i->_stopKey._bound.woCompare (
                               j->_stopKey._bound, FALSE ) ;
               if ( cmp2 < 0 || ( cmp2 == 0 && (
                         !i->_stopKey._inclusive ||
                          j->_stopKey._inclusive )))
               {
                  ++i ;
               }
               else
               {
                  i->_startKey = j->_stopKey ;
                  i->_startKey.flipInclusive() ;
                  ++j ;
               }
            }
         }
         else
         {
            INT32 cmp1 = i->_startKey._bound.woCompare ( j->_stopKey._bound,
                                                         FALSE ) ;
            if ( cmp1 > 0 || (cmp1 == 0 && (!i->_stopKey._inclusive ||
                                            !j->_stopKey._inclusive )))
            {
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
                  ++i ;
               }
               else
               {
                  i->_startKey = j->_stopKey ;
                  i->_startKey.flipInclusive() ;
                  ++j ;
               }
            }
         }
      } // while ( i != _startStopKeys.end() && j !=
      while ( i != _startStopKeys.end() )
      {
         newKeySet.push_back (*i) ;
         ++i ;
      }
      finishOperation ( newKeySet, right ) ;
      PD_TRACE_EXIT ( SDB_RTNPRED_OPMINUSEQ ) ;
      return *this ;
   }
   BOOLEAN rtnPredicate::operator<= ( const rtnPredicate &r ) const
   {
      rtnPredicate temp = *this ;
      temp -= r ;
      return temp.isEmpty() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_RTNPRED, "rtnPredicate::rtnPredicate" )
   rtnPredicate::rtnPredicate ( const BSONElement &e, INT32 opType,
                                BOOLEAN isNot, BOOLEAN mixCmp,
                                INT8 paramIndex, INT8 fuzzyIndex )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_TOSTRING, "rtnPredicate::toString()" )
   string rtnPredicate::toString() const
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
      return buf.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPRED_BINDPARAM, "rtnPredicate::bindParameters" )
   BOOLEAN rtnPredicate::bindParameters ( rtnParamList &parameters,
                                          BOOLEAN markDone )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB_RTNPRED_BINDPARAM ) ;

      if ( _paramIndex >= 0 )
      {
         for ( RTN_SSKEY_LIST::iterator i = _startStopKeys.begin() ;
               i != _startStopKeys.end();
               ++ i )
         {
            if ( i->_startKey._parameterized )
            {
               i->_startKey._bound = parameters.getParam( _paramIndex ) ;
               if ( _fuzzyIndex >= 0 )
               {
                  i->_startKey._inclusive =
                        parameters.getParam( _fuzzyIndex ).booleanSafe() ;
               }
            }
            if ( i->_stopKey._parameterized )
            {
               i->_stopKey._bound = parameters.getParam( _paramIndex ) ;
               if ( _fuzzyIndex >= 0 )
               {
                  i->_stopKey._inclusive =
                        parameters.getParam( _fuzzyIndex ).booleanSafe() ;
               }
            }
         }
         if ( markDone )
         {
            parameters.setDoneByPred( _paramIndex ) ;
         }
         res = TRUE ;
      }

      PD_TRACE_EXIT( SDB_RTNPRED_BINDPARAM ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITIN, "_rtnPredicateSet::_initIN" )
   INT32 rtnPredicate::_initIN ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN mixCmp, BOOLEAN expandRegex )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITIN ) ;

      PD_CHECK( Array == e.type(), SDB_INVALIDARG, error, PDERROR,
                "Invalid $in expression" ) ;

      if ( !isNot )
      {
         set<BSONElement, element_cmp_lt> vals ;
         RTN_PREDICATE_LIST regexes ;
         BSONObjIterator i ( e.embeddedObject() ) ;

         if ( !i.more() )
         {
            vals.insert ( e ) ;
         }
         while ( i.more() )
         {
            BSONElement ie = i.next() ;

            if ( ie.type() == Object &&
                 ie.embeddedObject().firstElement().getGtLtOp() ==
                       BSONObj::opELEM_MATCH )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "$eleMatch is not allowed within $in" ) ;
               goto done ;
            }

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

            vals.insert ( ie ) ;
         }

         for ( set<BSONElement,element_cmp_lt>::const_iterator i = vals.begin();
               i!=vals.end(); i++ )
         {
            _startStopKeys.push_back ( rtnStartStopKey ( *i ) ) ;
         }

         for ( RTN_PREDICATE_LIST::const_iterator i = regexes.begin();
               i!=regexes.end(); i++ )
         {
            *this |= *i ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITREGEX, "_rtnPredicateSet::_initRegEx" )
   INT32 rtnPredicate::_initRegEx ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITREGEX ) ;

      PD_CHECK( e.type() == RegEx ||
                ( e.type() == Object && !e.embeddedObject()[ "$regex" ].eoo() ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid regular expression operator" ) ;

      if ( !isNot )
      {
         const string r = simpleRegex( e ) ;

         if ( r.size() )
         {
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
            _initTypeRange( String, TRUE ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITREGEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITET, "_rtnPredicateSet::_initET" )
   INT32 rtnPredicate::_initET ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITET ) ;

      if ( isNot )
      {
         rc = _initNE( e, FALSE ) ;
      }
      else if ( Array == e.type() )
      {
      }
      else
      {
         _startStopKeys.push_back ( rtnStartStopKey( e ) ) ;
         if ( -1 != _paramIndex )
         {
            rtnStartStopKey &keyPair = _startStopKeys.back() ;
            keyPair._startKey._parameterized = TRUE ;
            keyPair._stopKey._parameterized = TRUE ;
         }
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITET, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITNE, "_rtnPredicateSet::_initNE" )
   INT32 rtnPredicate::_initNE ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITNE ) ;

      if ( e.type() == Array )
      {
         goto done ;
      }

      if ( isNot )
      {
         rc = _initET( e, FALSE ) ;
      }
      else
      {
         _initLT( e, FALSE, FALSE, TRUE ) ;
         _initGT( e, FALSE, FALSE, TRUE ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITNE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITLT, "_rtnPredicateSet::_initLT" )
   INT32 rtnPredicate::_initLT ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN inclusive, BOOLEAN mixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITLT ) ;

      if ( e.type() == Array )
      {
         goto done ;
      }

      if ( isNot )
      {
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
               startKey = minKey.firstElement() ;
               startInclusive = TRUE ;
            }
            else
            {
               _initMinRange( TRUE ) ;

               startKey = staticUndefined.firstElement() ;
               startInclusive = FALSE ;
            }
         }
         else
         {
            startKey = rtnKeyGetMinForCmp( e.type(), FALSE ).firstElement() ;

            startInclusive = ( startKey.canonicalType() == e.canonicalType() ) ;
         }

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

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITLT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITGT, "_rtnPredicateSet::_initGT" )
   INT32 rtnPredicate::_initGT ( const BSONElement &e, BOOLEAN isNot,
                                 BOOLEAN inclusive, BOOLEAN mixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITGT ) ;

      if ( e.type() == Array )
      {
         goto done ;
      }

      if ( isNot )
      {
         rc = _initLT( e, FALSE, !inclusive, mixCmp ) ;
      }
      else
      {
         BOOLEAN minRangeAdded = FALSE ;

         if ( e.canonicalType() <=
              staticUndefined.firstElement().canonicalType() )
         {
            _initMinRange( inclusive ) ;
            minRangeAdded = TRUE ;
         }

         _startStopKeys.push_back( rtnStartStopKey() ) ;
         rtnStartStopKey &keyPair = _startStopKeys.back() ;

         if ( minRangeAdded )
         {
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

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITGT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITALL, "_rtnPredicateSet::_initALL" )
   INT32 rtnPredicate::_initALL ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITALL ) ;

      PD_CHECK( e.type() == Array, SDB_INVALIDARG, error, PDERROR,
                "Must be array type for opALL" ) ;

      if ( !isNot )
      {
         BSONObjIterator i ( e.embeddedObject()) ;
         while ( i.more() )
         {
            BSONElement x = i.next() ;
            if ( x.type() == Object &&
                 x.embeddedObject().firstElement().getGtLtOp() ==
                 BSONObj::opELEM_MATCH )
               continue ;
            _startStopKeys.push_back( rtnStartStopKey( x ) ) ;
            break ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNPRED__INITALL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITMOD, "_rtnPredicateSet::_initMOD" )
   INT32 rtnPredicate::_initMOD ( const BSONElement &e, BOOLEAN isNot )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITMOD ) ;

      if ( !isNot )
      {
         rc = _initTypeRange( NumberDecimal, TRUE ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITMOD, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITEXISTS, "_rtnPredicateSet::_initEXISTS" )
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
         _startStopKeys.push_back (
               rtnStartStopKey( staticUndefined.firstElement() ) ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITEXISTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITISNULL, "_rtnPredicateSet::_initISNULL" )
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
         _startStopKeys.push_back (
               rtnStartStopKey( staticUndefined.firstElement() ) ) ;

         _startStopKeys.push_back (
               rtnStartStopKey( staticNull.firstElement() ) ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNPRED__INITISNULL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITFULLRANGE, "_rtnPredicateSet::_initFullRange" )
   INT32 rtnPredicate::_initFullRange ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITFULLRANGE ) ;

      _startStopKeys.push_back ( rtnStartStopKey() ) ;
      rtnStartStopKey &keyPair = _startStopKeys.back() ;

      keyPair._startKey._bound = minKey.firstElement() ;
      keyPair._startKey._inclusive = TRUE ;

      keyPair._stopKey._bound = maxKey.firstElement() ;
      keyPair._stopKey._inclusive = TRUE ;

      PD_TRACE_EXITRC( SDB__RTNPRED__INITFULLRANGE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITTYPERANGE, "_rtnPredicateSet::_initTypeRange" )
   INT32 rtnPredicate::_initTypeRange ( BSONType type, BOOLEAN forCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITTYPERANGE ) ;

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

      PD_TRACE_EXITRC( SDB__RTNPRED__INITTYPERANGE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPRED__INITMINRANGE, "_rtnPredicateSet::_initMinRange" )
   INT32 rtnPredicate::_initMinRange ( BOOLEAN startIncluded )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNPRED__INITMINRANGE ) ;

      _startStopKeys.push_back ( rtnStartStopKey() ) ;
      rtnStartStopKey &keyPair = _startStopKeys.back() ;

      keyPair._startKey._bound = minKey.firstElement() ;
      keyPair._startKey._inclusive = startIncluded ;

      keyPair._stopKey._bound = staticUndefined.firstElement() ;
      keyPair._stopKey._inclusive = FALSE ;

      PD_TRACE_EXITRC( SDB__RTNPRED__INITMINRANGE, rc ) ;

      return rc ;
   }

   string _rtnPredicateSet::toString() const
   {
      StringBuilder buf ;
      buf << "[ " ;
      RTN_PREDICATE_MAP::const_iterator it = _predicates.begin() ;
      while ( it != _predicates.end() )
      {
         buf << it->first << ":" << it->second.toString() << " " ;
         ++it ;
      }
      buf << " ]" ;
      return buf.str() ;
   }

   BSONObj _rtnPredicateSet::toBson() const
   {
      BSONObjBuilder builder ;
      RTN_PREDICATE_MAP::const_iterator it = _predicates.begin() ;
      for ( ; it != _predicates.end(); ++it )
      {
         BSONArrayBuilder sub( builder.subarrayStart( it->first ) ) ;
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
         pRet = &genericPredList ;
      }
      else
      {
         pRet = &(f->second) ;
      }
      PD_TRACE_EXIT ( SDB__RTNPRED_PARAMPRED ) ;
      return *pRet ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDSET_MALEFORINDEX, "_rtnPredicateSet::matchLevelForIndex" )
   INT32 _rtnPredicateSet::matchLevelForIndex (const BSONObj &keyPattern) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDSET_MALEFORINDEX ) ;
      INT32 level = 0 ;
      BSONObjIterator i ( keyPattern ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( predicate(e.fieldName()).isGeneric() )
            return level ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDSET_MALEFORINDEX ) ;
      return level ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDSET_ADDPRED, "_rtnPredicateSet::addPredicate" )
   INT32 _rtnPredicateSet::addPredicate ( const CHAR *fieldName,
                                          const BSONElement &e,
                                          INT32 opType, BOOLEAN isNot,
                                          BOOLEAN mixCmp, BOOLEAN addToParam,
                                          INT8 paramIndex, INT8 fuzzyIndex )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNPREDSET_ADDPRED ) ;
      std::pair< RTN_PREDICATE_MAP::iterator, BOOLEAN > ret ;
      rtnPredicate pred ( e, opType, isNot, mixCmp, paramIndex, fuzzyIndex ) ;
      if ( !pred.isInit() )
      {
         PD_LOG ( PDERROR, "Failed to add predicate %s: %s",
                  fieldName, e.toString().c_str()) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ret = _predicates.insert( make_pair( fieldName, pred ) ) ;
      if ( !(ret.second) )
      {
         ret.first->second &= pred ;
      }

      if ( addToParam )
      {
         RTN_PARAM_PREDICATE_MAP::iterator iter = _paramPredicates.find( fieldName ) ;
         if ( iter == _paramPredicates.end() )
         {
            RTN_PREDICATE_LIST predList ;
            predList.push_back( pred ) ;
            _paramPredicates.insert( make_pair( fieldName, predList ) ) ;
         }
         else
         {
            iter->second.push_back( pred ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNPREDSET_ADDPRED, rc ) ;
      return rc ;
   error :
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

      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;

      addedLevel = 0 ;

      BSONObjIterator i( _keyPattern ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         const rtnPredicate &pred = predSet.predicate ( e.fieldName() ) ;
         if ( pred.isEmpty() )
         {
            _addEmptyPredicate() ;
            break ;
         }
         _addPredicate( e, direction, pred ) ;
         addedLevel ++ ;
      }

      _fixedPredList = TRUE ;
      _initialized = TRUE ;

      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT, rc ) ;
      return rc ;
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

      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;

      BSONObjIterator i( _keyPattern ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         const RTN_PREDICATE_LIST &predicates =
                                 predSet.paramPredicate( e.fieldName() ) ;
         if ( predicates.empty() )
         {
            _addEmptyPredicate() ;
            continue ;
         }
         else
         {
            rtnPredicate pred, nonParamPred ;
            BOOLEAN containNonParamPred = FALSE ;
            BOOLEAN nonParamPredEmpty = FALSE ;
            BOOLEAN paramPredEmpty = FALSE ;

            RTN_PREDICATE_LIST paramList ;
            RTN_PREDICATE_LIST::const_iterator iter = predicates.begin () ;
            if ( iter != predicates.end() )
            {
               pred = (*iter) ;
               if ( !pred.bindParameters( parameters ) )
               {
                  nonParamPred = pred ;
                  containNonParamPred = TRUE ;
               }
               else
               {
                  paramList.push_back( pred ) ;
               }
               ++ iter ;
               for ( ;
                     iter != predicates.end() && !nonParamPredEmpty ;
                     ++ iter )
               {
                  rtnPredicate currentPred = (*iter) ;
                  if ( !currentPred.bindParameters( parameters ) )
                  {
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
                     paramList.push_back( currentPred ) ;
                  }

                  if ( !paramPredEmpty )
                  {
                     pred &= currentPred ;
                     if ( pred.isEmpty() )
                     {
                        paramPredEmpty = TRUE ;
                     }
                  }
               }

               if ( containNonParamPred )
               {
                  if ( nonParamPredEmpty )
                  {
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

            if ( needAddPred && pred.isEmpty() )
            {
               _addEmptyPredicate() ;
               needAddPred = FALSE ;
               if ( nonParamPredEmpty )
               {
                  break ;
               }
            }
            else if ( needAddPred )
            {
               _addPredicate( e, direction, pred ) ;
            }
         }
      }

      if ( !containParamPred )
      {
         _fixedPredList = TRUE ;
      }
      _initialized = TRUE ;

      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT_PARAM, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_INIT_PARAMLIST, "_rtnPredicateList::_rtnPredicateList" )
   INT32 _rtnPredicateList::initialize ( const RTN_PARAM_PREDICATE_LIST &paramPredList,
                                         const BSONObj &keyPattern,
                                         INT32 direction,
                                         rtnParamList &parameters )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_INIT_PARAMLIST ) ;

      _direction = direction > 0 ? 1 : -1 ;
      _keyPattern = keyPattern ;

      RTN_PARAM_PREDICATE_LIST::const_iterator iter = paramPredList.begin() ;
      BSONObjIterator i( _keyPattern ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( iter == paramPredList.end() )
         {
            _addEmptyPredicate() ;
            break ;
         }
         else
         {
            rtnPredicate pred ;

            RTN_PREDICATE_LIST::const_iterator predIter = iter->begin () ;
            if ( predIter != iter->end() )
            {
               pred = (*predIter) ;
               pred.bindParameters( parameters ) ;
               ++ predIter ;
               for ( ; predIter != iter->end() ; ++ predIter )
               {
                  rtnPredicate currentPred = (*predIter) ;
                  currentPred.bindParameters( parameters ) ;
                  pred &= currentPred ;
                  if ( pred.isEmpty() )
                  {
                     break ;
                  }
               }
            }

            if ( pred.isEmpty() )
            {
               _addEmptyPredicate() ;
               break ;
            }
            else
            {
               _addPredicate( e, direction, pred ) ;
            }
         }
         ++ iter ;
      }

      _initialized = TRUE ;

      PD_TRACE_EXITRC( SDB__RTNPREDLIST_INIT_PARAMLIST, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST__ADDPRED, "_rtnPredicateList::_addPredicate" )
   void _rtnPredicateList::_addPredicate ( const BSONElement &e,
                                           INT32 direction,
                                           const rtnPredicate &pred )
   {
      PD_TRACE_ENTRY( SDB__RTNPREDLIST__ADDPRED ) ;

      INT32 num = (INT32)e.number() ;

      BOOLEAN forward = ((num>=0?1:-1)*(direction>=0?1:-1)>0) ;
      if ( forward )
      {
         _predicates.push_back ( pred ) ;
      }
      else
      {
         _predicates.push_back ( rtnPredicate () ) ;
         pred.reverse ( _predicates.back() ) ;
      }

      PD_TRACE_EXIT( SDB__RTNPREDLIST__ADDPRED ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST__ADDEMPPRED, "_rtnPredicateList::_addEmptyPredicate" )
   void _rtnPredicateList::_addEmptyPredicate ()
   {
      PD_TRACE_ENTRY( SDB__RTNPREDLIST__ADDEMPPRED ) ;

      static rtnStartStopKey s_emptyStartStopKey( 0 ) ;

      rtnPredicate emptyPred ;
      emptyPred._startStopKeys.push_back( s_emptyStartStopKey ) ;

      _predicates.push_back( emptyPred ) ;

      PD_TRACE_EXIT( SDB__RTNPREDLIST__ADDEMPPRED ) ;
   }

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
   BSONObj _rtnPredicateList::obj() const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_OBJ ) ;
      BSONObjBuilder b ;
      for ( INT32 i = 0; i<(INT32)_predicates.size(); i++ )
      {
         BSONArrayBuilder a ( b.subarrayStart("") ) ;
         for ( RTN_SSKEY_LIST::const_iterator j =
                                    _predicates[i]._startStopKeys.begin() ;
               j != _predicates[i]._startStopKeys.end() ;
               ++ j )
         {
            a << BSONArray ( BSON_ARRAY ( j->_startKey._bound <<
                       j->_stopKey._bound ).clientReadable() ) ;
         }
         a.done () ;
      }
      PD_TRACE_EXIT ( SDB__RTNPREDLIST_OBJ ) ;
      return b.obj() ;
   }
   string _rtnPredicateList::toString() const
   {
      return obj().toString(false, false) ;
   }

   BSONObj _rtnPredicateList::getBound() const
   {
      BSONObjBuilder builder ;
      BSONObj predicate = obj() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_MATLOWELE, "_rtnPredicateList::matchingLowElement" )
   INT32 _rtnPredicateList::matchingLowElement ( const BSONElement &e, INT32 i,
                                                BOOLEAN direction,
                                                BOOLEAN &lowEquality ) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_MATLOWELE ) ;
      lowEquality = FALSE ;
      INT32 l = -1 ;
      INT32 h = _predicates[i]._startStopKeys.size() * 2 ;
      INT32 m = 0 ;
      while ( l + 1 < h )
      {
         m = ( l + h ) / 2 ;
         BSONElement toCmp ;
         BOOLEAN toCmpInclusive ;
         const rtnStartStopKey &startstopkey=_predicates[i]._startStopKeys[m/2];
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
         INT32 result = toCmp.woCompare ( e, FALSE ) ;
         if ( !direction )
            result = -result ;
         if ( result < 0 )
            l = m ;
         else if ( result > 0 )
            h = m ;
         else
         {
            if ( 0 == m%2 )
               lowEquality = TRUE ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLIST_MATKEY, "_rtnPredicateList::matchesKey" )
   BOOLEAN _rtnPredicateList::matchesKey ( const BSONObj &key ) const
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLIST_MATKEY ) ;
      BOOLEAN ret = TRUE ;
      BSONObjIterator j ( key ) ;
      BSONObjIterator k ( _keyPattern ) ;
      for ( INT32 l = 0; l < (INT32)_predicates.size(); ++l )
      {
         INT32 number = (INT32)k.next().number() ;
         BOOLEAN forward = (number>=0?1:-1)*(_direction>=0?1:-1)>0 ;
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
         _currentKey[j] = 0 ;
      }
      _after = TRUE ;
      return i ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_ADVTOLOBOU, "_rtnPredicateListIterator::advanceToLowerBound" )
   INT32 _rtnPredicateListIterator::advanceToLowerBound( INT32 i )
   {
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_ADVTOLOBOU ) ;
      _cmp[i] = &_predList._predicates[i]._startStopKeys[_currentKey[i]
                                                        ]._startKey._bound ;
      _inc[i] = _predList._predicates[i]._startStopKeys[_currentKey[i]
                                                       ]._startKey._inclusive ;
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
      if ( upperMatch == 0 && key._stopKey._inclusive )
      {
         hitUpperInclusive = TRUE ;
         re = MATCH ;
         goto done ;
      }
      if ( upperMatch <= 0 )
      {
         re = GREATER ;
         goto done ;
      }
      {
         INT32 lowerMatch = key._startKey._bound.woCompare ( currElt, FALSE ) ;
         if ( reverse )
            lowerMatch = -lowerMatch ;
         if ( lowerMatch == 0 && key._startKey._inclusive )
         {
            re = MATCH ;
            goto done ;
         }
         if ( lowerMatch >= 0 )
         {
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

   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNPREDLISTITE_ADVANCE, "_rtnPredicateListIterator::advance" )
   INT32 _rtnPredicateListIterator::advance ( const BSONObj &curr )
   {
      INT32 rc = -1 ;
      PD_TRACE_ENTRY ( SDB__RTNPREDLISTITE_ADVANCE ) ;
      BSONObjIterator j ( curr ) ;
      BSONObjIterator o ( _predList._keyPattern ) ;
      INT32 latestNonEndPoint = -1 ;
      for ( INT32 i = 0; i < (INT32)_currentKey.size(); ++i )
      {
         _prevKey[i] = _currentKey[i] ;
         if ( i > 0 && ( _prevKey[ i-1 ] != _currentKey[ i-1 ] ||
              !_predList._predicates[i-1]._startStopKeys[_currentKey[i-1]
                                                        ].isEquality() ) )
         {
            _currentKey[i] = -1 ;
         }
         BSONElement oo = o.next() ;
         BOOLEAN reverse = ((oo.number()<0)^(_predList._direction<0)) ;
         BSONElement jj = j.next() ;
         if ( -1 == _currentKey[i] )
         {
   retry:
            BOOLEAN lowEquality ;
            INT32 l = _predList.matchingLowElement ( jj, i, !reverse,
                                                     lowEquality ) ;
            if ( 0 == l%2 )
            {
               _currentKey[i] = l/2 ;
               if ( ((INT32)_predList._predicates[i]._startStopKeys.size() >
                        _currentKey[i]+1) ||
                     (_predList._predicates[i]._startStopKeys.back(
                      )._stopKey._bound.woCompare ( jj, FALSE ) != 0) )
               {
                  latestNonEndPoint = i ;
               }
               continue ;
            }
            else
            {
               if ( l ==
                 (INT32)_predList._predicates[i]._startStopKeys.size()*2-1 )
               {
                  if ( -1 == latestNonEndPoint )
                  {
                     rc = -2 ;
                     goto done ;
                  }
                  rc = advancePastZeroed ( latestNonEndPoint + 1 ) ;
                  goto done ;
               }
               _currentKey[i] = (l+1)/2 ;
               if ( lowEquality )
               {
                  rc = advancePastZeroed ( i+1 ) ;
                  goto done ;
               }
               rc = advanceToLowerBound(i) ;
               goto done ;
            }
         } // if ( -1 == _currentKey[i] )
         BOOLEAN eq = FALSE ;
         while ( _currentKey[i] <
                 (INT32)_predList._predicates[i]._startStopKeys.size())
         {
            rtnPredicateCompareResult compareResult =
               validateCurrentStartStopKey ( i, jj, reverse, eq ) ;
            if ( GREATER == compareResult )
            {
               _currentKey[i]++ ;
               continue ;
            }
            else if ( MATCH == compareResult )
            {
               break ;
            }
            else
            {
               if ( _prevKey[i] != _currentKey[i] )
               {
                  rc = advanceToLowerBound(i) ;
               }
               else
               {
                  goto retry ;
               }
               goto done ;
            }
         }
         INT32 diff = _predList._predicates[i]._startStopKeys.size() -
                      _currentKey[i] ;
         if ( diff > 1 || ( !eq && diff == 1 ) )
         {
            latestNonEndPoint = i ;
         }
         else if ( diff == 0 )
         {
            if ( -1 == latestNonEndPoint )
            {
               rc = -2 ;
               goto done ;
            }
            rc = advancePastZeroed ( latestNonEndPoint + 1 ) ;
            goto done ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__RTNPREDLISTITE_ADVANCE, rc ) ;
      return rc ;
   }
}
