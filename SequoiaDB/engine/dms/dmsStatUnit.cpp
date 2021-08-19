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

   Source File Name = dmsStatUnit.cpp

   Descriptive Name = DMS Statistics Units

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   statistics objects.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dmsStatUnit.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "msgDef.hpp"

using namespace bson ;

namespace engine
{

   #define DMS_STAT_CREATE_TIME               FIELD_NAME_CREATE_TIME
   #define DMS_STAT_SAMPLE_RECORDS            "SampleRecords"
   #define DMS_STAT_TOTAL_RECORDS             FIELD_NAME_TOTAL_RECORDS

   #define DMS_STAT_FIELD_FRAC_NAME           FIELD_NAME_FRAC

   #define DMS_STAT_CL_TOTAL_DATA_PAGES       FIELD_NAME_TOTAL_DATA_PAGES
   #define DMS_STAT_CL_TOTAL_DATA_SIZE        FIELD_NAME_TOTAL_DATA_SIZE
   #define DMS_STAT_CL_AVG_NUM_FIELDS         "AvgNumFields"

   #define DMS_STAT_IDX_KEY_PATTERN           FIELD_NAME_KEY_PATTERN
   #define DMS_STAT_IDX_INDEX_PAGES           "IndexPages"
   #define DMS_STAT_IDX_LEVELS                "IndexLevels"
   #define DMS_STAT_IDX_IS_UNIQUE             "IsUnique"
   #define DMS_STAT_IDX_DISTINCT_VALUES       "DistinctValues"
   #define DMS_STAT_IDX_NULL_FRAC             FIELD_NAME_NULL_FRAC
   #define DMS_STAT_IDX_UNDEF_FRAC            FIELD_NAME_UNDEF_FRAC
   #define DMS_STAT_IDX_MCV_VALUES            FIELD_NAME_VALUES
   #define DMS_STAT_IDX_MCV_FRAC              DMS_STAT_FIELD_FRAC_NAME
   #define DMS_STAT_IDX_HISTOGRAM             "Histogram"
   #define DMS_STAT_IDX_HISTOGRAM_FRAC        DMS_STAT_FIELD_FRAC_NAME
   #define DMS_STAT_IDX_HISTOGRAM_BOUNDS      "Bounds"
   #define DMS_STAT_IDX_TYPE_SET              "TypeSet"
   #define DMS_STAT_IDX_TYPE_SET_TYPES        "Types"
   #define DMS_STAT_IDX_TYPE_SET_FRAC         DMS_STAT_FIELD_FRAC_NAME

   #define DMS_STAT_CHECKHOLE_THRESHOLD       ( 10 )


   /*
      _dmsStatValues implement
    */
   _dmsStatValues::_dmsStatValues ()
   : _numKeys( 0 ),
     _size( 0 ),
     _allocSize( 0 ),
     _pValues( NULL )
   {
   }

   _dmsStatValues::~_dmsStatValues ()
   {
      _clear() ;
   }

   INT32 _dmsStatValues::init ( UINT32 size, UINT32 allocSize )
   {
      INT32 rc = SDB_OK ;

      if ( allocSize < size )
      {
         allocSize = size ;
      }

      if ( allocSize == 0 )
      {
         goto done ;
      }

      _pValues = new(std::nothrow) BSONObj[ allocSize ] ;
      PD_CHECK( _pValues, SDB_OOM, error, PDWARNING,
                "Failed to allocate %u values", allocSize ) ;

      _size = size ;
      _allocSize = allocSize ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStatValues::pushBack ( const BSONObj &boValue )
   {
      if ( _size == _allocSize )
      {
         return SDB_OOM ;
      }
      _pValues[ _size ] = boValue.getOwned() ;
      _size ++ ;
      return SDB_OK ;
   }

   INT32 _dmsStatValues::binarySearch ( dmsStatKey &keyValue, INT32 cmpFlag,
                                        INT32 keyIncFlag, BOOLEAN &isEqual ) const
   {
      isEqual = FALSE ;

      INT32 low = 0, high = _size - 1, mid = 0 ;
      INT32 index = -1 ;
      BSONObj boEmpty ;

      // The output idx means:
      // 1. idx is 0
      //    1. isEqual is true, value == _pValues[0]
      //    2. isEqual is false, value < _pValues[0]
      // 2. idx is 1 to _size - 1
      //    1. isEqual is true, value == _pValues[idx]
      //    2. isEqual is false, _pValues[idx-1]< value < _pValues[idx]
      // 3. idx is _size: _pValues[_size - 1] < value

      if ( 0 == _size )
      {
         return -1 ;
      }

      while ( low <= high )
      {
         mid = ( low + high ) / 2 ;

         INT32 res = keyValue.compareValue( cmpFlag, keyIncFlag,
                                            _pValues[ mid ] ) ;

         if ( 0 == res )
         {
            index = mid ;
            isEqual = TRUE ;
            break ;
         }
         else if ( res > 0 )
         {
            index = mid + 1 ;
            low = mid + 1 ;
         }
         else
         {
            high = mid - 1 ;
            index = mid ;
         }
      }

      return index ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSTAT_CHKVALS, "_dmsStatValues::checkValues" )
   INT32 _dmsStatValues::checkValues ( UINT32 numKeys, const BSONObj &keyPattern )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSTAT_CHKVALS ) ;

