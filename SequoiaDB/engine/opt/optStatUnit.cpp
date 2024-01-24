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

   Source File Name = optStatUnit.cpp

   Descriptive Name = Optimizer Statistics Object Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Statistics
   Objects.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "optStatUnit.hpp"
#include "dmsStorageUnit.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "pmd.hpp"
#include "ixm.hpp"
#include "optCommon.hpp"
#include "optAccessPlanHelper.hpp"

using namespace bson ;

namespace engine
{

   /*
      Helper functions define
    */
   static double optConvertStrToScalar ( const CHAR *pValue, UINT32 valueSize,
                                         UINT8 low, UINT8 high ) ;

   static FLOAT64 _optEvalDefETSel( UINT64 recordNum, const BSONElement &beValue ) ;
   static FLOAT64 _optEvalDefGTSel( const BSONElement &beStart, BOOLEAN startIncluded ) ;
   static FLOAT64 _optEvalDefLTSel( const BSONElement &beStop, BOOLEAN stopIncluded ) ;
   static FLOAT64 _optEvalDefRangeSel( const BSONElement &beStart, BOOLEAN startIncluded,
                                       const BSONElement &beStop, BOOLEAN stopIncluded ) ;

   static FLOAT64 _optStatPredEQSel[] =
   {
      OPT_PRED_DEFAULT_SELECTIVITY,
      OPT_PRED_EQ_DEF_SELECTIVITY,
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 2 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 3 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 4 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 5 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 6 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 7 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 8 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 9 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 10 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 11 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 12 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 13 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 14 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 15 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 16 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 17 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 18 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 19 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 20 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 21 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 22 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 23 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 24 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 25 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 26 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 27 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 28 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 29 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 30 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 31 ),
      pow( OPT_PRED_EQ_DEF_SELECTIVITY, 32 ),
   } ;

   static FLOAT64 _optStatPredRangeSel[] =
   {
      OPT_PRED_DEFAULT_SELECTIVITY,
      OPT_PRED_RANGE_DEF_SELECTIVITY,
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 2 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 3 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 4 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 5 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 6 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 7 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 8 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 9 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 10 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 11 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 12 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 13 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 14 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 15 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 16 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 17 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 18 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 19 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 20 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 21 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 22 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 23 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 24 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 25 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 26 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 27 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 28 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 29 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 30 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 31 ),
      pow( OPT_PRED_RANGE_DEF_SELECTIVITY, 32 ),
   } ;

   static const UINT32 _optStatMaxPredNum = 32 ;

   static FLOAT64 _optStatGetEQSel( UINT32 numKeys )
   {
      if ( numKeys > _optStatMaxPredNum )
      {
         return _optStatPredEQSel[ _optStatMaxPredNum ] ;
      }
      return _optStatPredEQSel[ numKeys ] ;
   }

   static FLOAT64 _optStatGetRangeSel( UINT32 numKeys )
   {
      if ( numKeys > _optStatMaxPredNum )
      {
         return _optStatPredRangeSel[ _optStatMaxPredNum ] ;
      }
      return _optStatPredRangeSel[ numKeys ] ;
   }

   /*
      _optIndexPathEncoder implement
    */
   ossPoolString _optIndexPathEncoder::getPath()
   {
      try
      {
         if ( _isError )
         {
            return ossPoolString() ;
         }
         else
         {
            _done() ;
            return ossPoolString( _bb.buf(), _bb.len() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to get index path, occur exception %s",
                 e.what() ) ;
         _isError = TRUE ;
         return ossPoolString() ;
      }
   }

   void _optIndexPathEncoder::append( const CHAR *pFieldName, BOOLEAN isAllRange )
   {
      if ( !_isDone && !_isError )
      {
         try
         {
            if ( _bb.len() > 0 )
            {
               _bb.appendChar( '_' ) ;
            }
            CHAR buf[ 32 ] = { 0 } ;
            sprintf( buf, "%u", (UINT32)( ossStrlen( pFieldName ) ) ) ;
            _bb.appendStr( buf, false ) ;
            _bb.appendChar( '_' ) ;
            _bb.appendStr( pFieldName, false ) ;
            ++ _keyCount ;

            // if is all range, the current field is not valid
            // if the current field is valie, the prefix of path is valid,
            // even though it has all range fields
            if ( !isAllRange )
            {
               _validLength = _bb.len() ;
               _validCount = _keyCount ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to append index path, occur exception %s",
                    e.what() ) ;
            _isError = TRUE ;
         }
      }
      else if ( _isDone )
      {
         PD_LOG( PDWARNING, "Failed to append index path, encoder is already done" ) ;
         _isError = TRUE ;
      }
   }

   /*
      _optStatListKey implement
    */
   _optStatListKey::_optStatListKey ()
   : ossPoolList<const rtnKeyBoundary *> (),
     _dmsStatKey( TRUE )
   {
   }

   INT32 _optStatListKey::compareValue ( INT32 cmpFlag, INT32 incFlag,
                                         const BSONObj &rValue )
   {
      INT32 res = 0 ;

      _optStatListKey::iterator iterLeft = begin() ;
      BSONObjIterator iterRight( rValue ) ;

      while ( iterLeft != end() && iterRight.more() )
      {
         const BSONElement &beLeft = (*iterLeft)->_bound ;
         BOOLEAN inclusive = (*iterLeft)->_inclusive ;
         iterLeft ++ ;

         BSONElement beRight = iterRight.next() ;

         res = beLeft.woCompare( beRight, FALSE ) ;

         if ( 0 != res )
         {
            break ;
         }
         else if ( !inclusive && cmpFlag != 0 )
         {
            // Result is equal but not included, adjust with cmpFlag
            // cmpFlag is -1, left is bigger than right
            // cmpFlag is 1, left is smaller than right
            res = cmpFlag > 0 ? -1 : 1 ;
            break ;
         }
      }

      if ( 0 == res )
      {
         // Compared elements are equal, adjust with incFlag
         if ( iterRight.more() )
         {
            res = _equalButRightMore( incFlag ) ;
         }
         else if ( iterLeft != end() )
         {
            res = _equalButLeftMore( incFlag ) ;
         }
         else
         {
            res = _equalDefault( incFlag ) ;
         }
      }

      return res ;
   }

   BOOLEAN _optStatListKey::compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                               const BSONObj &rValue )
   {
      if ( startIdx > size() || startIdx > (UINT32)rValue.nFields() )
      {
         return FALSE ;
      }

      if ( size() != (UINT32)rValue.nFields() )
      {
         return FALSE ;
      }

      _optStatListKey::iterator iterLeft = begin() ;
      BSONObjIterator iterRight( rValue ) ;
      UINT32 idx = 0 ;

      while ( iterLeft != end() && iterRight.more() )
      {
         const BSONElement &beLeft = (*iterLeft)->_bound ;
         BOOLEAN inclusive = (*iterLeft)->_inclusive ;
         iterLeft ++ ;

         BSONElement beRight = iterRight.next() ;

         if ( idx >= startIdx )
         {
            INT32 res = beLeft.woCompare( beRight, FALSE ) ;

            if ( cmpFlag > 0 && res <= 0 )
            {
               // bigger is expected, but got smaller value
               if ( inclusive && res < 0 )
               {
                  return FALSE ;
               }
               else if ( !inclusive )
               {
                  return FALSE ;
               }
            }
            else if ( cmpFlag < 0 && res >= 0 )
            {
               // smaller is expected, but got bigger value
               if ( inclusive && res > 0 )
               {
                  return FALSE ;
               }
               else if ( !inclusive )
               {
                  return FALSE ;
               }
            }
            else if ( cmpFlag == 0 && res != 0 )
            {
               // equal is expected, but got non-equal value
               return FALSE ;
            }
         }
         idx ++ ;
      }

      if ( iterLeft == end() && !iterRight.more() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   string _optStatListKey::toString ()
   {
      if ( empty() )
      {
         return "{}" ;
      }

      BOOLEAN first = TRUE ;
      StringBuilder s ;
      s << "{ " ;

      iterator iter = begin() ;
      while ( iter != end() )
      {
         if ( first )
         {
            first = FALSE ;
         }
         else
         {
            s << ", " ;
         }

         (*iter)->_bound.toString( s, TRUE, TRUE ) ;

         iter ++ ;
      }

      s << " }" ;

      return s.str() ;
   }

   /*
      _optStatElementKey implement
    */
   _optStatElementKey::_optStatElementKey ( const BSONElement &element,
                                            BOOLEAN included )
   : BSONElement( element ),
     _dmsStatKey( included )
   {
   }

   INT32 _optStatElementKey::compareValue ( INT32 cmpFlag, INT32 incFlag,
                                            const BSONObj &rValue )
   {
      INT32 res = 0 ;
      BSONObjIterator iterRight( rValue ) ;

      if ( iterRight.more() )
      {
         BSONElement beRight = iterRight.next() ;
         res = woCompare( beRight, FALSE ) ;
      }

      if ( !_included && 0 == res && cmpFlag != 0 )
      {
         // Result is equal but not included, adjust with cmpFlag
         // cmpFlag is -1, left is bigger than right
         // cmpFlag is 1, left is smaller than right
         res = cmpFlag > 0 ? -1 : 1 ;
      }

      if ( 0 == res )
      {
         // Compared elements are equal, adjust with incFlag
         if ( iterRight.more() )
         {
            res = _equalButRightMore( incFlag ) ;
         }
         else if ( rValue.nFields() == 0 )
         {
            res = _equalButLeftMore( incFlag ) ;
         }
         else
         {
            res = _equalDefault( incFlag ) ;
         }
      }

      return res ;
   }

   BOOLEAN _optStatElementKey::compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                                  const BSONObj &rValue )
   {
      if ( startIdx > 0 )
      {
         return FALSE ;
      }

      BSONObjIterator iterRight( rValue ) ;

      if ( iterRight.more() )
      {
         BSONElement beRight = iterRight.next() ;
         INT32 res = woCompare( beRight, FALSE ) ;
         if ( cmpFlag > 0 && res <= 0 )
         {
            // bigger is expected, but got smaller value
            return FALSE ;
         }
         else if ( cmpFlag < 0 && res >= 0 )
         {
            // smaller is expected, but got bigger value
            return FALSE ;
         }
         else if ( cmpFlag == 0 && res != 0 )
         {
            // equal is expected, but got non-equal value
            return FALSE ;
         }
      }
      else
      {
         return FALSE ;
      }

      return TRUE ;
   }

   /*
      _optStatUnit implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTSTATUNIT_EVALPRED, "_optStatUnit::evalPredicate" )
   double _optStatUnit::evalPredicate ( const CHAR *pFieldName,
                                        rtnPredicate &predicate,
                                        BOOLEAN mixCmp,
                                        BOOLEAN &isAllRange ) const
   {
      double selectivity = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTSTATUNIT_EVALPRED ) ;

      if ( predicate.isEvaluated() )
      {
         isAllRange = predicate.isAllRange() ;
         selectivity = predicate.getSelectivity() ;
      }
      else
      {
         selectivity = 0.0 ;

         for ( RTN_SSKEY_LIST::iterator iterSSKey =
                                          predicate._startStopKeys.begin() ;
               iterSSKey != predicate._startStopKeys.end() ;
               iterSSKey ++ )
         {
            const rtnKeyBoundary &startKey = iterSSKey->_startKey ;
            const rtnKeyBoundary &stopKey = iterSSKey->_stopKey ;

            optStatElementKey beStart( startKey._bound, startKey._inclusive ) ;
            optStatElementKey beStop( stopKey._bound, stopKey._inclusive ) ;

            if ( beStart.type() == MinKey && beStop.type() == MaxKey )
            {
               isAllRange = TRUE ;
               break ;
            }

            BOOLEAN subIsEqual = iterSSKey->isEquality() ;
            INT32 majorType = iterSSKey->_majorType ;

            double subSelectivity = 1.0, dummy = 1.0 ;

            subSelectivity = evalKeyPair( pFieldName, beStart, beStop,
                                          subIsEqual, majorType, mixCmp,
                                          dummy ) ;
            selectivity += subSelectivity ;
         }

         if ( isAllRange )
         {
            selectivity = 1.0 ;
         }
         else
         {
            selectivity = OPT_ROUND_SELECTIVITY( selectivity ) ;
         }

         // Cache the selectivity to avoid duplicated evaluations
         predicate.setSelectivity( selectivity, isAllRange ) ;
      }

      PD_TRACE_EXIT( SDB__OPTSTATUNIT_EVALPRED ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTSTATUNIT__EVALKEYPAIR, "_optStatUnit::_evalKeyPair" )
   INT32 _optStatUnit::_evalKeyPair ( const dmsIndexStat *pIndexStat,
                                      rtnStatPredList::iterator &predIter,
                                      rtnStatPredList::iterator &endIter,
                                      optStatListKey &startKeys,
                                      optStatListKey &stopKeys,
                                      UINT32 keyLevelNum,
                                      UINT32 prefixEqualNum,
                                      BOOLEAN needCalcScanSel,
                                      double curPredSelectivity,
                                      double curScanSelectivity,
                                      double &predSelectivity,
                                      double &scanSelectivity ) const
   {
      SDB_ASSERT( isValid(), "Should not be invalid" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTSTATUNIT__EVALKEYPAIR ) ;

      BOOLEAN isEqual = keyLevelNum == prefixEqualNum ? TRUE : FALSE ;
      rtnStatPredList::iterator curPred = predIter ;

      if ( curPred == endIter )
      {
         if ( isEqual && startKeys.size() == pIndexStat->getNumKeys() )
         {
            rc = pIndexStat->evalETOperator( startKeys, curPredSelectivity, curScanSelectivity,
                                             predSelectivity, scanSelectivity ) ;
         }
         else
         {
            rc = pIndexStat->evalRangeOperator( startKeys, stopKeys,
                                                prefixEqualNum,
                                                curPredSelectivity,
                                                curScanSelectivity,
                                                predSelectivity,
                                                scanSelectivity ) ;
         }
      }
      else
      {
         rtnPredicate *pPredicate = *curPred ;
         rtnStatPredList::iterator nextPred = predIter ;
         ++ nextPred ;

         BOOLEAN startIncluded = startKeys.isIncluded() ;
         BOOLEAN stopIncluded = stopKeys.isIncluded() ;

         if ( pPredicate )
         {
            double tmpScanSel = 0.0, tmpPredSel = 0.0 ;
            for ( RTN_SSKEY_LIST::const_iterator iterSSKey =
                                          pPredicate->_startStopKeys.begin() ;
                  iterSSKey != pPredicate->_startStopKeys.end() ;
                  iterSSKey ++ )
            {
               BOOLEAN curIsEqual = iterSSKey->isEquality() ;
               BOOLEAN subIsEqual = isEqual && curIsEqual ;
               double subScanSel = 1.0, subPredSel = 1.0 ;
               double curKeySel = 1.0 ;

               // [$minKey, $minKey], [$minKey, $undefined), [$maxKey, $maxKey]
               // which could be ignored
               if ( ( iterSSKey->_startKey._bound.type() == MinKey &&
                      ( iterSSKey->_stopKey._bound.type() == MinKey ||
                      ( iterSSKey->_stopKey._bound.type() == Undefined &&
                        !( iterSSKey->_stopKey._inclusive ) ) ) ) ||
                    ( iterSSKey->_startKey._bound.type() == MaxKey &&
                      iterSSKey->_stopKey._bound.type() == MaxKey ) )
               {
                  continue ;
               }

               startKeys.pushKeyBound( &(iterSSKey->_startKey) ) ;
               stopKeys.pushKeyBound( &(iterSSKey->_stopKey) ) ;

               if ( iterSSKey->isEquality() )
               {
                  // predicate is an equal
                  curKeySel = _optEvalDefETSel( getTotalRecords(),
                                                iterSSKey->_startKey._bound ) ;
               }
               else if ( iterSSKey->_startKey._bound.type() != MinKey &&
                         iterSSKey->_stopKey._bound.type() != MaxKey )
               {
                  // predicate is a range
                  curKeySel = _optEvalDefRangeSel( iterSSKey->_startKey._bound,
                                                   iterSSKey->_startKey._inclusive,
                                                   iterSSKey->_stopKey._bound,
                                                   iterSSKey->_stopKey._inclusive ) ;
               }
               else if ( iterSSKey->_startKey._bound.type() != MinKey )
               {
                  // predicate is a greater-than
                  curKeySel = _optEvalDefGTSel( iterSSKey->_startKey._bound,
                                                iterSSKey->_startKey._inclusive ) ;
               }
               else if ( iterSSKey->_stopKey._bound.type() != MaxKey )
               {
                  // predicate is a less-than
                  curKeySel = _optEvalDefLTSel( iterSSKey->_stopKey._bound,
                                                iterSSKey->_stopKey._inclusive ) ;
               }

               double nextPredSel = curPredSelectivity * curKeySel ;
               double nextScanSel = ( needCalcScanSel ) ?
                                    ( ( curIsEqual || 0 == keyLevelNum ) ?
                                      ( curScanSelectivity * curKeySel ) :
                                      ( curScanSelectivity * curKeySel * OPT_IDX_SCAN_FAN_OUT ) ) :
                                    ( curScanSelectivity ) ;
               rc = _evalKeyPair( pIndexStat, nextPred, endIter,
                                  startKeys, stopKeys,
                                  keyLevelNum + 1,
                                  subIsEqual ? prefixEqualNum + 1 : prefixEqualNum,
                                  needCalcScanSel, nextPredSel, nextScanSel, subPredSel, subScanSel ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               tmpPredSel += subPredSel ;
               tmpScanSel += subScanSel ;

               startKeys.popElement( startIncluded ) ;
               stopKeys.popElement( stopIncluded ) ;
            }

            predSelectivity = OPT_ROUND_SELECTIVITY( tmpPredSel ) ;
            scanSelectivity = OPT_ROUND_SELECTIVITY( tmpScanSel ) ;
         }
         else
         {
            // full range
            static rtnKeyBoundary minKeyBound( minKey.firstElement(), TRUE ) ;
            static rtnKeyBoundary maxKeyBound( maxKey.firstElement(), TRUE ) ;

            startKeys.pushKeyBound( &minKeyBound ) ;
            stopKeys.pushKeyBound( &maxKeyBound ) ;

            rc = _evalKeyPair( pIndexStat, nextPred, endIter, startKeys, stopKeys,
                               keyLevelNum + 1, prefixEqualNum, FALSE,
                               curPredSelectivity, curScanSelectivity, predSelectivity, scanSelectivity ) ;

            startKeys.popElement( startIncluded ) ;
            stopKeys.popElement( stopIncluded ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__OPTSTATUNIT__EVALKEYPAIR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _optIndexStat implement
    */
   _optIndexStat::_optIndexStat ( const optCollectionStat &collectionStat,
                                  const ixmIndexCB &indexCB )
   : _optStatUnit( collectionStat.getTotalRecords( TRUE ) ),
     _collectionStat( collectionStat ),
     _pIndexStat( collectionStat.getIndexStat( indexCB.getLogicalID() ) ),
     _keyPattern( indexCB.keyPattern() ),
     _isUnique( indexCB.unique() )
   {
      _keyPattern = _keyPattern.getOwned() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTIDXSTAT_EVALPREDLIST, "_optIndexStat::evalPredicateList" )
   double _optIndexStat::evalPredicateList ( const CHAR *pFirstFieldName,
                                             rtnStatPredList &predList,
                                             BOOLEAN mixCmp,
                                             double &scanSelectivity ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double predSelectivity = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTIDXSTAT_EVALPREDLIST ) ;

      if ( predList.size() == 0 )
      {
         predSelectivity = 1.0 ;
         scanSelectivity = 1.0 ;

         goto done ;
      }
      else if ( isValid() )
      {
         rtnStatPredList::iterator predIter = predList.begin() ;
         rtnStatPredList::iterator endIter = predList.end() ;
         optStatListKey startKeys, stopKeys ;
         rc = _evalKeyPair( _pIndexStat, predIter, endIter, startKeys, stopKeys,
                            0, 0, TRUE,
                            OPT_PRED_DEFAULT_SELECTIVITY, OPT_PRED_DEFAULT_SELECTIVITY,
                            predSelectivity, scanSelectivity ) ;
      }

      if ( SDB_OK != rc )
      {
         rtnPredicate *pPredicate = predList.front() ;

         if ( pPredicate )
         {
            BOOLEAN isAllRange = FALSE ;
            predSelectivity = evalPredicate( pFirstFieldName, *pPredicate,
                                             mixCmp, isAllRange ) ;
         }
         else
         {
            predSelectivity = 1.0 ;
         }
         scanSelectivity = predSelectivity ;
      }

   done :
      PD_TRACE_EXIT( SDB__OPTIDXSTAT_EVALPREDLIST ) ;
      return predSelectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTIDXSTAT_EVALKEYPAIR, "_optIndexStat::evalKeyPair" )
   double _optIndexStat::evalKeyPair ( const CHAR *pFieldName,
                                       dmsStatKey &startKey,
                                       dmsStatKey &stopKey,
                                       BOOLEAN isEqual,
                                       INT32 majorType,
                                       BOOLEAN mixCmp,
                                       double &scanSelectivity ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double predSelectivity = OPT_PRED_DEFAULT_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTIDXSTAT_EVALKEYPAIR ) ;

      if ( isValid() )
      {
         if ( isEqual && startKey.size() == _pIndexStat->getNumKeys() )
         {
            FLOAT64 curSelectivity = _optStatGetEQSel( startKey.size() ) ;
            rc = _pIndexStat->evalETOperator( startKey,
                                              curSelectivity,
                                              curSelectivity,
                                              predSelectivity,
                                              scanSelectivity ) ;
         }
         else
         {
            FLOAT64 curSelectivity = OPT_PRED_DEFAULT_SELECTIVITY ;
            FLOAT64 curScanFanOut = 1.0 ;
            if ( isEqual )
            {
               curSelectivity = _optStatGetEQSel( startKey.size() ) ;
            }
            else
            {
               curSelectivity = _optStatGetRangeSel( startKey.size() ) ;
               curScanFanOut = startKey.size() > 1 ? OPT_IDX_SCAN_FAN_OUT : 1.0 ;
            }
            rc = _pIndexStat->evalRangeOperator( startKey, stopKey, 0,
                                                 curSelectivity,
                                                 OPT_ROUND_SELECTIVITY( curSelectivity * curScanFanOut ),
                                                 predSelectivity,
                                                 scanSelectivity ) ;
         }
      }

      if ( SDB_OK != rc )
      {
         // Simply evaluate one
         predSelectivity = _collectionStat.evalKeyPair( pFieldName,
                                                        startKey, stopKey,
                                                        isEqual, majorType,
                                                        mixCmp,
                                                        scanSelectivity ) ;
      }

      PD_TRACE_EXIT( SDB__OPTIDXSTAT_EVALKEYPAIR ) ;

      return predSelectivity ;
   }

   /*
      _optCollectionStat implement
    */
   _optCollectionStat::_optCollectionStat ( UINT32 pageSizeLog2,
                                            _dmsMBContext * mbContext,
                                            const _optAccessPlanHelper & helper,
                                            const dmsStatCache * statCache )
   : _optStatUnit( 0 ),
     _pageSizeLog2( pageSizeLog2 ),
     _totalDataPages( 0 ),
     _totalDataSize( 0 ),
     _numIndexes( 0 ),
     _totalIndexPages( 0 ),
     _totalIndexSize( 0 ),
     _avgIndexPages( 0 ),
     _avgIndexSize( 0 ),
     _pCollectionStat( NULL ),
     _bestIndexStat( NULL )
   {
      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      _initCurrentStat( mbContext ) ;
      _initHistoryStat( mbContext, helper, statCache ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCLSTAT__INITCURSTAT, "_optCollectionStat::_initCurrentStat" )
   void _optCollectionStat::_initCurrentStat( _dmsMBContext *mbContext )
   {
      PD_TRACE_ENTRY( SDB_OPTCLSTAT__INITCURSTAT ) ;

      _totalRecords = mbContext->mbStat()->_totalRecords.fetch() ;
      _totalDataPages = mbContext->mbStat()->_totalDataPages ;
      _totalDataSize = mbContext->mbStat()->_totalOrgDataLen.fetch() ;

      _numIndexes = mbContext->mb()->_numIndexes ;
      if ( _numIndexes > 0 )
      {
         _totalIndexPages = mbContext->mbStat()->_totalIndexPages ;
         _totalIndexSize = ( (UINT64)_totalIndexPages << _pageSizeLog2 ) -
                           mbContext->mbStat()->_totalIndexFreeSpace ;
         _avgIndexPages =
               (UINT32)ceil( (double)_totalIndexPages / (double)_numIndexes ) ;
         _avgIndexSize =
               OPT_ROUND_NUM( (UINT64)ceil( (double)_totalIndexSize /
                                            (double)_numIndexes ) ) ;
      }

      PD_TRACE_EXIT( SDB_OPTCLSTAT__INITCURSTAT ) ;
   }

   void _optCollectionStat::_initHistoryStat (
                                    _dmsMBContext * mbContext,
                                    const _optAccessPlanHelper & planHelper,
                                    const dmsStatCache * statCache )
   {
      UINT32 pageSize = getPageSize() ;
      UINT32 estPages = ossRoundUpToMultipleX( _totalDataSize, pageSize ) / pageSize ;
      if ( ( NULL == statCache ) ||
           ( (INT32)_totalDataPages <= planHelper.getOptCostThreshold() &&
             (INT32)estPages <= planHelper.getOptCostThreshold() ) )
      {
         // no cache or collection is too small to use statistics
         return ;
      }
      // for statistics cache, mbID is ID of cache unit
      _pCollectionStat = (const dmsCollectionStat *)
                           statCache->getCacheUnit( mbContext->mbID() ) ;
      if ( NULL != _pCollectionStat &&
           optCheckStatExpiredByPage( _totalDataPages,
                                _pCollectionStat->getTotalDataPages(),
                                planHelper.getOptCostThreshold(),
                                _pageSizeLog2 ) )
      {
         // if the statistics is expired, ignore it
         PD_LOG( PDDEBUG, "Statistics for collection [%s.%s] is expired, "
                 "current pages [%d], statistics pages [%d], "
                 "cost threshold [%d]", _pCollectionStat->getCSName(),
                 _pCollectionStat->getCLName(), _totalDataPages,
                 _pCollectionStat->getTotalDataPages(),
                 planHelper.getOptCostThreshold() ) ;
         _pCollectionStat = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALPREDSET, "_optCollectionStat::evalPredicateSet" )
   double _optCollectionStat::evalPredicateSet ( rtnPredicateSet &predicateSet,
                                                 BOOLEAN mixCmp,
                                                 double &scanSelectivity,
                                                 optIndexPathEncoder &encoder )
   {
      double selectivity = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT_EVALPREDSET ) ;

      RTN_PREDICATE_MAP predicates = predicateSet.predicates() ;
      const dmsIndexStat *pIndexStat = NULL ;

      if ( predicates.size() == 0 )
      {
         selectivity = 1.0 ;
         scanSelectivity = 1.0 ;
         goto done ;
      }

      if ( isValid() )
      {
         pIndexStat = _getMatchedIndex( predicateSet ) ;
      }

      if ( pIndexStat )
      {
         rtnStatPredList predicateList ;
         BSONObjIterator iterKey( pIndexStat->getKeyPattern() ) ;
         while ( iterKey.more() )
         {
            BSONElement beKey = iterKey.next() ;
            const CHAR *pFieldName = beKey.fieldName() ;
            RTN_PREDICATE_MAP::iterator iterPred = predicates.find( pFieldName ) ;
            if ( iterPred == predicates.end() )
            {
               predicateList.push_back( NULL ) ;
               encoder.append( pFieldName, TRUE ) ;
            }
            else
            {
               predicateList.push_back( &( iterPred->second ) ) ;
               encoder.append( pFieldName, FALSE ) ;
            }
         }

         rtnStatPredList::iterator predIter = predicateList.begin() ;
         rtnStatPredList::iterator endIter = predicateList.end() ;
         optStatListKey startKeys, stopKeys ;
         INT32 rc = _optStatUnit::_evalKeyPair( pIndexStat, predIter, endIter,
                                                startKeys, stopKeys,
                                                0, 0, TRUE, OPT_PRED_DEFAULT_SELECTIVITY,
                                                OPT_PRED_DEFAULT_SELECTIVITY,
                                                selectivity,
                                                scanSelectivity ) ;

         if ( SDB_OK == rc )
         {
            _setBestIndex( pIndexStat ) ;
            goto done ;
         }
      }

      // Need to evaluate one by one
      selectivity = 1.0 ;
      for ( RTN_PREDICATE_MAP::iterator iterPred = predicates.begin() ;
            iterPred != predicates.end();
            iterPred ++ )
      {
         BOOLEAN isAllRange = FALSE ;
         const CHAR *pFieldName = iterPred->first.c_str() ;
         rtnPredicate &predicate = iterPred->second ;

         double curSelectivity = 1.0 ;
         curSelectivity = evalPredicate( pFieldName, predicate, mixCmp,
                                         isAllRange ) ;
         selectivity *= curSelectivity ;
      }

      scanSelectivity = selectivity ;

   done :
      PD_TRACE_EXIT( SDB__OPTCLSTAT_EVALPREDSET ) ;
      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALKEYPAIR, "_optCollectionStat::evalKeyPair" )
   double _optCollectionStat::evalKeyPair ( const CHAR *pFieldName,
                                            dmsStatKey &startKey,
                                            dmsStatKey &stopKey,
                                            BOOLEAN isEqual,
                                            INT32 majorType,
                                            BOOLEAN mixCmp,
                                            double &scanSelectivity ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double predSelectivity = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT_EVALKEYPAIR ) ;

      // Use the first field only
      const BSONElement &beStart = startKey.firstElement() ;
      const BSONElement &beStop = stopKey.firstElement() ;
      BOOLEAN startIncluded = startKey.isIncluded() ;
      BOOLEAN stopIncluded = stopKey.isIncluded() ;

      // Try to use the field statistics first
      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat )
      {
         if ( isEqual && pIndexStat->getNumKeys() == 1 )
         {
            optStatElementKey eleKey( beStart, TRUE ) ;
            rc = pIndexStat->evalETOperator( eleKey,
                                             OPT_PRED_EQ_DEF_SELECTIVITY,
                                             OPT_PRED_EQ_DEF_SELECTIVITY,
                                             predSelectivity, scanSelectivity ) ;
         }
         else
         {
            // First key only
            optStatElementKey startEleKey( beStart, startIncluded ) ;
            optStatElementKey stopEleKey( beStop, stopIncluded ) ;
            double tmpSel = isEqual ? OPT_PRED_EQ_DEF_SELECTIVITY : OPT_PRED_RANGE_DEF_SELECTIVITY ;
            rc = pIndexStat->evalRangeOperator( startEleKey, stopEleKey,
                                                isEqual ? 1 : 0, tmpSel, tmpSel,
                                                predSelectivity, scanSelectivity ) ;
         }
      }

      // Failed to use field statistics, evaluate by default
      if ( SDB_OK != rc )
      {
         if ( isEqual )
         {
            predSelectivity = _evalETOperator( beStart ) ;
         }
         else
         {
            predSelectivity = _evalKeyPair( beStart, startIncluded,
                                            beStop, stopIncluded, majorType,
                                            mixCmp ) ;
         }
         scanSelectivity = predSelectivity ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT_EVALKEYPAIR ) ;

      return predSelectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALETOPTR, "_optCollectionStat::evalETOpterator" )
   double _optCollectionStat::evalETOpterator ( const CHAR *pFieldName,
                                                const BSONElement &beValue ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double selectivity = 1.0, dummy = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT_EVALETOPTR ) ;

      // Try to use the field statistics first
      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat && pIndexStat->isValidForEstimate() )
      {
         optStatElementKey statKey( beValue, TRUE ) ;
         if ( pIndexStat->getNumKeys() == 1 )
         {
            rc = pIndexStat->evalETOperator( statKey, OPT_PRED_EQ_DEF_SELECTIVITY,
                                             OPT_PRED_EQ_DEF_SELECTIVITY,
                                             selectivity, dummy ) ;
         }
         else
         {
            rc = pIndexStat->evalRangeOperator( statKey, statKey, 1,
                                                OPT_PRED_EQ_DEF_SELECTIVITY,
                                                OPT_PRED_EQ_DEF_SELECTIVITY,
                                                selectivity, dummy ) ;
         }
      }

      if ( SDB_OK != rc )
      {
         // Failed to use field statistics, evaluate by default
         selectivity = _evalETOperator( beValue ) ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT_EVALETOPTR ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALGTOPTR, "_optCollectionStat::evalGTOpterator" )
   double _optCollectionStat::evalGTOpterator ( const CHAR *pFieldName,
                                                const BSONElement &beValue,
                                                BOOLEAN included ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double selectivity = 1.0, dummy = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT_EVALGTOPTR ) ;

      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat && pIndexStat->isValidForEstimate() )
      {
         optStatElementKey statKey( beValue, included ) ;
         rc = pIndexStat->evalGTOperator( statKey, selectivity, dummy ) ;
      }

      if ( SDB_OK != rc )
      {
         // Failed to use field statistics, evaluate by default
         selectivity = _evalGTOperator( beValue, included ) ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT_EVALGTOPTR ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALLTOPTR, "_optCollectionStat::evalLTOpterator" )
   double _optCollectionStat::evalLTOpterator ( const CHAR *pFieldName,
                                                const BSONElement &beValue,
                                                BOOLEAN included ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double selectivity = 1.0, dummy = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT_EVALLTOPTR ) ;

      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat && pIndexStat->isValidForEstimate() )
      {
         optStatElementKey statKey( beValue, included ) ;
         rc = pIndexStat->evalLTOperator( statKey, selectivity, dummy ) ;
      }

      if ( SDB_OK != rc )
      {
         // Failed to use field statistics, evaluate by default
         selectivity = _evalLTOperator( beValue, included ) ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT_EVALLTOPTR ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT__EVALKEYPAIR, "_optCollectionStat::_evalKeyPair" )
   double _optCollectionStat::_evalKeyPair ( const BSONElement &startKey,
                                             BOOLEAN startIncluded,
                                             const BSONElement &stopKey,
                                             BOOLEAN stopIncluded,
                                             INT32 majorType,
                                             BOOLEAN mixCmp ) const
   {
      double selectivity = OPT_PRED_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT__EVALKEYPAIR ) ;

      if ( ( startKey.type() == MinKey ||
             startKey.type() == Undefined ) &&
           stopKey.type() == MaxKey )
      {
         // Cover all values
         selectivity = 1.0 ;
      }
      else if ( ( startKey.type() == MinKey &&
                  ( stopKey.type() == MinKey ||
                    ( stopKey.type() == Undefined && !stopIncluded ) ) ) ||
                ( startKey.type() == MaxKey && stopKey.type() == MaxKey ) )
      {
         // [$minKey, $minKey], [$minKey, $undefined), [$maxKey, $maxKey]
         // which could be ignored
         selectivity = 0.0 ;
      }
      else if ( startKey.type() == MinKey ||
                startKey.type() == Undefined )
      {
         selectivity = _evalLTOperator( stopKey, stopIncluded ) ;
      }
      else if ( stopKey.type() == MaxKey )
      {
         selectivity = _evalGTOperator( startKey, startIncluded ) ;
      }

      else if ( mixCmp )
      {
         selectivity = _evalRangeOperator( startKey, startIncluded,
                                           stopKey, stopIncluded ) ;
      }
      else
      {
         BSONElement minEle, maxEle ;

         if ( majorType == RTN_KEY_MAJOR_DEFAULT )
         {
            minEle = rtnKeyGetMinForCmp( startKey.type(), FALSE ).firstElement() ;
            maxEle = rtnKeyGetMaxForCmp( stopKey.type(), FALSE ).firstElement() ;
         }
         else
         {
            minEle = rtnKeyGetMinForCmp( majorType, FALSE ).firstElement() ;
            maxEle = rtnKeyGetMaxForCmp( majorType, FALSE ).firstElement() ;
         }

         BOOLEAN sameType = ( minEle.canonicalType() == maxEle.canonicalType() ) ;
         BOOLEAN startIsMin = ( 0 == startKey.woCompare( minEle, FALSE ) ) ;
         BOOLEAN stopIsMax = ( 0 == stopKey.woCompare( maxEle, FALSE ) ) ;

         if ( startIsMin && stopIsMax &&
              ( ( startIncluded && stopIncluded ) || !sameType ) )
         {
            selectivity = 1.0 ;
         }
         else if ( startIsMin && startIncluded )
         {
            selectivity = _evalLTOperator( stopKey, stopIncluded ) ;
         }
         else if ( stopIsMax && stopIncluded )
         {
            selectivity = _evalGTOperator( startKey, startIncluded ) ;
         }
         else
         {
            selectivity = _evalRangeOperator( startKey, startIncluded,
                                              stopKey, stopIncluded ) ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT__EVALKEYPAIR ) ;

      return selectivity ;
   }

   double _optCollectionStat::_evalETOperator ( const BSONElement &beValue ) const
   {
      return _optEvalDefETSel( getTotalRecords(), beValue ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTEVALDFTETOPTR, "_optEvalDefETSel" )
   FLOAT64 _optEvalDefETSel( UINT64 recordNum, const BSONElement &beValue )
   {
      FLOAT64 selectivity = OPT_PRED_EQ_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTEVALDFTETOPTR ) ;

      if ( beValue.type() == Bool )
      {
         selectivity = 0.5 ;
      }
      else if ( recordNum > 0 )
      {
         // Assume that each 2 records have different values
         selectivity = OSS_MIN( OPT_PRED_EQ_DEF_SELECTIVITY,
                                OPT_PRED_EQ_DEF_NUM_KEYS / (double)recordNum ) ;
      }

      PD_TRACE_EXIT( SDB__OPTEVALDFTETOPTR ) ;

      return selectivity ;
   }

   double _optCollectionStat::_evalRangeOperator ( const BSONElement &beStart,
                                                   BOOLEAN startIncluded,
                                                   const BSONElement &beStop,
                                                   BOOLEAN stopIncluded ) const
   {
      return _optEvalDefRangeSel( beStart, startIncluded, beStop, stopIncluded ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTEVALDFTRANGEOPTR, "_optEvalDefRangeSel" )
   FLOAT64 _optEvalDefRangeSel( const BSONElement &beStart, BOOLEAN startIncluded,
                                const BSONElement &beStop, BOOLEAN stopIncluded )
   {
      double selectivity = OPT_PRED_RANGE_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTEVALDFTRANGEOPTR ) ;

      if ( beStart.canonicalType() == beStop.canonicalType() )
      {
         switch ( beStart.type() )
         {
            case Bool :
            {
               if ( startIncluded && stopIncluded )
               {
                  // [ true, false ] is all range
                  selectivity = 1.0 ;
               }
               else if ( startIncluded || stopIncluded )
               {
                  // Either true or false
                  selectivity = 0.5 ;
               }
               else
               {
                  // No matched
                  selectivity = 0.0 ;
               }
               break ;
            }
            case NumberDouble :
            case NumberInt :
            case NumberLong :
            case NumberDecimal :
            {
               double start = OPT_ROUND_BSON_NUM( beStart.number() ) ;
               double stop = OPT_ROUND_BSON_NUM( beStop.number() ) ;
               selectivity = fabs( stop - start ) /
                             ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN ) ;
               break ;
            }
            case Timestamp :
            case Date :
            {
               double start = OPT_ROUND_BSON_NUM( beStart.number() ) ;
               double stop = OPT_ROUND_BSON_NUM( beStop.number() ) ;
               selectivity = fabs( stop - start ) /
                             ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN ) ;
               break ;
            }
            case String :
            {
               UINT32 startSize = beStart.valuestrsize() ;
               const CHAR *pStartStr = beStart.valuestr() ;
               startSize = OSS_MIN( startSize, OPT_BSON_STR_MIN_LEN ) ;

               UINT32 stopSize = beStop.valuestrsize() ;
               const CHAR *pStopStr = beStop.valuestr() ;
               stopSize = OSS_MIN( stopSize, OPT_BSON_STR_MIN_LEN ) ;

               selectivity = fabs( optConvertStrToScalar( pStopStr, stopSize,
                                                          OPT_BSON_STR_MIN,
                                                          OPT_BSON_STR_MAX ) -
                                   optConvertStrToScalar( pStartStr, startSize,
                                                          OPT_BSON_STR_MIN,
                                                          OPT_BSON_STR_MAX ) ) ;
               break ;
            }
            case Undefined:
            case jstNULL:
            {
               // special case for null values
               selectivity = OPT_PRED_NULL_DEF_SELECTIVITY ;
               break ;
            }
            default :
            {
               selectivity = OPT_PRED_RANGE_DEF_SELECTIVITY ;
               break ;
            }
         }
      }
      else
      {
         if ( ( Undefined == beStop.type() ) ||
              ( jstNULL == beStop.type() ) )
         {
            // special case for null values
            selectivity = OPT_PRED_NULL_DEF_SELECTIVITY ;
         }
         else
         {
            // Start key and stop key are different types
            selectivity = OPT_PRED_DEF_SELECTIVITY ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTEVALDFTRANGEOPTR ) ;

      return OPT_ROUND_SELECTIVITY(
                  OSS_MAX( selectivity, OPT_PRED_RANGE_MIN_SELECTIVITY ) ) ;
   }

   double _optCollectionStat::_evalGTOperator ( const BSONElement &beStart,
                                                BOOLEAN startIncluded ) const
   {
      return _optEvalDefGTSel( beStart, startIncluded ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTEVALDFTGTOPTR, "_optEvalDefGTSel" )
   FLOAT64 _optEvalDefGTSel( const BSONElement &beStart, BOOLEAN startIncluded )
   {
      double selectivity = OPT_PRED_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTEVALDFTGTOPTR ) ;

      switch ( beStart.type() )
      {
         case Bool :
         {
            if ( beStart.boolean() )
            {
               if ( startIncluded )
               {
                  // >= true
                  selectivity = 0.5 ;
               }
               else
               {
                  // > true
                  selectivity = 0.0 ;
               }
            }
            else
            {
               if ( startIncluded )
               {
                  // >= false
                  selectivity = 1.0 ;
               }
               else
               {
                  // > false
                  selectivity = 0.5 ;
               }
            }
            break ;
         }
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
         {
            double start = OPT_ROUND_BSON_NUM( beStart.number() ) ;
            selectivity = ( OPT_BSON_NUM_MAX - start ) /
                          ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN ) ;
            break ;
         }
         case Timestamp :
         case Date :
         {
            double start = OPT_ROUND_BSON_NUM( beStart.number() ) ;
            selectivity = ( OPT_BSON_NUM_MAX - start ) /
                          ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN ) ;
            break ;
         }
         case String :
         {
            UINT32 strSize = beStart.valuestrsize() ;
            const CHAR *pStr = beStart.valuestr() ;
            strSize = OSS_MIN( strSize, OPT_BSON_STR_MIN_LEN ) ;
            selectivity = 1.0 - optConvertStrToScalar( pStr, strSize,
                                                       OPT_BSON_STR_MIN,
                                                       OPT_BSON_STR_MAX ) ;
            break ;
         }
         default :
         {
            selectivity = OPT_PRED_DEF_SELECTIVITY ;
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTEVALDFTGTOPTR ) ;

      return OPT_ROUND_SELECTIVITY(
                  OSS_MAX( selectivity, OPT_PRED_GTORLT_MIN_SELECTIVITY ) ) ;
   }

   double _optCollectionStat::_evalLTOperator ( const BSONElement &beStop,
                                                BOOLEAN stopIncluded ) const
   {
      return _optEvalDefLTSel( beStop, stopIncluded ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTEVALDFTLTOPTR, "_optEvalDefLTSel" )
   FLOAT64 _optEvalDefLTSel( const BSONElement &beStop, BOOLEAN stopIncluded )
   {
      double selectivity = OPT_PRED_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTEVALDFTLTOPTR ) ;

      switch ( beStop.type() )
      {
         case Bool :
         {
            if ( beStop.boolean() )
            {
               if ( stopIncluded )
               {
                  // <= true
                  selectivity = 1.0 ;
               }
               else
               {
                  // < true
                  selectivity = 0.5 ;
               }
            }
            else
            {
               if ( stopIncluded )
               {
                  // <= false
                  selectivity = 0.5 ;
               }
               else
               {
                  // < false
                  selectivity = 0.0 ;
               }
            }
            break ;
         }
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
         {
            double stop = OPT_ROUND_BSON_NUM( beStop.number() ) ;
            selectivity = ( stop - OPT_BSON_NUM_MIN ) /
                          ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN ) ;
            break ;
         }
         case Timestamp :
         case Date :
         {
            double stop = OPT_ROUND_BSON_NUM( beStop.number() ) ;
            selectivity = ( stop - OPT_BSON_NUM_MIN ) /
                          ( OPT_BSON_NUM_MAX - OPT_BSON_NUM_MIN );
            break ;
         }
         case String :
         {
            UINT32 strSize = beStop.valuestrsize() ;
            const CHAR *pStr = beStop.valuestr() ;
            strSize = OSS_MIN( strSize, OPT_BSON_STR_MIN_LEN ) ;
            selectivity = optConvertStrToScalar( pStr, strSize,
                                                 OPT_BSON_STR_MIN,
                                                 OPT_BSON_STR_MAX ) ;
            break ;
         }
         default :
         {
            selectivity = OPT_PRED_DEF_SELECTIVITY ;
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTEVALDFTLTOPTR ) ;

      return OPT_ROUND_SELECTIVITY(
                  OSS_MAX( selectivity, OPT_PRED_GTORLT_MIN_SELECTIVITY ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCLSTAT_GETMATCHEDIDX, "_optCollectionStat::_getMatchedIndex" )
   const dmsIndexStat * _optCollectionStat::_getMatchedIndex ( rtnPredicateSet &predicateSet ) const
   {
      PD_TRACE_ENTRY( SDB_OPTCLSTAT_GETMATCHEDIDX ) ;

      if ( !isValid() )
      {
         return NULL ;
      }

      RTN_PREDICATE_MAP &predicates = predicateSet.predicates() ;

      if ( predicates.size() == 0 )
      {
         return NULL ;
      }
      else if ( predicates.size() == 1 )
      {
         RTN_PREDICATE_MAP::const_iterator iterPred = predicates.begin() ;
         return getFieldStat( iterPred->first.c_str() ) ;
      }

      const dmsIndexStat *pBestIndexStat = NULL ;
      const INDEX_STAT_MAP &indexStats = _pCollectionStat->getIndexStats() ;

      for ( INDEX_STAT_CONST_ITERATOR iter = indexStats.begin() ;
            iter != indexStats.end() ;
            ++ iter )
      {
         const dmsIndexStat *pIndexStat = iter->second ;

         if ( pIndexStat->getNumKeys() < predicates.size() )
         {
            // The index could not cover all predicates, it is not the best
            // matched index
            continue ;
         }

         BSONObjIterator iterKey( pIndexStat->getKeyPattern() ) ;
         UINT32 matchedCount = 0 ;
         while ( iterKey.more() )
         {
            BSONElement beKey = iterKey.next() ;
            RTN_PREDICATE_MAP::iterator iterPred = predicates.find( beKey.fieldName() ) ;
            if ( iterPred == predicates.end() )
            {
               if ( 0 == matchedCount )
               {
                  break ;
               }
               continue ;
            }
            matchedCount ++ ;
         }

         if ( matchedCount == predicates.size() )
         {
            // The index covers all predicates, which is a candidate of the best
            // matched index
            if ( matchedCount == pIndexStat->getNumKeys() )
            {
               // The number of keys are matched, it is the best one
               pBestIndexStat = pIndexStat ;
               goto done ;
            }

            // The number of keys are different, find the smaller one
            if ( pBestIndexStat )
            {
               if ( pIndexStat->getNumKeys() < pBestIndexStat->getNumKeys() )
               {
                  pBestIndexStat = pIndexStat ;
               }
            }
            else
            {
               pBestIndexStat = pIndexStat ;
            }
         }
      }

   done :
      PD_TRACE_EXIT( SDB_OPTCLSTAT_GETMATCHEDIDX ) ;
      return pBestIndexStat ;
   }

   /*
      Helper functions implement
    */
   double optConvertStrToScalar ( const CHAR *pValue, UINT32 valueSize,
                                  UINT8 low, UINT8 high )
   {
      if ( 0 == valueSize )
      {
         // Empty string
         return 0.0 ;
      }

      UINT8 base = high - low + 1;
      double scalar = 0.0 ;
      double denom = base ;

      // Convert initial characters to fraction
      while ( valueSize-- > 0 )
      {
         UINT8 ch = (UINT8) *( pValue++ ) ;

         ch = OPT_ROUND( ch, low, high ) ;
         scalar += ( (double) (ch - low) ) / denom ;
         denom *= base ;
      }

      return scalar ;
   }

   #define OPT_EXPIRED_THRESOLD     ( 8589934592L )   // 8 GB
   #define OPT_EXPIRED_SMALL_STEP   ( 268435456 )     // 256 MB
   #define OPT_EXPIRED_LARGE_STEP   ( 1073741824 )    // 1 GB
   #define OPT_EXPIRED_QUICK_STEP   ( ( OPT_EXPIRED_SMALL_STEP ) / \
                                      ( DMS_PAGE_SIZE_MAX ) )

   BOOLEAN optCheckStatExpiredByPage( UINT32 currentPages,
                                      UINT32 statPages,
                                      UINT32 costThreshold,
                                      UINT32 pageSizeLog2 )
   {
      // CASE 1:   current and history number of pages are the same, always not
      //           be expired
      // CASE 2:   both current and history number of pages are smaller than
      //           cost threshold, means the collection is small, always not
      //           be expired
      // CASE 3:   one of current or history number of pages are larger than
      //           cost threshold, will expire the history statistics
      // CASE 4:   for case that both current and history number of pages are
      //           bigger than the cost threshold,
      // CASE 4.1: for <= 8 GB, increasing each 256 MB will be expired
      // CASE 4.2: for > 8 GB, increasing each 1 GB will be expired
      // CASE 4.3: quick check for CASE 4: if increasing number of pages is
      //           smaller than 256 MB / 64 KB, must not be expired
      if ( currentPages == statPages ||
           ( currentPages <= costThreshold && statPages <= costThreshold ) )
      {
         // CASE 1, CASE 2
         return FALSE ;
      }
      else if ( ( currentPages > costThreshold &&
                  statPages <= costThreshold ) ||
                ( currentPages <= costThreshold &&
                  statPages > costThreshold ) )
      {
         // CASE 3
         return TRUE ;
      }
      else if ( currentPages < statPages + OPT_EXPIRED_QUICK_STEP )
      {
         // CASE 4.3
         return FALSE ;
      }
      else if ( ( (UINT64)currentPages << pageSizeLog2 ) <
                OPT_EXPIRED_THRESOLD )
      {
         // CASE 4.1
         return ( currentPages > statPages +
                                 ( OPT_EXPIRED_SMALL_STEP >> pageSizeLog2 ) ) ;
      }
      // CASE 4.2
      return ( currentPages > statPages +
                              ( OPT_EXPIRED_LARGE_STEP >> pageSizeLog2 ) ) ;
   }

   BOOLEAN optCheckStatExpiredBySize( UINT32 currentDataSize,
                                      UINT32 statDataSize,
                                      UINT32 costThreshold,
                                      UINT32 pageSizeLog2 )
   {
      // CASE 1:   current and history number of pages are the same, always not
      //           be expired
      // CASE 2:   both current and history number of pages are smaller than
      //           cost threshold, means the collection is small, always not
      //           be expired
      // CASE 3:   one of current or history number of pages are larger than
      //           cost threshold, will expire the history statistics
      // CASE 4:   for case that both current and history number of pages are
      //           bigger than the cost threshold,
      // CASE 4.1: for <= 8 GB, increasing each 256 MB will be expired
      // CASE 4.2: for > 8 GB, increasing each 1 GB will be expired
      // CASE 4.3: quick check for CASE 4: if increasing number of pages is
      //           smaller than 256 MB / 64 KB, must not be expired
      UINT32 pageSize = 1 << pageSizeLog2 ;
      UINT32 currentPages = ossRoundUpToMultipleX( currentDataSize, pageSize ) / pageSize ;
      UINT32 statPages = ossRoundUpToMultipleX( statDataSize, pageSize ) / pageSize ;
      return optCheckStatExpiredByPage( currentPages, statPages, costThreshold, pageSizeLog2 ) ;
   }

}
