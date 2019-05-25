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
#include "ixmExtent.hpp"
#include "optCommon.hpp"

namespace engine
{

   /*
      Helper functions define
    */
   static double optConvertStrToScalar ( const CHAR *pValue, UINT32 valueSize,
                                         UINT8 low, UINT8 high ) ;

   /*
      _optStatListKey implement
    */
   _optStatListKey::_optStatListKey ()
   : _utilList<const rtnKeyBoundary *> (),
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
            res = cmpFlag > 0 ? -1 : 1 ;
            break ;
         }
      }

      if ( 0 == res )
      {
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
         res = cmpFlag > 0 ? -1 : 1 ;
      }

      if ( 0 == res )
      {
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
            return FALSE ;
         }
         else if ( cmpFlag < 0 && res >= 0 )
         {
            return FALSE ;
         }
         else if ( cmpFlag == 0 && res != 0 )
         {
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

         predicate.setSelectivity( selectivity, isAllRange ) ;
      }

      PD_TRACE_EXIT( SDB__OPTSTATUNIT_EVALPRED ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTSTATUNIT__EVALKEYPAIR, "_optStatUnit::_evalKeyPair" )
   INT32 _optStatUnit::_evalKeyPair ( const dmsIndexStat *pIndexStat,
                                      rtnStatPredList::iterator &predIter,
                                      optStatListKey &startKeys,
                                      optStatListKey &stopKeys,
                                      BOOLEAN isEqual,
                                      double &predSelectivity,
                                      double &scanSelectivity ) const
   {
      SDB_ASSERT( isValid(), "Should not be invalid" ) ;

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTSTATUNIT__EVALKEYPAIR ) ;

      rtnStatPredList::iterator curPred = predIter ;
      rtnStatPredList::iterator nextPred = ++ predIter ;
      if ( curPred == nextPred )
      {
         if ( isEqual && startKeys.size() == pIndexStat->getNumKeys() )
         {
            rc = pIndexStat->evalETOperator( startKeys, predSelectivity, scanSelectivity ) ;
         }
         else
         {
            rc = pIndexStat->evalRangeOperator( startKeys, stopKeys,
                                                predSelectivity, scanSelectivity ) ;
         }
      }
      else
      {
         rtnPredicate *pPredicate = *curPred ;

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
               BOOLEAN subIsEqual = isEqual && iterSSKey->isEquality() ;
               double subScanSel = 1.0, subPredSel = 1.0 ;

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

               rc = _evalKeyPair( pIndexStat, nextPred,
                                  startKeys, stopKeys,
                                  subIsEqual, subPredSel, subScanSel ) ;
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
            static rtnKeyBoundary minKeyBound( minKey.firstElement(), TRUE ) ;
            static rtnKeyBoundary maxKeyBound( maxKey.firstElement(), TRUE ) ;

            startKeys.pushKeyBound( &minKeyBound ) ;
            stopKeys.pushKeyBound( &maxKeyBound ) ;

            rc = _evalKeyPair( pIndexStat, nextPred, startKeys, stopKeys,
                               FALSE, predSelectivity, scanSelectivity ) ;

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
     _pIndexStat( collectionStat.getIndexStat( indexCB.getName() ) ),
     _keyPattern( indexCB.keyPattern() )
   {
      _keyPattern = _keyPattern.getOwned() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTIDXSTAT_EVALPREDLIST, "_optStatUnit::evalPredicateList" )
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
         optStatListKey startKeys, stopKeys ;
         rc = _evalKeyPair( _pIndexStat, predIter, startKeys, stopKeys,
                            TRUE, predSelectivity, scanSelectivity ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTIDXSTAT_EVALKEYPAIR, "_optStatUnit::evalKeyPair" )
   double _optIndexStat::evalKeyPair ( const CHAR *pFieldName,
                                       dmsStatKey &startKey,
                                       dmsStatKey &stopKey,
                                       BOOLEAN isEqual,
                                       INT32 majorType,
                                       BOOLEAN mixCmp,
                                       double &scanSelectivity ) const
   {
      INT32 rc = SDB_INVALIDARG ;
      double predSelectivity = 1.0 ;

      PD_TRACE_ENTRY( SDB__OPTIDXSTAT_EVALKEYPAIR ) ;

      if ( isValid() )
      {
         if ( isEqual && startKey.size() == _pIndexStat->getNumKeys() )
         {
            rc = _pIndexStat->evalETOperator( startKey, predSelectivity, scanSelectivity ) ;
         }
         else
         {
            rc = _pIndexStat->evalRangeOperator( startKey, stopKey,
                                                 predSelectivity,
                                                 scanSelectivity ) ;
         }
      }

      if ( SDB_OK != rc )
      {
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
   _optCollectionStat::_optCollectionStat ( UINT32 pageSize,
                                            _dmsMBContext *mbContext,
                                            const dmsStatCache *statCache )
   : _optStatUnit( 0 ),
     _pageSize( pageSize ),
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
      if ( statCache )
      {
         _pCollectionStat = (const dmsCollectionStat *)
                            statCache->getCacheUnit( mbContext->mbID() ) ;
      }
      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      initCurStat( mbContext ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCLSTAT_INITCURSTAT, "_optCollectionStat::initCurStat" )
   INT32 _optCollectionStat::initCurStat( _dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTCLSTAT_INITCURSTAT ) ;

      _totalRecords = mbContext->mbStat()->_totalRecords ;
      _totalDataPages = mbContext->mbStat()->_totalDataPages ;
      _totalDataSize = mbContext->mbStat()->_totalOrgDataLen ;

      _numIndexes = mbContext->mb()->_numIndexes ;
      if ( _numIndexes > 0 )
      {
         _totalIndexPages = mbContext->mbStat()->_totalIndexPages ;
         _totalIndexSize = mbContext->mbStat()->_totalIndexPages * _pageSize -
                           mbContext->mbStat()->_totalIndexFreeSpace ;
         _avgIndexPages = (UINT32)ceil( (double)_totalIndexPages / (double)_numIndexes ) ;
         _avgIndexSize = OPT_ROUND_NUM( (UINT64)ceil( (double)_totalIndexSize / (double)_numIndexes ) ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTCLSTAT_INITCURSTAT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT_EVALPREDSET, "_optCollectionStat::evalPredicateSet" )
   double _optCollectionStat::evalPredicateSet ( rtnPredicateSet &predicateSet,
                                                 BOOLEAN mixCmp,
                                                 double &scanSelectivity )
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
            if ( iterPred == predicates.end() ||
                 iterPred->second.isEmpty() )
            {
               predicateList.push_back( NULL ) ;
            }
            else
            {
               predicateList.push_back( &( iterPred->second ) ) ;
            }
         }

         rtnStatPredList::iterator predIter = predicateList.begin() ;
         optStatListKey startKeys, stopKeys ;
         INT32 rc = _optStatUnit::_evalKeyPair( pIndexStat, predIter,
                                                startKeys, stopKeys,
                                                TRUE, selectivity,
                                                scanSelectivity ) ;

         if ( SDB_OK == rc )
         {
            _setBestIndex( pIndexStat ) ;
            goto done ;
         }
      }

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

      const BSONElement &beStart = startKey.firstElement() ;
      const BSONElement &beStop = stopKey.firstElement() ;
      BOOLEAN startIncluded = startKey.isIncluded() ;
      BOOLEAN stopIncluded = stopKey.isIncluded() ;

      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat )
      {
         if ( isEqual && pIndexStat->getNumKeys() == 1 )
         {
            optStatElementKey eleKey( beStart, TRUE ) ;
            rc = pIndexStat->evalETOperator( eleKey, predSelectivity, scanSelectivity ) ;
         }
         else
         {
            optStatElementKey startEleKey( beStart, startIncluded ) ;
            optStatElementKey stopEleKey( beStop, stopIncluded ) ;
            rc = pIndexStat->evalRangeOperator( startEleKey, stopEleKey,
                                                predSelectivity, scanSelectivity ) ;
         }
      }

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

      const dmsIndexStat *pIndexStat = getFieldStat( pFieldName ) ;
      if ( pIndexStat && pIndexStat->isValidForEstimate() )
      {
         optStatElementKey statKey( beValue, TRUE ) ;
         if ( pIndexStat->getNumKeys() == 1 )
         {
            rc = pIndexStat->evalETOperator( statKey, selectivity, dummy ) ;
         }
         else
         {
            rc = pIndexStat->evalRangeOperator( statKey, statKey, selectivity, dummy ) ;
         }
      }

      if ( SDB_OK != rc )
      {
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
         selectivity = 1.0 ;
      }
      else if ( ( startKey.type() == MinKey &&
                  ( stopKey.type() == MinKey ||
                    ( stopKey.type() == Undefined && !stopIncluded ) ) ) ||
                ( startKey.type() == MaxKey && stopKey.type() == MaxKey ) )
      {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT__EVALETOPTR, "_optCollectionStat::_evalETOperator" )
   double _optCollectionStat::_evalETOperator ( const BSONElement &beValue ) const
   {
      double selectivity = OPT_PRED_EQ_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT__EVALETOPTR ) ;

      if ( beValue.type() == Bool )
      {
         selectivity = 0.5 ;
      }
      else if ( getTotalRecords() > 0 )
      {
         selectivity = OSS_MIN( OPT_PRED_EQ_DEF_SELECTIVITY,
                                1.0 / (double)getTotalRecords() ) ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT__EVALETOPTR ) ;

      return selectivity ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT__EVALRANGEOPTR, "_optCollectionStat::_evalRangeOperator" )
   double _optCollectionStat::_evalRangeOperator ( const BSONElement &beStart,
                                                   BOOLEAN startIncluded,
                                                   const BSONElement &beStop,
                                                   BOOLEAN stopIncluded ) const
   {
      double selectivity = OPT_PRED_RANGE_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT__EVALRANGEOPTR ) ;

      if ( beStart.canonicalType() == beStop.canonicalType() )
      {
         switch ( beStart.type() )
         {
            case Bool :
            {
               if ( startIncluded && stopIncluded )
               {
                  selectivity = 1.0 ;
               }
               else if ( startIncluded || stopIncluded )
               {
                  selectivity = 0.5 ;
               }
               else
               {
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
            default :
            {
               selectivity = OPT_PRED_RANGE_DEF_SELECTIVITY ;
               break ;
            }
         }
      }
      else
      {
         selectivity = OPT_PRED_DEF_SELECTIVITY ;
      }

      PD_TRACE_EXIT( SDB__OPTCLSTAT__EVALRANGEOPTR ) ;

      return OPT_ROUND_SELECTIVITY( selectivity ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT__EVALGTOPTR, "_optCollectionStat::_evalGTOperator" )
   double _optCollectionStat::_evalGTOperator ( const BSONElement &beStart,
                                                BOOLEAN startIncluded ) const
   {
      double selectivity = OPT_PRED_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT__EVALGTOPTR ) ;

      switch ( beStart.type() )
      {
         case Bool :
         {
            if ( beStart.boolean() )
            {
               if ( startIncluded )
               {
                  selectivity = 0.5 ;
               }
               else
               {
                  selectivity = 0.0 ;
               }
            }
            else
            {
               if ( startIncluded )
               {
                  selectivity = 1.0 ;
               }
               else
               {
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

      PD_TRACE_EXIT( SDB__OPTCLSTAT__EVALGTOPTR ) ;

      return OPT_ROUND_SELECTIVITY( selectivity ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTCLSTAT__EVALLTOPTR, "_optCollectionStat::_evalLTOperator" )
   double _optCollectionStat::_evalLTOperator ( const BSONElement &beStop,
                                                BOOLEAN stopIncluded ) const
   {
      double selectivity = OPT_PRED_DEF_SELECTIVITY ;

      PD_TRACE_ENTRY( SDB__OPTCLSTAT__EVALLTOPTR ) ;

      switch ( beStop.type() )
      {
         case Bool :
         {
            if ( beStop.boolean() )
            {
               if ( stopIncluded )
               {
                  selectivity = 1.0 ;
               }
               else
               {
                  selectivity = 0.5 ;
               }
            }
            else
            {
               if ( stopIncluded )
               {
                  selectivity = 0.5 ;
               }
               else
               {
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

      PD_TRACE_EXIT( SDB__OPTCLSTAT__EVALLTOPTR ) ;

      return OPT_ROUND_SELECTIVITY( selectivity ) ;
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
            continue ;
         }

         BSONObjIterator iterKey( pIndexStat->getKeyPattern() ) ;
         UINT32 matchedCount = 0 ;
         while ( iterKey.more() )
         {
            BSONElement beKey = iterKey.next() ;
            RTN_PREDICATE_MAP::iterator iterPred = predicates.find( beKey.fieldName() ) ;
            if ( iterPred == predicates.end() ||
                 iterPred->second.isEmpty() )
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
            if ( matchedCount == pIndexStat->getNumKeys() )
            {
               pBestIndexStat = pIndexStat ;
               goto done ;
            }

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
         return 0.0 ;
      }

      UINT8 base = high - low + 1;
      double scalar = 0.0 ;
      double denom = base ;

      while ( valueSize-- > 0 )
      {
         UINT8 ch = (UINT8) *( pValue++ ) ;

         ch = OPT_ROUND( ch, low, high ) ;
         scalar += ( (double) (ch - low) ) / denom ;
         denom *= base ;
      }

      return scalar ;
   }
}