      for ( UINT32 idx = 0 ; idx < _size ; idx ++ )
      {
         PD_CHECK( numKeys >= (UINT32)_pValues[idx].nFields(),
                   SDB_INVALIDARG, error, PDWARNING, "Number of keys are not "
                   "matched, index: %u, expected: %u, actual: %d",
                   idx, numKeys, _pValues[idx].nFields() ) ;
         if ( numKeys == (UINT32)_pValues[idx].nFields() )
         {
            BSONObjIterator iterKey( keyPattern ) ;
            BSONObjIterator iterValue( _pValues[idx] ) ;
            while( iterKey.more() && iterValue.more() )
            {
               const BSONElement beKey = iterKey.next() ;
               const BSONElement beValue = iterValue.next() ;

               PD_CHECK( 0 == ossStrcmp( beKey.fieldName(), beValue.fieldName() ),
                         SDB_INVALIDARG, error, PDWARNING,
                         "Key names are not matched, expected: %s, actual: %s",
                         beKey.fieldName(), beValue.fieldName() ) ;
            }
            PD_CHECK( !iterKey.more() && !iterValue.more(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Number of keys are not matched" ) ;
         }
         else
         {
            BSONObjIterator iterKey( keyPattern ) ;
            BSONObjIterator iterValue( _pValues[idx] ) ;
            BSONObjBuilder valueBuilder ;
            while( iterKey.more() )
            {
               const BSONElement beKey = iterKey.next() ;
               if ( iterValue.more() )
               {
                  const BSONElement beValue = iterValue.next() ;

                  PD_CHECK( 0 == ossStrcmp( beKey.fieldName(), beValue.fieldName() ),
                            SDB_INVALIDARG, error, PDWARNING,
                            "Key names are not matched, expected: %s, actual: %s",
                            beKey.fieldName(), beValue.fieldName() ) ;

                  valueBuilder.append( beValue ) ;
               }
               else
               {
                  valueBuilder.appendUndefined( beKey.fieldName() ) ;
               }
            }
            PD_CHECK( !iterValue.more() && !iterKey.more(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Number of keys are not matched" ) ;
            _pValues[idx] = valueBuilder.obj() ;
         }
      }

      _numKeys = numKeys ;

   done :
      PD_TRACE_EXIT( SDB__DMSIDXSTAT_CHKVALS ) ;
      return rc ;
   error :
      goto done ;
   }

   void _dmsStatValues::_clear ()
   {
      if ( _pValues )
      {
         delete [] _pValues ;
         _pValues = NULL ;
      }
      _size = 0 ;
   }

   BOOLEAN _dmsStatValues::_inRange ( UINT32 idx, dmsStatKey *pStartKey,
                                      dmsStatKey *pStopKey ) const
   {
      if ( idx > _size )
      {
         return FALSE ;
      }

      if ( idx == _size )
      {
         return pStopKey ? FALSE : TRUE ;
      }

      if ( pStartKey )
      {
         // Expect start key < value[idx]
         // The first element is compared in earlier range comparison
         if ( !pStartKey->compareAllValues( 1, -1, _pValues[idx] ) )
         {
            return FALSE ;
         }
      }
      if ( pStopKey )
      {
         // Expect stop key > value[idx]
         // The first element is compared in earlier range comparison
         if ( !pStopKey->compareAllValues( 1, 1, _pValues[idx] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   /*
      _dmsStatMCVSet implement
    */
   _dmsStatMCVSet::_dmsStatMCVSet ()
   : _dmsStatValues (),
     _pFractions( NULL ),
     _totalFrac( 0 )
   {
   }

   _dmsStatMCVSet::~_dmsStatMCVSet ()
   {
      if ( _pFractions )
      {
         delete [] _pFractions ;
         _pFractions = NULL ;
      }
   }

   INT32 _dmsStatMCVSet::init ( UINT32 size, UINT32 allocSize )
   {
      INT32 rc = SDB_OK ;

      if ( allocSize < size )
      {
         allocSize = size ;
      }

      if ( allocSize == 0 )
      {
         goto done ;
      }

      rc = _dmsStatValues::init( size, allocSize ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to init values, rc: %d", rc ) ;

      _pFractions = new(std::nothrow)UINT16[ allocSize ] ;
      PD_CHECK( _pFractions, SDB_OOM, error, PDWARNING,
                "Failed to allocate memory for %u fractions",
                allocSize ) ;

      _totalFrac = 0 ;

   done :
      return rc ;
   error :
      _clear() ;
      goto done ;
   }

   INT32 _dmsStatMCVSet::pushBack ( const BSONObj &boValue, UINT16 fraction )
   {
      INT32 rc = _dmsStatValues::pushBack( boValue ) ;

      if ( SDB_OK == rc )
      {
         SDB_ASSERT( _size > 0, "_size is invalid" ) ;
         _pFractions[ _size - 1 ] = fraction ;
      }

      return rc ;
   }

   void _dmsStatMCVSet::clear ()
   {
      if ( _pFractions )
      {
         delete [] _pFractions ;
         _pFractions = NULL ;
      }
      _totalFrac = 0 ;
      _dmsStatValues::_clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATMCV_EVALOPTR, "_dmsStatMCVSet::evalOperator" )
   INT32 _dmsStatMCVSet::evalOperator ( dmsStatKey *pStartKey,
                                        dmsStatKey *pStopKey,
                                        BOOLEAN &hitMCV,
                                        double &predSelectivity,
                                        double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATMCV_EVALOPTR ) ;

      BOOLEAN startIncluded = FALSE, stopIncluded = FALSE ;
      BOOLEAN startEqual = FALSE, stopEqual = FALSE ;
      INT32 startFlag = 0, stopFlag = 0 ;
      INT32 startIdx = -1, stopIdx = -1 ;
      UINT32 rangeCount = 0 ;
      UINT16 tmpPredSel = 0, tmpScanSel = 0 ;

      BOOLEAN checkHoles = FALSE ;

      PD_CHECK( getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      PD_CHECK( ( pStartKey && pStopKey && pStartKey->size() == pStopKey->size() ) ||
                  pStartKey || pStopKey, SDB_INVALIDARG, error, PDWARNING,
                  "Numbers of keys are not matched" ) ;

      // Only check holes when number of keys are larger than 1
      if ( _numKeys > 1 && ( ( pStartKey && pStartKey->size() > 1 ) ||
                             ( pStopKey && pStopKey->size() > 1 ) ) )
      {
         checkHoles = TRUE ;
      }

      // Get the index of start key
      if ( pStartKey )
      {
         startIncluded = pStartKey->isIncluded() ;
         startFlag = startIncluded ? -1 : 1 ;
         startIdx = binarySearch( *pStartKey, -1, startFlag, startEqual ) ;
         PD_CHECK( startIdx >= 0, SDB_INVALIDARG, error, PDWARNING,
                   "Failed to locate start key %s in MCV set",
                   pStartKey->toString().c_str() ) ;
      }
      else
      {
         // $lt operator, include all smaller MCV items
         startIdx = 0 ;
         startIncluded = TRUE ;
         startEqual = FALSE ;
      }

      // Get the index of stop key
      if ( pStopKey )
      {
         stopIncluded = pStopKey->isIncluded() ;
         stopFlag = stopIncluded ? 1 : -1 ;
         stopIdx = binarySearch( *pStopKey, 1, stopFlag, stopEqual ) ;
         PD_CHECK( stopIdx >= 0, SDB_INVALIDARG, error, PDWARNING,
                   "Failed to locate stop key %s in MCV set",
                   pStopKey->toString().c_str() ) ;
      }
      else
      {
         // $gt operator, include all greater MCV items
         stopIdx = getSize() ;
         stopIncluded = TRUE ;
         stopEqual = FALSE ;
      }

      // Don't need to check holes if the range is too large
      if ( checkHoles && ( stopIdx - startIdx > DMS_STAT_CHECKHOLE_THRESHOLD ) )
      {
         checkHoles = FALSE ;
      }

      if ( startIdx != stopIdx )
      {
         // Check startIdx
         if ( startIncluded || !startEqual )
         {
            // The start key <= value case, add the first selected MCV item
            tmpScanSel += getFracInt( startIdx ) ;
            if ( checkHoles && ( ( startIncluded && startEqual ) ||
                                 _inRange( (UINT32)startIdx, pStartKey, pStopKey ) ) )
            {
               tmpPredSel += getFracInt( startIdx ) ;
            }
            rangeCount ++ ;
         }

         // Check startIdx + 1 to stopIdx - 1
         for ( INT32 idx = startIdx + 1 ; idx < stopIdx ; idx ++ )
         {
            // Every ranges between selected MCV items are needed
            tmpScanSel += getFracInt( idx ) ;
            if ( checkHoles && _inRange( idx, pStartKey, pStopKey ) )
            {
               tmpPredSel += getFracInt( idx ) ;
            }
            rangeCount ++ ;
         }

         // Check stopIdx
         if ( stopIncluded && stopEqual && stopIdx < (INT32)getSize() )
         {
            // The stop key is equal to the last selected MCV item, add
            // the last selected MCV item
            tmpScanSel += getFracInt( stopIdx ) ;
            if ( checkHoles )
            {
               tmpPredSel += getFracInt( stopIdx ) ;
            }
            rangeCount ++ ;
         }
      }
      else
      {
         // start idx is equal to stop idx, which means the start key and stop
         // key are equal to the same MCV item
         if ( startIncluded && startEqual && stopIncluded && stopEqual &&
              startIdx < (INT32)getSize() )
         {
            // The stop key is equal to the selected MCV item, which should
            // be included
            tmpScanSel = getFracInt( startIdx ) ;
            if ( checkHoles )
            {
               tmpPredSel += getFracInt( startIdx ) ;
            }
            rangeCount ++ ;
         }
      }

      scanSelectivity = (double)tmpScanSel / (double)DMS_STAT_FRACTION_SCALE  ;
      scanSelectivity = DMS_STAT_ROUND_SELECTIVITY( scanSelectivity ) ;

      if ( checkHoles )
      {
         predSelectivity = (double)tmpPredSel / (double)DMS_STAT_FRACTION_SCALE ;
         predSelectivity = DMS_STAT_ROUND_SELECTIVITY( predSelectivity ) ;
      }
      else
      {
         predSelectivity = scanSelectivity ;
      }
      hitMCV = rangeCount > 0 ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATMCV_EVALOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATMCV_EVALETOPTR, "_dmsStatMCVSet::evalETOperator" )
   INT32 _dmsStatMCVSet::evalETOperator ( dmsStatKey &key,
                                          BOOLEAN &hitMCV,
                                          double &predSelectivity,
                                          double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATMCV_EVALETOPTR );

      BOOLEAN equal = FALSE ;
      INT32 idx = -1 ;
      double tmpPredSel = 0.0 ;

      PD_CHECK( getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      idx = binarySearch( key, 0, 0, equal ) ;
      PD_CHECK( idx >= 0, SDB_INVALIDARG, error, PDWARNING,
                "Failed to locate start key %s in MCV set",
                key.toString().c_str() ) ;

      if ( idx < (INT32)getSize() && equal )
      {
         tmpPredSel = getFrac( idx ) ;
         hitMCV = TRUE ;
      }
      else
      {
         hitMCV = FALSE ;
      }

      predSelectivity = DMS_STAT_ROUND_SELECTIVITY( tmpPredSel ) ;
      scanSelectivity = predSelectivity ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATMCV_EVALETOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _dmsStatUnit implement
    */
   _dmsStatUnit::_dmsStatUnit ()
   : _utilSUCacheUnit(),
     _sampleRecords( DMS_STAT_DEF_TOTAL_RECORDS ),
     _totalRecords( DMS_STAT_DEF_TOTAL_RECORDS ),
     _suLogicalID( DMS_INVALID_LOGICCSID ),
     _clLogicalID( DMS_INVALID_CLID )
   {
   }

   _dmsStatUnit::_dmsStatUnit ( UINT32 suLID, UINT16 mbID, UINT32 clLID,
                                UINT64 createTime )
   : _utilSUCacheUnit( mbID, createTime ),
     _sampleRecords( DMS_STAT_DEF_TOTAL_RECORDS ),
     _totalRecords( DMS_STAT_DEF_TOTAL_RECORDS ),
     _suLogicalID( suLID ),
     _clLogicalID( clLID )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATBASE_INIT, "_dmsStatUnit::init" )
   INT32 _dmsStatUnit::init ( const BSONObj &boStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATBASE_INIT ) ;

      try
      {
         BSONElement beItem ;

         // Required fields

         beItem = boStat.getField( DMS_STAT_COLLECTION_SPACE ) ;
         PD_CHECK( String == beItem.type(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", DMS_STAT_COLLECTION_SPACE ) ;
         setCSName( beItem.valuestr() ) ;

         beItem = boStat.getField( DMS_STAT_COLLECTION ) ;
         PD_CHECK( String == beItem.type(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", DMS_STAT_COLLECTION ) ;
         setCLName( beItem.valuestr() ) ;

         beItem = boStat.getField( DMS_STAT_CREATE_TIME ) ;
         PD_CHECK( beItem.isNumber(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", DMS_STAT_CREATE_TIME ) ;
         setCreateTime( (UINT64)beItem.numberLong() ) ;

         beItem = boStat.getField( DMS_STAT_SAMPLE_RECORDS ) ;
         PD_CHECK( beItem.isNumber(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", DMS_STAT_SAMPLE_RECORDS ) ;
         setSampleRecords( (UINT64)beItem.numberLong() ) ;

         beItem = boStat.getField( DMS_STAT_TOTAL_RECORDS ) ;
         PD_CHECK( beItem.isNumber(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", DMS_STAT_TOTAL_RECORDS ) ;
         setTotalRecords( (UINT64)beItem.numberLong() ) ;

         rc = _initItem( boStat ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to initialize items, rc: %d", rc ) ;

         rc = postInit() ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to process after initialization, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize statistics object, received "
                 "unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATBASE_INIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _dmsStatUnit::postInit ()
   {
      return _postInit() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATBASE_TOBSON, "_dmsStatUnit::toBSON" )
   BSONObj _dmsStatUnit::toBSON () const
   {
      BSONObjBuilder builder ;

      PD_TRACE_ENTRY( SDB_DMSSTATBASE_TOBSON ) ;

      builder.append( DMS_STAT_COLLECTION_SPACE, getCSName() ) ;
      builder.append( DMS_STAT_COLLECTION, getCLName() ) ;
      builder.append( DMS_STAT_CREATE_TIME, (INT64)getCreateTime() ) ;
      builder.append( DMS_STAT_SAMPLE_RECORDS, (INT64)getSampleRecords() ) ;
      builder.append( DMS_STAT_TOTAL_RECORDS, (INT64)getTotalRecords() ) ;

      _toBSON( builder ) ;

      PD_TRACE_EXIT( SDB_DMSSTATBASE_TOBSON ) ;

      return builder.obj() ;
   }

   /*
      _dmsIndexStat implement
    */
   _dmsIndexStat::_dmsIndexStat ()
   : _dmsStatUnit (),
     _indexLogicalID( DMS_INVALID_EXTENT ),
     _pFirstField( NULL ),
     _numKeys( 0 ),
     _indexPages( DMS_STAT_DEF_TOTAL_PAGES ),
     _indexLevels( DMS_STAT_DEF_IDX_LEVELS ),
     _isUnique( FALSE ),
     _distinctValues( 0 ),
     _nullFrac( 0 ),
     _undefFrac( 0 ),
     _mcvSet()
   {
      ossMemset( _pCSName, 0, sizeof( _pCSName ) ) ;
      ossMemset( _pCLName, 0, sizeof( _pCLName ) ) ;

      setIndexName( NULL ) ;
   }

   _dmsIndexStat::_dmsIndexStat ( const CHAR *pCSName, const CHAR *pCLName,
                                  const CHAR *pIndexName, UINT32 suLID,
                                  UINT16 mbID, UINT32 clLID,
                                  UINT64 createTime )
   : _dmsStatUnit( suLID, mbID, clLID, createTime ),
     _indexLogicalID( DMS_INVALID_EXTENT ),
     _pFirstField( NULL ),
     _numKeys( 0 ),
     _indexPages( DMS_STAT_DEF_TOTAL_PAGES ),
     _indexLevels( DMS_STAT_DEF_IDX_LEVELS ),
     _isUnique( FALSE ),
     _distinctValues( 0 ),
     _nullFrac( 0 ),
     _undefFrac( 0 ),
     _mcvSet()
   {
      ossMemset( _pCSName, 0, sizeof( _pCSName ) ) ;
      ossMemset( _pCLName, 0, sizeof( _pCLName ) ) ;

      setCSName( pCSName ) ;
      setCLName( pCLName ) ;
      setIndexName( pIndexName ) ;
   }

   _dmsIndexStat::~_dmsIndexStat ()
   {
   }

   INT32 _dmsIndexStat::initMCVSet ( UINT32 allocSize )
   {
      _mcvSet.clear() ;
      return _mcvSet.init( 0, allocSize ) ;
   }

   INT32 _dmsIndexStat::pushMCVSet ( const BSONObj &boValue, double fraction )
   {
      INT32 rc = SDB_OK ;

      BSONObjBuilder keyBuilder ;
      BSONObj boFullValue ;

      UINT16 scaledFraction = 0 ;

      BSONObjIterator iterKey( _keyPattern ) ;
      BSONObjIterator iterCur ( boValue ) ;
      while ( iterKey.more() && iterCur.more() )
      {
         BSONElement beKey = iterKey.next() ;
         BSONElement beCur = iterCur.next() ;

         switch ( beCur.type() )
         {
            case NumberDouble :
            case NumberInt :
            case NumberLong :
            case NumberDecimal :
            case String :
            case Bool :
            case Date :
            case Timestamp :
            case jstOID :
            case jstNULL :
            case Undefined :
               break ;
            default :
               // Ignore non-supported types
               goto done ;
         }

         keyBuilder.appendAs( beCur, beKey.fieldName() ) ;
      }

      PD_CHECK( !iterKey.more() && !iterCur.more(), SDB_SYS, error, PDERROR,
                "Size of keys are not matched" ) ;

      boFullValue = keyBuilder.obj() ;

      // Round to 0 ~ 1.0 and scaled to 10000x
      fraction = DMS_STAT_ROUND_SELECTIVITY( fraction ) *
                 DMS_STAT_FRACTION_SCALE ;
      // Round to integer
      scaledFraction = (UINT16)DMS_STAT_ROUND_INT( fraction ) ;
      rc = _mcvSet.pushBack( boFullValue, scaledFraction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert mcv value [%s], rc: %d",
                   boFullValue.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT_EVALRANGEOPTR, "_dmsIndexStat::evalRangeOperator" )
   INT32 _dmsIndexStat::evalRangeOperator ( dmsStatKey &startKey,
                                            dmsStatKey &stopKey,
                                            double &predSelectivity,
                                            double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT_EVALRANGEOPTR ) ;

      rc = _evalOperator( &startKey, &stopKey, predSelectivity, scanSelectivity ) ;

      PD_TRACE_EXITRC( SDB_DMSIDXSTAT_EVALRANGEOPTR, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT_EVALETOPTR, "_dmsIndexStat::evalETOperator" )
   INT32 _dmsIndexStat::evalETOperator ( dmsStatKey &key,
                                         double &predSelectivity,
                                         double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT_EVALETOPTR );

      BOOLEAN hitMCV = FALSE ;

      // Special case for unique index, could be one of the totalRecords
      if ( _isUnique && key.size() == _numKeys )
      {
         predSelectivity = 1.0 / (double)_totalRecords ;
         scanSelectivity = predSelectivity ;
         goto done ;
      }

      PD_CHECK( _mcvSet.getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      rc = _mcvSet.evalETOperator( key, hitMCV, predSelectivity, scanSelectivity ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to evaluate from MCV set, rc: %d", rc ) ;

      if ( !hitMCV )
      {
         // The value is not in MCV set, evaluate in the rest of values
         if ( _distinctValues == _mcvSet.getSize() )
         {
            predSelectivity = ( 1.0 - _mcvSet.getTotalFrac() ) *
                              DMS_STAT_PRED_EQ_DEF_SELECTIVITY ;
         }
         else
         {
            predSelectivity = ( 1.0 - _mcvSet.getTotalFrac() ) /
                              (double) ( _distinctValues - _mcvSet.getSize() ) ;
         }
         scanSelectivity = predSelectivity ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT_EVALETOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT_EVALGTOPTR, "_dmsIndexStat::evalGTOperator" )
   INT32 _dmsIndexStat::evalGTOperator ( dmsStatKey &startKey,
                                         double &predSelectivity,
                                         double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT_EVALGTOPTR );

      rc = _evalOperator( &startKey, NULL, predSelectivity, scanSelectivity ) ;

      PD_TRACE_EXITRC( SDB_DMSIDXSTAT_EVALGTOPTR, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT_EVALLTOPTR, "_dmsIndexStat::evalLTOperator" )
   INT32 _dmsIndexStat::evalLTOperator ( dmsStatKey &stopKey,
                                         double &predSelectivity,
                                         double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT_EVALLTOPTR );

      rc = _evalOperator( NULL, &stopKey, predSelectivity, scanSelectivity ) ;

      PD_TRACE_EXITRC( SDB_DMSIDXSTAT_EVALLTOPTR, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__EVALOPTR, "_dmsIndexStat::_evalOperator" )
   INT32 _dmsIndexStat::_evalOperator ( dmsStatKey *pStartKey,
                                        dmsStatKey *pStopKey,
                                        double &predSelectivity,
                                        double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__EVALOPTR ) ;

      BOOLEAN hitMCV = FALSE ;

      PD_CHECK( _mcvSet.getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" ) ;

      rc = _mcvSet.evalOperator( pStartKey, pStopKey,
                                 hitMCV, predSelectivity, scanSelectivity ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to evaluate from MCV set, rc: %d", rc ) ;

      if ( !hitMCV )
      {
         // The values do not include any MCV items, try to evaluate from
         // the rest of values
         predSelectivity = ( 1.0 - _mcvSet.getTotalFrac() ) *
                           DMS_STAT_PRED_RANGE_DEF_SELECTIVITY ;
         scanSelectivity = predSelectivity ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT__EVALOPTR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__INITITEM, "_dmsIndexStat::_initItem" )
   INT32 _dmsIndexStat::_initItem ( const BSONObj &boStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__INITITEM ) ;

      BSONElement beItem ;

      // Required fields

      beItem = boStat.getField( DMS_STAT_IDX_INDEX ) ;
      PD_CHECK( String == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_INDEX ) ;
      setIndexName( beItem.valuestr() ) ;

      beItem = boStat.getField( DMS_STAT_IDX_KEY_PATTERN ) ;
      PD_CHECK( Object == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_KEY_PATTERN ) ;
      rc = _initKeyPattern ( beItem.embeddedObject() ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to init key pattern, rc: %d", rc ) ;

      beItem = boStat.getField( DMS_STAT_IDX_IS_UNIQUE ) ;
      PD_CHECK( Bool == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_IS_UNIQUE ) ;
      setUnique( beItem.booleanSafe() ) ;

      beItem = boStat.getField( DMS_STAT_IDX_INDEX_PAGES ) ;
      PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_INDEX_PAGES ) ;
      setIndexPages( (UINT32)beItem.numberInt() ) ;

      beItem = boStat.getField( DMS_STAT_IDX_LEVELS ) ;
      PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_LEVELS ) ;
      setIndexLevels( (UINT32)beItem.numberInt() ) ;

      // Optional fields

      beItem = boStat.getField( DMS_STAT_IDX_NULL_FRAC ) ;
      if ( beItem.isNumber() )
      {
         setNullFrac( (UINT16)beItem.numberInt() ) ;
      }

      beItem = boStat.getField( DMS_STAT_IDX_UNDEF_FRAC ) ;
      if ( beItem.isNumber() )
      {
         setUndefFrac( (UINT16)beItem.numberInt() ) ;
      }

      beItem = boStat.getField( DMS_STAT_IDX_DISTINCT_VALUES ) ;
      if ( beItem.isNumber() )
      {
         setDistinctValues( (UINT64)beItem.numberLong() ) ;
      }

      beItem = boStat.getField( DMS_STAT_IDX_MCV ) ;
      if ( Object == beItem.type() )
      {
         _initMCV( beItem.embeddedObject() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT__INITITEM, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__POSTINIT, "_dmsIndexStat::_postInit" )
   INT32 _dmsIndexStat::_postInit ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__POSTINIT ) ;

      // Initialize the distinct value number if it's not found
      if ( 0 == _distinctValues )
      {
         if ( _isUnique )
         {
            _distinctValues = _totalRecords ;
         }
         else
         {
            _distinctValues = _mcvSet.getSize() ;
         }
      }

      rc = _mcvSet.checkValues( _numKeys, _keyPattern ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to set numKeys of MCV set, rc: %d",
                   rc ) ;

      _mcvSet.setTotalFrac() ;

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT__POSTINIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__TOBSON, "_dmsIndexStat::_toBSON" )
   void _dmsIndexStat::_toBSON ( BSONObjBuilder &builder ) const
   {
      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__TOBSON ) ;

      builder.append( DMS_STAT_IDX_INDEX, getIndexName() ) ;
      builder.append( DMS_STAT_IDX_INDEX_PAGES, (INT32)getIndexPages() ) ;
      builder.append( DMS_STAT_IDX_LEVELS, (INT32)getIndexLevels() ) ;
      builder.append( DMS_STAT_IDX_KEY_PATTERN, getKeyPattern() ) ;
      builder.appendBool( DMS_STAT_IDX_IS_UNIQUE, isUnique() ) ;

      if ( _mcvSet.getSize() > 0 )
      {
         BSONObjBuilder mcvBuilder( builder.subobjStart( DMS_STAT_IDX_MCV ) ) ;

         BSONArrayBuilder mcvValueBuilder(
               mcvBuilder.subarrayStart( DMS_STAT_IDX_MCV_VALUES ) ) ;
         for ( UINT32 i = 0 ; i < _mcvSet.getSize() ; i++ )
         {
            mcvValueBuilder.append( _mcvSet.getValue( i ) ) ;
         }
         mcvValueBuilder.done() ;

         BSONArrayBuilder mcvFracBuilder(
                        mcvBuilder.subarrayStart( DMS_STAT_IDX_MCV_FRAC ) ) ;
         for ( UINT32 i = 0 ; i < _mcvSet.getSize() ; i++ )
         {
            mcvFracBuilder.append( (INT32)_mcvSet.getFracInt( i ) ) ;
         }
         mcvFracBuilder.done() ;

         mcvBuilder.done() ;
      }

      PD_TRACE_EXIT( SDB_DMSIDXSTAT__TOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__INITKEYPTN, "_dmsIndexStat::_initKeyPattern" )
   INT32 _dmsIndexStat::_initKeyPattern ( const BSONObj &boKeyPattern )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__INITKEYPTN ) ;

      _keyPattern = boKeyPattern.getOwned() ;
      _numKeys = _keyPattern.nFields() ;

      PD_CHECK( _numKeys > 0, SDB_INVALIDARG, error, PDWARNING,
                "Empty key pattern" ) ;

      _pFirstField = _keyPattern.firstElementFieldName() ;

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT__INITKEYPTN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXSTAT__INITMCV, "_dmsIndexStat::_initMCV" )
   INT32 _dmsIndexStat::_initMCV ( const BSONObj &boMCV )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSIDXSTAT__INITMCV ) ;

      BSONElement beItem ;

      beItem = boMCV.getField( DMS_STAT_IDX_MCV_VALUES ) ;
      PD_CHECK( Array == beItem.type(),
                SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_MCV_VALUES ) ;
      {
         BSONObj boValues = beItem.embeddedObject() ;
         BSONObjIterator iterValue( boValues ) ;
         UINT32 idx = 0 ;

         if ( _mcvSet.getSize() == 0 && boValues.nFields() > 0 )
         {
            UINT32 size = boValues.nFields() ;
            rc = _mcvSet.init( size, size ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to initialize MCV, rc: %d",
                         rc ) ;
         }
         PD_CHECK( _mcvSet.getSize() == (UINT32)boValues.nFields(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] 's lenght is not matched",
                   beItem.toString().c_str() ) ;

         while ( iterValue.more() )
         {
            BSONElement tempVal = iterValue.next() ;
            PD_CHECK( Object == tempVal.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] 's type is not matched",
                      tempVal.toString().c_str() ) ;
            _mcvSet.setValue( idx, tempVal.embeddedObject() ) ;
            ++ idx ;
         }
      }

      beItem = boMCV.getField( DMS_STAT_IDX_MCV_FRAC ) ;
      PD_CHECK( Array == beItem.type(),
                SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_IDX_MCV_FRAC ) ;
      {
         BSONObj boFrac = beItem.embeddedObject() ;
         BSONObjIterator iterFrac( boFrac ) ;
         UINT32 idx = 0 ;

         PD_CHECK( _mcvSet.getSize() == (UINT32)boFrac.nFields(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] 's length is not matched",
                   beItem.toString().c_str() ) ;

         while ( iterFrac.more() )
         {
            UINT32 frac = 0 ;
            BSONElement tempFrac = iterFrac.next() ;
            PD_CHECK( tempFrac.isNumber(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] 's type is not matched",
                      tempFrac.toString().c_str() ) ;
            frac = (UINT16)tempFrac.numberInt() ;
            _mcvSet.setFrac( idx, frac ) ;
            ++ idx ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXSTAT__INITMCV, rc ) ;
      return rc ;
   error :
      _mcvSet.clear() ;
      goto done ;
   }

   /*
      _dmsCollectionStat implement
    */
   _dmsCollectionStat::_dmsCollectionStat ()
   : _dmsStatUnit(),
     _totalDataPages( DMS_STAT_DEF_TOTAL_PAGES ),
     _totalDataSize( DMS_STAT_DEF_DATA_SIZE * DMS_STAT_DEF_TOTAL_RECORDS ),
     _avgNumFields( DMS_STAT_DEF_AVG_NUM_FIELDS )
   {
      ossMemset( _pCSName, 0, sizeof( _pCSName ) ) ;
      ossMemset( _pCLName, 0, sizeof( _pCLName ) ) ;
   }

   _dmsCollectionStat::_dmsCollectionStat ( const CHAR *pCSName,
                                            const CHAR *pCLName,
                                            UINT32 suLID,
                                            UINT16 mbID,
                                            UINT32 clLID,
                                            UINT64 createTime )
   : _dmsStatUnit( suLID, mbID, clLID, createTime ),
     _totalDataPages( DMS_STAT_DEF_TOTAL_PAGES ),
     _totalDataSize( DMS_STAT_DEF_DATA_SIZE * DMS_STAT_DEF_TOTAL_RECORDS ),
     _avgNumFields( DMS_STAT_DEF_AVG_NUM_FIELDS )
   {
      ossMemset( _pCSName, 0, sizeof( _pCSName ) ) ;
      ossMemset( _pCLName, 0, sizeof( _pCLName ) ) ;

      setCSName( pCSName ) ;
      setCLName( pCLName ) ;
   }

   _dmsCollectionStat::~_dmsCollectionStat ()
   {
      clearSubUnits() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_ADDSUBUNIT, "_dmsCollectionStat::addSubUnit" )
   BOOLEAN _dmsCollectionStat::addSubUnit ( utilSUCacheUnit *pSubUnit,
                                            BOOLEAN ignoreCrtTime )
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT_ADDSUBUNIT ) ;

      BOOLEAN added = FALSE ;

      if ( pSubUnit &&
           pSubUnit->getUnitType() == UTIL_SU_CACHE_UNIT_IXSTAT &&
           pSubUnit->getUnitID() == getUnitID() )
      {
         dmsIndexStat *pIndexStat = (dmsIndexStat *)pSubUnit ;
         dmsExtentID indexLID = pIndexStat->getIndexLogicalID() ;
         INDEX_STAT_MAP::value_type idxStatValue( indexLID, pIndexStat ) ;

         INDEX_STAT_ITERATOR iter = _indexStats.find( indexLID ) ;
         if ( iter != _indexStats.end() )
         {
            dmsIndexStat *pTempStat = iter->second ;

            if ( ignoreCrtTime ||
                 pTempStat->getCreateTime() < pIndexStat->getCreateTime() )
            {
               _indexStats.erase( iter ) ;
               // Update field statistics before delete it
               _findNewFieldStat( pTempStat ) ;
               SAFE_OSS_DELETE( pTempStat ) ;

               _indexStats.insert( idxStatValue ) ;
               added = TRUE ;
            }
         }
         else
         {
            _indexStats.insert( idxStatValue ) ;
            added = TRUE ;
         }

         if ( added )
         {
            // The name pointer should be fixed
            pIndexStat->setCSName( _pCSName ) ;
            pIndexStat->setCLName( _pCLName ) ;
            _addFieldStat( pIndexStat, ignoreCrtTime ) ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_ADDSUBUNIT ) ;

      return added ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_CLEARSUBUNITS, "_dmsCollectionStat::clearSubUnits" )
   void _dmsCollectionStat::clearSubUnits ()
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT_CLEARSUBUNITS ) ;

      _fieldStats.clear() ;

      INDEX_STAT_ITERATOR iterIndex = _indexStats.begin() ;
      while ( iterIndex != _indexStats.end() )
      {
         dmsIndexStat *pIndexStat = iterIndex->second ;
         iterIndex = _indexStats.erase( iterIndex ) ;

         SAFE_OSS_DELETE( pIndexStat ) ;
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_CLEARSUBUNITS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_RMIDXSTAT, "_dmsCollectionStat::removeIndexStat" )
   BOOLEAN _dmsCollectionStat::removeIndexStat ( dmsExtentID indexLID,
                                                 BOOLEAN findNewFieldStat )
   {
      BOOLEAN deleted = FALSE ;

      PD_TRACE_ENTRY( SDB_DMSCLSTAT_RMIDXSTAT ) ;

      if ( DMS_INVALID_EXTENT != indexLID )
      {
         INDEX_STAT_ITERATOR iter = _indexStats.find( indexLID ) ;
         if ( iter != _indexStats.end() )
         {
            dmsIndexStat *pDeletingStat = iter->second ;
            if ( pDeletingStat )
            {
               if ( findNewFieldStat )
               {
                  _findNewFieldStat( pDeletingStat ) ;
               }
               else
               {
                  _removeFieldStat( pDeletingStat ) ;
               }
            }
            _indexStats.erase( iter ) ;
            SAFE_OSS_DELETE( pDeletingStat ) ;
            deleted = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_RMIDXSTAT ) ;

      return deleted ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_RMFLDSTAT, "_dmsCollectionStat::removeFieldStat" )
   BOOLEAN _dmsCollectionStat::removeFieldStat ( const CHAR *pFieldName,
                                                 BOOLEAN findNewFieldStat )
   {
      BOOLEAN deleted = FALSE ;

      PD_TRACE_ENTRY( SDB_DMSCLSTAT_RMFLDSTAT ) ;

      if ( pFieldName )
      {
         FIELD_STAT_ITERATOR iter = _fieldStats.find( pFieldName ) ;
         if ( iter != _fieldStats.end() )
         {
            dmsIndexStat *pDeletingStat = iter->second ;
            if ( pDeletingStat )
            {
               if ( findNewFieldStat )
               {
                  _findNewFieldStat( pDeletingStat ) ;
               }
               else
               {
                  _removeFieldStat( pDeletingStat ) ;
               }
            }
            else
            {
               // Erase item only, no need to delete statistics
               _fieldStats.erase( iter ) ;
            }
            deleted = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_RMFLDSTAT ) ;

      return deleted ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_GETIDXSTAT, "_dmsCollectionStat::getIndexStat" )
   const dmsIndexStat * _dmsCollectionStat::getIndexStat ( dmsExtentID indexLID ) const
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT_GETIDXSTAT ) ;

      const dmsIndexStat *pIndexStat = NULL ;
      INDEX_STAT_CONST_ITERATOR iter = _indexStats.find( indexLID ) ;

      if ( iter != _indexStats.end() )
      {
         pIndexStat = iter->second ;
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_GETIDXSTAT ) ;

      return pIndexStat ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT_GETFLDSTAT, "_dmsCollectionStat::getFieldStat" )
   const dmsIndexStat * _dmsCollectionStat::getFieldStat ( const CHAR *pFieldName ) const
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT_GETFLDSTAT ) ;

      const dmsIndexStat *pFieldStat = NULL ;
      FIELD_STAT_CONST_ITERATOR iter = _fieldStats.find( pFieldName ) ;

      if ( iter != _fieldStats.end() )
      {
         pFieldStat = iter->second ;
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT_GETFLDSTAT ) ;

      return pFieldStat ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT__INITITEM, "_dmsCollectionStat::_initItem" )
   INT32 _dmsCollectionStat::_initItem ( const BSONObj &boStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSCLSTAT__INITITEM ) ;

      BSONElement beItem ;

      // Required fields

      beItem = boStat.getField( DMS_STAT_CL_TOTAL_DATA_PAGES ) ;
      PD_CHECK( beItem.isNumber(),
                SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_CL_TOTAL_DATA_PAGES ) ;
      setTotalDataPages( (UINT32)beItem.numberInt() ) ;

      beItem = boStat.getField( DMS_STAT_CL_TOTAL_DATA_SIZE ) ;
      PD_CHECK( beItem.isNumber(),
                SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", DMS_STAT_CL_TOTAL_DATA_SIZE ) ;
      setTotalDataSize( (UINT64)beItem.numberLong() ) ;

      // Optional fields

      beItem = boStat.getField( DMS_STAT_CL_AVG_NUM_FIELDS ) ;
      if ( beItem.isNumber() )
      {
         setAvgNumFields( (UINT32)beItem.numberInt() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSCLSTAT__INITITEM, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT__TOBSON, "_dmsCollectionStat::_toBSON" )
   void _dmsCollectionStat::_toBSON ( BSONObjBuilder &builder ) const
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT__TOBSON ) ;

      builder.append( DMS_STAT_CL_TOTAL_DATA_PAGES, (INT32)getTotalDataPages() ) ;
      builder.append( DMS_STAT_CL_TOTAL_DATA_SIZE, (INT64)getTotalDataSize() ) ;
      builder.append( DMS_STAT_CL_AVG_NUM_FIELDS, (INT32)getAvgNumFields() ) ;

      PD_TRACE_EXIT( SDB_DMSCLSTAT__TOBSON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT__ADDFLDSTAT, "_dmsCollectionStat::_addFieldStat" )
   void _dmsCollectionStat::_addFieldStat ( dmsIndexStat *pIndexStat,
                                            BOOLEAN ignoreCrtTime )
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT__ADDFLDSTAT ) ;

      if ( pIndexStat &&
           pIndexStat->isValidForEstimate() &&
           pIndexStat->getNumKeys() > 0 )
      {
         const CHAR *pFirstField = pIndexStat->getFirstField() ;
         FIELD_STAT_MAP::value_type fieldStatValue( pFirstField, pIndexStat ) ;
         FIELD_STAT_ITERATOR iter = _fieldStats.find( pFirstField ) ;
         if ( iter != _fieldStats.end() )
         {
            dmsIndexStat *pTempStat = iter->second ;

            if ( pIndexStat->getNumKeys() < pTempStat->getNumKeys() )
            {
               _fieldStats.erase( iter ) ;
               _fieldStats.insert( fieldStatValue ) ;
            }
            else if ( pTempStat->getNumKeys() == pIndexStat->getNumKeys() &&
                      ( ignoreCrtTime ||
                        pTempStat->getCreateTime() < pIndexStat->getCreateTime() ) )
            {
               _fieldStats.erase( iter ) ;
               _fieldStats.insert( fieldStatValue ) ;
            }
         }
         else
         {
            _fieldStats.insert( fieldStatValue ) ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT__ADDFLDSTAT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT__RMFLDSTAT, "_dmsCollectionStat::_removeFieldStat" )
   void _dmsCollectionStat::_removeFieldStat ( dmsIndexStat *pDeletingStat )
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT__RMFLDSTAT ) ;

      const CHAR *pFieldName = pDeletingStat->getFirstField() ;
      FIELD_STAT_ITERATOR iterField = _fieldStats.find( pFieldName ) ;

      if ( iterField->second == pDeletingStat )
      {
         _fieldStats.erase( iterField ) ;
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT__RMFLDSTAT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCLSTAT__FINDFLDSTAT, "_dmsCollectionStat::_findNewFieldStat" )
   void _dmsCollectionStat::_findNewFieldStat ( dmsIndexStat *pDeletingStat )
   {
      PD_TRACE_ENTRY( SDB_DMSCLSTAT__FINDFLDSTAT ) ;

      dmsIndexStat *pNewFieldStat = NULL ;
      const CHAR *pFieldName = pDeletingStat->getFirstField() ;
      FIELD_STAT_ITERATOR iterField = _fieldStats.find( pFieldName ) ;

      if ( iterField != _fieldStats.end() && pDeletingStat == iterField->second )
      {
         for ( INDEX_STAT_ITERATOR iterIndex = _indexStats.begin() ;
               iterIndex != _indexStats.end() ;
               ++ iterIndex )
         {
            dmsIndexStat *pTempFieldStat = iterIndex->second ;
            if ( pTempFieldStat != pDeletingStat &&
                 0 == ossStrcmp( pFieldName, pTempFieldStat->getFirstField() ) )
            {
               if ( !pNewFieldStat )
               {
                  pNewFieldStat = pTempFieldStat ;
               }
               else if ( pTempFieldStat->getNumKeys() < pNewFieldStat->getNumKeys() )
               {
                  pNewFieldStat = pTempFieldStat ;
               }
               else if ( pTempFieldStat->getNumKeys() == pNewFieldStat->getNumKeys() &&
                         pTempFieldStat->getCreateTime() > pNewFieldStat->getCreateTime() )
               {
                  pNewFieldStat = pTempFieldStat ;
               }
            }
         }
         _fieldStats.erase( iterField ) ;
         if ( pNewFieldStat )
         {
            FIELD_STAT_MAP::value_type fieldStatValue(
                  pNewFieldStat->getFirstField(), pNewFieldStat ) ;
            _fieldStats.insert( fieldStatValue ) ;
         }
      }

      PD_TRACE_EXIT( SDB_DMSCLSTAT__FINDFLDSTAT ) ;
   }

}
