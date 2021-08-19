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

   Source File Name = rtnContextData.cpp

   Descriptive Name = RunTime Data Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextData.hpp"
#include "rtnContextSort.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "rtnIXScannerFactory.hpp"
#include "dpsTransLockCallback.hpp"
#include "dmsScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "pmdController.hpp"

using namespace bson ;

namespace engine
{
   /*
      _rtnContextData implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextData, RTN_CONTEXT_DATA, "DATA")

   _rtnContextData::_rtnContextData( INT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID )
   {
      _dmsCB            = NULL ;
      _su               = NULL ;
      _mbContext        = NULL ;
      _scanType         = UNKNOWNSCAN ;
      _numToReturn      = -1 ;
      _numToSkip        = 0 ;

      _extentID         = DMS_INVALID_EXTENT ;
      _lastExtLID       = DMS_INVALID_EXTENT ;
      _segmentScan      = FALSE ;
      _indexBlockScan   = FALSE ;
      _scanner          = NULL ;
      _direction        = 0 ;
      _queryModifier    = NULL ;

      // Save query activity
      _enableMonContext = TRUE ;
      _enableQueryActivity = TRUE ;
      _rsFilter         = NULL ;
      _appendRIDFilter  = FALSE ;
   }

   _rtnContextData::~_rtnContextData ()
   {
      rtnScannerFactory f ;
      f.releaseScanner( _scanner ) ;

      // first release plan
      setQueryActivity( _hitEnd ) ;
      _planRuntime.reset() ;

      // second release mb context
      if ( _mbContext && _su )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
      }
      // last unlock su
      if ( _dmsCB && _su && -1 != contextID() )
      {
         _dmsCB->suUnlock ( _su->CSID() ) ;
      }
      // query modifier
      if ( _queryModifier )
      {
         SDB_OSS_DEL _queryModifier ;
         _queryModifier = NULL ;
         _dmsCB->writeDown( pmdGetThreadEDUCB() ) ;
      }
   }

   const CHAR* _rtnContextData::name() const
   {
      return "DATA" ;
   }

   RTN_CONTEXT_TYPE _rtnContextData::getType() const
   {
      return RTN_CONTEXT_DATA ;
   }

   BOOLEAN _rtnContextData::isWrite() const
   {
      return _queryModifier ? TRUE : FALSE ;
   }

   BOOLEAN _rtnContextData::needRollback() const
   {
      return isWrite() ;
   }

   void _rtnContextData::setResultSetFilter( rtnResultSetFilter *rsFilter,
                                             BOOLEAN appendMode )
   {
      _rsFilter = rsFilter ;
      _appendRIDFilter = appendMode ;
   }

   void _rtnContextData::_toString( stringstream & ss )
   {
      if ( NULL != _su && NULL != _planRuntime.getPlan() )
      {
         ss << ",Collection:" << _planRuntime.getCLFullName() ;
         ss << ",Matcher:" << _planRuntime.getParsedMatcher().toString() ;
      }
      ss << ",ScanType:" << ( ( TBSCAN == _scanType ) ? "TBSCAN" : "IXSCAN" ) ;
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTDATA_OPIXSC, "_rtnContextData::_openIXScan" )
   INT32 _rtnContextData::_openIXScan( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       const rtnReturnOptions &returnOptions,
                                       const BSONObj *blockObj,
                                       INT32 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCONTEXTDATA_OPIXSC );

      rtnScannerFactory f ;
      IXScannerType scanType = ( DPS_INVALID_TRANS_ID != cb->getTransID() ) ?
                               SCANNER_TYPE_MERGE : SCANNER_TYPE_DISK ;
      rtnPredicateList *predList = NULL ;

      // for index scan, we maintain context by runtime instead of by DMS
      ixmIndexCB indexCB ( _planRuntime.getIndexCBExtent(),
                           su->index(),
                           NULL ) ;
      if ( !indexCB.isInitialized() )
      {
         PD_LOG ( PDERROR, "unable to get proper index control block" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( indexCB.getLogicalID() != _planRuntime.getIndexLID() )
      {
         PD_LOG( PDERROR, "Index[extent id: %d] logical id[%d] is not "
                 "expected[%d]", _planRuntime.getIndexCBExtent(),
                 indexCB.getLogicalID(), _planRuntime.getIndexLID() ) ;
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }
      // get the predicate list
      predList = _planRuntime.getPredList() ;
      SDB_ASSERT ( predList, "predList can't be NULL" ) ;

      // create scanner
      if ( _scanner )
      {
         f.releaseScanner( _scanner ) ;
      }

      rc = f.createScanner( scanType, &indexCB, predList, su, cb, _scanner ) ;
      if ( rc )
      {
         goto error ;
      }
      // index block scan
      if ( blockObj )
      {
         SDB_ASSERT( direction == 1 || direction == -1,
                     "direction must be 1 or -1" ) ;

         _direction = direction ;
         rc = _parseIndexBlocks( *blockObj, _indexBlocks, _indexRIDs ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse index blocks failed, rc: %d", rc ) ;
         _indexBlockScan = TRUE ;

         if ( _indexBlocks.size() < 2 )
         {
            _hitEnd = TRUE ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCONTEXTDATA_OPIXSC , rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTDATA_OPTBSC, "_rtnContextData::_openTBScan" )
   INT32 _rtnContextData::_openTBScan( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB * cb,
                                       const rtnReturnOptions &returnOptions,
                                       const BSONObj *blockObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCONTEXTDATA_OPTBSC );

      if ( blockObj )
      {
         rc = _parseSegments( *blockObj, _segments ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse segments[%s] failed, rc: %d",
                      blockObj->toString().c_str(), rc ) ;

         _segmentScan = TRUE ;
         _extentID = _segments.size() > 0 ? *_segments.begin() :
                     DMS_INVALID_EXTENT ;
      }
      else
      {
         _extentID = mbContext->mb()->_firstExtentID ;
      }

      if ( DMS_INVALID_EXTENT == _extentID )
      {
         _hitEnd = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCONTEXTDATA_OPTBSC , rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::open( dmsStorageUnit *su,
                                dmsMBContext *mbContext,
                                pmdEDUCB *cb,
                                const rtnReturnOptions &returnOptions,
                                const BSONObj *blockObj,
                                INT32 direction )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isStictType = FALSE ;
      BSONObj selector = returnOptions.getSelector() ;

      SDB_ASSERT( su && mbContext, "Invalid param" ) ;
      SDB_ASSERT( _planRuntime.hasPlan(), "Invalid plan" ) ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_QUERY ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         isStictType = TRUE ;
      }

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      if ( TBSCAN == _planRuntime.getScanType() )
      {
         rc = _openTBScan( su, mbContext, cb, returnOptions, blockObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open tbscan, rc: %d", rc ) ;

         mbContext->mbStat()->_crudCB.increaseTbScan( 1 ) ;
      }
      else if ( IXSCAN == _planRuntime.getScanType() )
      {
         rc = _openIXScan( su, mbContext, cb, returnOptions,
                           blockObj, direction ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open ixscan, rc: %d", rc ) ;

         mbContext->mbStat()->_crudCB.increaseIxScan( 1 ) ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unknow scan type: %d", _planRuntime.getScanType() ) ;
         goto error ;
      }

      // once context is opened, let's construct matcher and selector
      if ( !selector.isEmpty() )
      {
         try
         {
            rc = _selector.loadPattern ( selector, isStictType ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Invalid pattern is detected for select: %s: %s",
                     selector.toString().c_str(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for select: "
                      "%s, rc: %d", selector.toString().c_str(), rc ) ;
      }

      _dmsCB = pmdGetKRCB()->getDMSCB() ;
      _su = su ;
      _mbContext = mbContext ;
      _scanType = _planRuntime.getScanType() ;

      _returnOptions.setSelector( returnOptions.getSelector().getOwned() ) ;
      _returnOptions.setSkip( returnOptions.getSkip() ) ;
      _returnOptions.setLimit( returnOptions.getLimit() ) ;
      _returnOptions.resetFlag( returnOptions.getFlag() ) ;
      _numToReturn = returnOptions.getLimit() ;
      _numToSkip = returnOptions.getSkip() ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      mbContext->mbUnlock() ;
      return rc ;
   error:
      _isOpened = FALSE ;
      _hitEnd = TRUE ;
      goto done ;
   }

   INT32 _rtnContextData::openTraversal( dmsStorageUnit *su,
                                         dmsMBContext *mbContext,
                                         rtnIXScanner *scanner,
                                         pmdEDUCB *cb,
                                         const BSONObj &selector,
                                         INT64 numToReturn,
                                         INT64 numToSkip )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN strictDataMode = FALSE ;

      SDB_ASSERT( su && mbContext && scanner, "Invalid param" ) ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }
      if ( IXSCAN != _planRuntime.getScanType() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Open traversal must IXSCAN" ) ;
         goto error ;
      }

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_QUERY ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         strictDataMode = TRUE ;
      }

      if ( _scanner )
      {
         SDB_OSS_DEL _scanner ;
      }
      _scanner = scanner ;

      // once context is opened, let's construct matcher and selector
      if ( !selector.isEmpty() )
      {
         try
         {
            rc = _selector.loadPattern ( selector, strictDataMode ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Invalid pattern is detected for select: %s: %s",
                     selector.toString().c_str(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for select: "
                      "%s, rc: %d", selector.toString().c_str(), rc ) ;
      }

      _dmsCB = pmdGetKRCB()->getDMSCB() ;
      _su = su ;
      _mbContext = mbContext ;
      _scanType = _planRuntime.getScanType() ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip > 0 ? numToSkip : 0 ;

      _isOpened = TRUE ;
      _hitEnd = _scanner->eof() ? TRUE : FALSE ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextData::setQueryModifier ( rtnQueryModifier* modifier )
   {
      SDB_ASSERT( NULL == _queryModifier, "_queryModifier already exists" ) ;

      _queryModifier = modifier ;
   }

   void _rtnContextData::setQueryActivity ( BOOLEAN hitEnd )
   {
      if ( _planRuntime.canSetQueryActivity() &&
           enabledMonContext() &&
           enabledQueryActivity() )
      {
         _planRuntime.setQueryActivity( MON_SELECT, _monCtxCB, _returnOptions,
                                        hitEnd ) ;
      }
   }

   INT32 _rtnContextData::_queryModify( pmdEDUCB* eduCB,
                                        const dmsRecordID& recordID,
                                        ossValuePtr recordDataPtr,
                                        BSONObj& obj,
                                        IDmsOprHandler* pHandler,
                                        const dmsTransRecordInfo *pInfo )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _queryModifier, "_queryModifier can't be null" ) ;
      // check id index
      if ( OSS_BIT_TEST( _mbContext->mb()->_attributes,
                         DMS_MB_ATTR_NOIDINDEX ) )
      {
         PD_LOG( PDERROR, "can not update data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      if ( _queryModifier->isUpdate() )
      {
         BSONObj* newObjPtr = NULL ;

         if ( _queryModifier->returnNew() )
         {
            newObjPtr = &obj ;
         }
         else
         {
            obj = obj.getOwned() ;
         }

         SDB_ASSERT( NULL != _queryModifier->getDollarList(),
                     "dollarList can't be null" ) ;

         rc = _su->data()->updateRecord( _mbContext, recordID,
                                         recordDataPtr, eduCB, getDPSCB(),
                                         _queryModifier->getModifier(),
                                         newObjPtr, pHandler ) ;
         PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;
         _queryModifier->getDollarList()->clear() ;
      }
      else if ( _queryModifier->isRemove() )
      {
         rc = _su->data()->deleteRecord( _mbContext, recordID,
                                         recordDataPtr, eduCB, getDPSCB(),
                                         pHandler, pInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_prepareData( pmdEDUCB *cb )
   {
      vector<INT64>* dollarList = NULL ;
      DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH ;
      INT32 rc = SDB_OK ;

      if ( NULL != cb && NULL != _mbContext )
      {
         cb->registerMonCRUDCB( &( _mbContext->mbStat()->_crudCB ) ) ;
      }

      if ( _queryModifier )
      {
         if ( _queryModifier->isUpdate() )
         {
            accessType = DMS_ACCESS_TYPE_UPDATE ;
            dollarList = _queryModifier->getDollarList() ;
         }
         else if ( _queryModifier->isRemove() )
         {
            accessType = DMS_ACCESS_TYPE_DELETE ;
         }
         else
         {
            SDB_ASSERT( FALSE, "_queryModifier is invalid" ) ;
            PD_LOG( PDERROR, "_queryModifier is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( TBSCAN == _scanType )
      {
         rc = _prepareByTBScan( cb, accessType, dollarList ) ;
      }
      else if ( IXSCAN == _scanType )
      {
         rc = _prepareByIXScan( cb, accessType, dollarList ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }

   done:
      if ( NULL != cb && NULL != _mbContext )
      {
         cb->unregisterMonCRUDCB() ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__SELANDAPPD, "_rtnContextData::_selectAndAppend" )
   INT32 _rtnContextData::_selectAndAppend( mthSelector *selector,
                                            BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObj selObj ;
      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__SELANDAPPD );

      if ( selector )
      {
         rc = selector->select( obj, selObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build select record,"
                      "src obj: %s, rc: %d", obj.toString().c_str(),
                      rc ) ;
      }
      else
      {
         selObj = obj ;
      }

      rc = append( selObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                   selObj.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__SELANDAPPD, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_innerAppend( mthSelector *selector,
                                        _mthRecordGenerator &generator )
   {
      INT32 rc = SDB_OK ;

      while ( generator.hasNext() )
      {
         BSONObj record ;
         rc = generator.getNext( record ) ;
         PD_RC_CHECK( rc, PDERROR, "get next record failed:rc=%d", rc ) ;

         rc = _selectAndAppend( selector, record ) ;
         PD_RC_CHECK( rc, PDERROR, "selectAndAppend failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN, "_rtnContextData::_prepareByTBScan" )
   INT32 _rtnContextData::_prepareByTBScan( pmdEDUCB * cb,
                                            DMS_ACCESS_TYPE accessType,
                                            vector<INT64>* dollarList )
   {
      INT32 rc = SDB_OK ;
      INT32 startNumRecords = numRecords() ;
      dmsRecordID recordID ;
      ossValuePtr recordDataPtr = 0 ;
      _mthRecordGenerator generator ;
      BOOLEAN hasLocked = _mbContext->isMBLock() ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      mthMatchRuntime *matchRuntime = _planRuntime.getMatchRuntime( TRUE ) ;
      mthSelector *selector   = NULL ;
      dmsExtScannerFactory* extFactory = dmsGetScannerFactory() ;
      dmsExtScannerBase* extScanner = NULL ;

      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN );

      if ( _selector.isInitialized() )
      {
         selector = &_selector ;
      }

      if ( DMS_INVALID_EXTENT == _extentID )
      {
         SDB_ASSERT( FALSE, "extentID can't be INVALID" ) ;
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( NULL != _queryModifier )
      {
         generator.setQueryModify( TRUE ) ;
      }

      extScanner = extFactory->create( _su->data(), _mbContext, matchRuntime,
                                       _extentID, accessType,
                                       _numToReturn, _numToSkip,
                                       _returnOptions.getFlag() ) ;
      if ( !extScanner )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to create extent scanner" ) ;
         goto error ;
      }

      while ( numRecords() == startNumRecords )
      {
         _mthMatchTreeContext mthContext ;
         if ( NULL != dollarList )
         {
            mthContext.enableDollarList() ;
         }

         // prefetch
         if ( eduID() != cb->getID() && !isOpened() )
         {
            rc = SDB_DMS_CONTEXT_IS_CLOSE ;
            goto error ;
         }

         while ( SDB_OK == ( rc = extScanner->advance( recordID, generator,
                                                       cb, &mthContext ) ) )
         {
            try
            {
               generator.getDataPtr( recordDataPtr ) ;
               BSONObj obj( (const CHAR*)recordDataPtr ) ;

               if ( _rsFilter )
               {
                  if ( _appendRIDFilter )
                  {
                     BOOLEAN pushed = FALSE ;
                     rc = _rsFilter->push( recordID, pushed ) ;
                     PD_RC_CHECK( rc, PDERROR, "Push record id to result set "
                                  "filter failed: %d", rc ) ;
                     if ( !pushed )
                     {
                        continue ;
                     }
                  }
                  else
                  {
                     if ( _rsFilter->isFiltered( recordID ) )
                     {
                        continue ;
                     }
                  }
               }

               if ( _queryModifier )
               {
                  //dollarList is pointed to _queryModifier->getDollarList()
                  mthContext.getDollarList( dollarList ) ;
                  rc = _queryModify( cb, recordID, recordDataPtr,
                                     obj, extScanner->callbackHandler(),
                                     extScanner->recordInfo() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to query modify" ) ;
                  generator.resetValue( obj, &mthContext ) ;
               }

               rc = _innerAppend( selector, generator ) ;
               PD_RC_CHECK( rc, PDERROR, "innerAppend failed:rc=%d", rc ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // increase counter
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;
            // decrease numToReturn
            if ( _numToReturn > 0 )
            {
               --_numToReturn ;
            }

            //do not clear dollarlist flag
            mthContext.clearRecordInfo() ;
         } // end while

         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Extent scanner failed, rc: %d", rc ) ;
            goto error ;
         }

         _numToReturn = extScanner->getMaxRecords() ;
         _numToSkip   = extScanner->getSkipNum() ;

         if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( _segmentScan )
         {
            if ( DMS_INVALID_EXTENT == extScanner->nextExtentID() ||
                 _su->data()->extent2Segment( *_segments.begin() ) !=
                 _su->data()->extent2Segment( extScanner->nextExtentID() ) )
            {
               _segments.erase( _segments.begin() ) ;
               if ( _segments.size() > 0 )
               {
                  _extentID = *_segments.begin() ;
               }
               else
               {
                  _extentID = DMS_INVALID_EXTENT ;
               }
            }
            else
            {
               _extentID = extScanner->nextExtentID() ;
            }
         }
         else
         {
            _extentID = extScanner->nextExtentID() ;
         }
         _lastExtLID = extScanner->curExtent()->_logicID ;

         // If the next extent is valid, let's step to it. Otherwise, the end
         // is hit.
         if ( DMS_INVALID_EXTENT == _extentID ||
              SDB_DMS_EOC == extScanner->stepToNextExtent() )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( !hasLocked )
         {
            _mbContext->pause() ;
         }
      } // end while

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      if ( !hasLocked )
      {
         _mbContext->pause() ;
      }
      if ( extScanner )
      {
         SDB_OSS_DEL extScanner ;
      }
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN, "_rtnContextData::_prepareByIXScan" )
   INT32 _rtnContextData::_prepareByIXScan( pmdEDUCB *cb,
                                            DMS_ACCESS_TYPE accessType,
                                            vector<INT64>* dollarList )
   {
      INT32 rc                   = SDB_OK ;
      rtnIXScanner *scanner      = _scanner ;
      mthMatchRuntime *matchRuntime = _planRuntime.getMatchRuntime( TRUE ) ;
      mthSelector *selector      = NULL ;
      monAppCB * pMonAppCB       = cb ? cb->getMonAppCB() : NULL ;
      BOOLEAN hasLocked          = _mbContext->isMBLock() ;
      INT32 startNumRecords      = numRecords();

      dmsRecordID rid ;
      BSONObj dataRecord ;

      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN );
      if ( _selector.isInitialized() )
      {
         selector = &_selector ;
      }

      _mthRecordGenerator generator ;
      dmsRecordID recordID ;
      ossValuePtr recordDataPtr = 0 ;

      if ( NULL != _queryModifier )
      {
         generator.setQueryModify( TRUE ) ;
      }

      // loop until we read something in the buffer
      while ( numRecords() == startNumRecords )
      {
         _mthMatchTreeContext mthContext ;
         if ( NULL != dollarList )
         {
            mthContext.enableDollarList() ;
         }

         // prefetch
         if ( eduID() != cb->getID() && !isOpened() )
         {
            rc = SDB_DMS_CONTEXT_IS_CLOSE ;
            goto error ;
         }

         dmsIXSecScanner secScanner( _su->data(), _mbContext, matchRuntime,
                                     scanner, accessType, _numToReturn,
                                     _numToSkip,
                                     _returnOptions.getFlag() ) ;
         if ( _indexBlockScan )
         {
            secScanner.enableIndexBlockScan( _indexBlocks[0],
                                             _indexBlocks[1],
                                             _indexRIDs[0],
                                             _indexRIDs[1],
                                             _direction ) ;
         }
         if ( isCountMode() && ( !cb->isTransaction() || cb->isTransRU() ) )
         {
            secScanner.enableCountMode() ;
         }

         while ( SDB_OK == ( rc = secScanner.advance( recordID, generator,
                                                      cb, &mthContext ) ) )
         {
            if ( _rsFilter )
            {
               if ( _appendRIDFilter )
               {
                  BOOLEAN pushed = FALSE ;
                  rc = _rsFilter->push( recordID, pushed ) ;
                  PD_RC_CHECK( rc, PDERROR, "Push record id to result set "
                               "filter failed: %d", rc ) ;
                  if ( !pushed )
                  {
                     continue ;
                  }
               }
               else
               {
                  if ( _rsFilter->isFiltered( recordID ) )
                  {
                     continue ;
                  }
               }
            }

            if ( !isCountMode() )
            {
               try
               {
                  generator.getDataPtr( recordDataPtr ) ;
                  BSONObj obj( (const CHAR*)recordDataPtr ) ;

                  if ( _queryModifier )
                  {
                     //dollarList is pointed to _queryModifier->getDollarList()
                     mthContext.getDollarList( dollarList ) ;
                     rc = _queryModify( cb, recordID, recordDataPtr,
                                        obj,  secScanner.callbackHandler(),
                                        secScanner.recordInfo() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Failed to query modify" ) ;
                     generator.resetValue( obj, &mthContext ) ;
                  }

                  rc = _innerAppend( selector, generator ) ;
                  PD_RC_CHECK( rc, PDERROR, "innerAppend failed:rc=%d", rc ) ;

                  // make sure we still have room to read another
                  // record_max_sz (i.e. 16MB). if we have less than 16MB
                  // to 256MB, we can't safely assume the next record we
                  // read will not overflow the buffer, so let's just break
                  // before reading the next record
                  if ( buffEndOffset() + DMS_RECORD_MAX_SZ >
                       RTN_RESULTBUFFER_SIZE_MAX )
                  {
                     secScanner.stop () ;
                     // let's break if there's no room for another max record
                     break ;
                  }
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               // increase counter
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;
            }
            else
            {
               static BSONObj dummyObj ;
               rc = append( dummyObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Append empty obj failed, rc: %d",
                            rc ) ;
            }

            //do not clear dollarlist flag
            mthContext.clearRecordInfo() ;
         }

         if ( rc && SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Extent scanner failed, rc: %d", rc ) ;
            goto error ;
         }

         _numToReturn = secScanner.getMaxRecords() ;
         _numToSkip   = secScanner.getSkipNum() ;

         if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( secScanner.eof() )
         {
            if ( _indexBlockScan )
            {
               _indexBlocks.erase( _indexBlocks.begin() ) ;
               _indexBlocks.erase( _indexBlocks.begin() ) ;
               _indexRIDs.erase( _indexRIDs.begin() ) ;
               _indexRIDs.erase( _indexRIDs.begin() ) ;
               if ( _indexBlocks.size() < 2 )
               {
                  _hitEnd = TRUE ;
                  break ;
               }
            }
            else
            {
               _hitEnd = TRUE ;
               break ;
            }
         }

         if ( !hasLocked )
         {
            _mbContext->pause() ;
         }
      } // end while

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done :
      if ( !hasLocked )
      {
         _mbContext->pause() ;
      }
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnContextData::_parseSegments( const BSONObj &obj,
                                          vector< dmsExtentID > &segments )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      segments.clear() ;

      try
      {
         BSONObjIterator it ( obj ) ;
         while ( it.more() )
         {
            ele = it.next() ;
            if ( NumberInt != ele.type() )
            {
               PD_LOG( PDWARNING, "Datablocks[%s] value type[%d] is not NumberInt",
                       obj.toString().c_str(), ele.type() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            segments.push_back( ele.numberInt() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse segments: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_parseRID( const BSONElement & ele,
                                     dmsRecordID & rid )
   {
      INT32 rc = SDB_OK ;
      rid.reset() ;

      if ( ele.eoo() )
      {
         goto done ;
      }
      else if ( Array != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDWARNING, "Field[%s] type is not Array",
                 ele.toString().c_str() ) ;
         goto error ;
      }
      else
      {
         UINT32 count = 0 ;
         BSONElement ridEle ;
         BSONObjIterator it( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            ridEle = it.next() ;
            if ( NumberInt != ridEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "RID type is not NumberInt in field[%s]",
                       ele.toString().c_str() ) ;
               goto error ;
            }
            if ( 0 == count )
            {
               rid._extent = ridEle.numberInt() ;
            }
            else if ( 1 == count )
            {
               rid._offset = ridEle.numberInt() ;
            }

            ++count ;
         }

         if ( 2 != count )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "RID array size[%d] is not 2", count ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_parseIndexBlocks( const BSONObj &obj,
                                             vector< BSONObj > &indexBlocks,
                                             vector< dmsRecordID > &indexRIDs )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj indexObj ;
      BSONObj startKey, endKey ;
      dmsRecordID startRID, endRID ;

      indexBlocks.clear() ;
      indexRIDs.clear() ;

      try
      {
         BSONObjIterator it ( obj ) ;
         while ( it.more() )
         {
            ele = it.next() ;
            if ( Object != ele.type() )
            {
               PD_LOG( PDWARNING, "Indexblocks[%s] value type[%d] is not "
                       "Object", obj.toString().c_str(), ele.type() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            indexObj = ele.embeddedObject() ;
            // StartKey
            rc = rtnGetObjElement( indexObj, FIELD_NAME_STARTKEY, startKey ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s] from obj[%s], "
                         "rc: %d", FIELD_NAME_STARTKEY,
                         indexObj.toString().c_str(), rc ) ;
            // EndKey
            rc = rtnGetObjElement( indexObj, FIELD_NAME_ENDKEY, endKey ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s] from obj[%s], "
                         "rc: %d", FIELD_NAME_ENDKEY,
                         indexObj.toString().c_str(), rc ) ;
            // StartRID
            rc = _parseRID( indexObj.getField( FIELD_NAME_STARTRID ),
                            startRID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to parse %s, rc: %d",
                         FIELD_NAME_STARTRID, rc ) ;

            // EndRID
            rc = _parseRID( indexObj.getField( FIELD_NAME_ENDRID ), endRID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to parse %s, rc: %d",
                         FIELD_NAME_ENDRID, rc ) ;

            indexBlocks.push_back( rtnNullKeyNameObj( startKey ).getOwned() ) ;
            indexBlocks.push_back( rtnNullKeyNameObj( endKey ).getOwned() ) ;

            indexRIDs.push_back( startRID ) ;
            indexRIDs.push_back( endRID ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse indexBlocks: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( indexBlocks.size() != indexRIDs.size() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "block array size is not the same with rid array" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _rtnContextParaData implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextParaData, RTN_CONTEXT_PARADATA, "PARADATA")

   _rtnContextParaData::_rtnContextParaData( INT64 contextID, UINT64 eduID )
   :_rtnContextData( contextID, eduID )
   {
      _isParalled = FALSE ;
      _curIndex   = 0 ;
      _step       = 1 ;
   }

   _rtnContextParaData::~_rtnContextParaData()
   {
      setQueryActivity( _hitEnd ) ;
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         (*it)->_close () ;
         ++it ;
      }
      it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         (*it)->waitForPrefetch() ;
         SDB_OSS_DEL (*it) ;
         ++it ;
      }
      _vecContext.clear () ;
   }

   const CHAR* _rtnContextParaData::name() const
   {
      return "PARADATA" ;
   }

   RTN_CONTEXT_TYPE _rtnContextParaData::getType () const
   {
      return RTN_CONTEXT_PARADATA ;
   }

   INT32 _rtnContextParaData::open( dmsStorageUnit *su,
                                    dmsMBContext *mbContext,
                                    pmdEDUCB *cb,
                                    const rtnReturnOptions &returnOptions,
                                    const BSONObj *blockObj,
                                    INT32 direction )
   {
      INT32 rc = SDB_OK ;
      rtnReturnOptions subReturnOptions( returnOptions ) ;

      _step = pmdGetKRCB()->getOptionCB()->maxSubQuery() ;
      if ( 0 == _step )
      {
         _step = 1 ;
      }

      rc = _rtnContextData::open( su, mbContext, cb, returnOptions, blockObj,
                                  direction ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( eof() )
      {
         goto done ;
      }

      if ( TBSCAN == _scanType && FALSE == _segmentScan )
      {
         rc = _su->getSegExtents( NULL, _segments, mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get segment extent, rc: %d",
                      rc ) ;
         if ( _segments.size() <= 1 )
         {
            _segments.clear() ;
            goto done ;
         }
         _segmentScan = TRUE ;
      }
      else if ( IXSCAN == _scanType && FALSE == _indexBlockScan )
      {
         rc = rtnGetIndexSeps( &_planRuntime, su, mbContext, cb, _indexBlocks,
                               _indexRIDs ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index seperations, rc: %d",
                      rc ) ;
         if ( _indexBlocks.size() <= 2 )
         {
            _indexBlocks.clear() ;
            _indexRIDs.clear() ;
            goto done ;
         }
         _indexBlockScan = TRUE ;
         _direction = 1 ;
      }

      if ( ( _segmentScan && _segments.size() <= 1 ) ||
           ( _indexBlockScan && _indexBlocks.size() <= 2 ) )
      {
         goto done ;
      }

      _isParalled = TRUE ;
      mbContext->mbUnlock() ;

      if ( returnOptions.getLimit() > 0 &&
           returnOptions.getSkip() > 0 )
      {
         subReturnOptions.setLimit( returnOptions.getLimit() +
                                    returnOptions.getSkip() ) ;
      }
      subReturnOptions.setSkip( 0 ) ;
      subReturnOptions.clearFlag( FLG_QUERY_PARALLED ) ;

      while ( NULL != ( blockObj = _nextBlockObj() ) )
      {
         rc = _openSubContext( cb, subReturnOptions, blockObj ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      _checkAndPrefetch () ;

   done:
      mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextParaData::_removeSubContext( rtnContextData *pContext )
   {
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         if ( *it == pContext )
         {
            pContext->waitForPrefetch() ;
            SDB_OSS_DEL pContext ;
            _vecContext.erase( it ) ;
            break ;
         }
         ++it ;
      }
   }

   INT32 _rtnContextParaData::_openSubContext( pmdEDUCB *cb,
                                               const rtnReturnOptions &subReturnOptions,
                                               const BSONObj *blockObj )
   {
      INT32 rc = SDB_OK ;

      dmsMBContext *mbContext = NULL ;
      rtnContextData *dataContext = NULL ;

      rc = _su->data()->getMBContext( &mbContext, _planRuntime.getCLMBID(),
                                      DMS_INVALID_CLID, DMS_INVALID_CLID, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;
      PD_CHECK( _planRuntime.getCLLID() == mbContext->clLID(), SDB_DMS_NOTEXIST,
                error, PDERROR, "Failed to get dms mb context, rc: %d",
                SDB_DMS_NOTEXIST ) ;

      // create a new context
      dataContext = SDB_OSS_NEW rtnContextData( -1, eduID() ) ;
      if ( !dataContext )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc sub context out of memory" ) ;
         goto error ;
      }
      _vecContext.push_back( dataContext ) ;

      dataContext->getPlanRuntime()->inheritRuntime( &_planRuntime ) ;

      rc = dataContext->open( _su, mbContext, cb, subReturnOptions, blockObj,
                              _direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Open sub context failed, blockObj: %s, "
                   "rc: %d", blockObj->toString().c_str(), rc ) ;

      mbContext = NULL ;

      dataContext->enablePrefetch ( cb, &_prefWather ) ;
      dataContext->setEnableQueryActivity( FALSE ) ;

      // sample timetamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         dataContext->getMonCB()->recordStartTimestamp() ;
      }
      dataContext->getSelector().setStringOutput(
         getSelector().getStringOutput() ) ;

   done :
      return rc ;
   error :
      if ( mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      goto done ;
   }

   INT32 _rtnContextParaData::_checkAndPrefetch ()
   {
      INT32 rc = SDB_OK ;
      rtnContextData *pContext = NULL ;
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         pContext = *it ;
         if ( pContext->eof() && pContext->isEmpty() )
         {
            pContext->waitForPrefetch() ;
            SDB_OSS_DEL pContext ;
            it = _vecContext.erase( it ) ;
            continue ;
         }
         else if ( !pContext->isEmpty() ||
                   pContext->_getWaitPrefetchNum() > 0 )
         {
            ++it ;
            continue ;
         }
         pContext->_onDataEmpty() ;
         ++it ;
      }

      if ( _vecContext.size() == 0 )
      {
         rc = SDB_DMS_EOC ;
         _hitEnd = TRUE ;
      }

      return rc ;
   }

   const BSONObj* _rtnContextParaData::_nextBlockObj ()
   {
      BSONArrayBuilder builder ;
      UINT32 curIndex = _curIndex ;

      if ( _curIndex >= _step ||
           ( TBSCAN == _scanType && _curIndex >= _segments.size() ) ||
           ( IXSCAN == _scanType && _curIndex + 1 >= _indexBlocks.size() ) )
      {
         return NULL ;
      }
      ++_curIndex ;

      if ( TBSCAN == _scanType )
      {
         while ( curIndex < _segments.size() )
         {
            builder.append( _segments[curIndex] ) ;
            curIndex += _step ;
         }
      }
      else if ( IXSCAN == _scanType )
      {
         while ( curIndex + 1 < _indexBlocks.size() )
         {
            builder.append( BSON( FIELD_NAME_STARTKEY <<
                                  _indexBlocks[curIndex] <<
                                  FIELD_NAME_ENDKEY <<
                                  _indexBlocks[curIndex+1] <<
                                  FIELD_NAME_STARTRID <<
                                  BSON_ARRAY( _indexRIDs[curIndex]._extent <<
                                              _indexRIDs[curIndex]._offset ) <<
                                  FIELD_NAME_ENDRID <<
                                  BSON_ARRAY( _indexRIDs[curIndex+1]._extent <<
                                              _indexRIDs[curIndex+1]._offset )
                                 )
                            ) ;
            curIndex += _step ;
         }
      }
      else
      {
         return NULL ;
      }

      _blockObj = builder.arr() ;
      return &_blockObj ;
   }

   INT32 _rtnContextParaData::_getSubCtxWithData( rtnContextData **ppContext,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT32 index = 0 ;

      do
      {
         index = 0 ;
         _prefWather.reset() ;

         while ( index < _vecContext.size() )
         {
            rc = _vecContext[index]->prefetchResult () ;
            if ( rc && SDB_DMS_EOC != rc )
            {
               goto error ;
            }
            rc = SDB_OK ;

            if ( !_vecContext[index]->isEmpty() &&
                 !_vecContext[index]->_isInPrefetching () )
            {
               *ppContext = _vecContext[index] ;
               goto done ;
            }
            ++index ;
         }
      } while ( _prefWather.waitDone( OSS_ONE_SEC * 5 ) > 0 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextParaData::_getSubContextData( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextData *pContext = NULL ;
      INT64 maxReturnNum = -1 ;
      INT32 startNumRecords = numRecords();

      while ( numRecords() == startNumRecords && 0 != _numToReturn )
      {
         pContext = NULL ;
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _numToSkip <= 0 )
         {
            rc = _getSubCtxWithData( &pContext, cb ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         if ( !pContext && _vecContext.size() > 0 )
         {
            pContext = _vecContext[0] ;
         }

         // get data
         if ( pContext )
         {
            rtnContextBuf buffObj ;
            if ( _numToSkip > 0 )
            {
               maxReturnNum = _numToSkip ;
            }
            else
            {
               maxReturnNum = -1 ;
            }

            // get data
            rc = pContext->getMore( maxReturnNum, buffObj, cb ) ;
            if ( rc )
            {
               _removeSubContext( pContext ) ;
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG( PDERROR, "Failed to get more from sub context, "
                          "rc: %d", rc ) ;
                  goto error ;
               }
               continue ;
            }

            if ( _numToSkip > 0 )
            {
               _numToSkip -= buffObj.recordNum() ;
               continue ;
            }

            if ( _numToReturn > 0 && buffObj.recordNum() > _numToReturn )
            {
               buffObj.truncate( _numToReturn ) ;
            }
            // append data
            rc = appendObjs( buffObj.data(), buffObj.size(),
                             buffObj.recordNum() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add objs, rc: %d", rc ) ;
            if ( _numToReturn > 0 )
            {
               _numToReturn -= buffObj.recordNum() ;
            }
         } // end if ( pContext )

         if ( SDB_OK != _checkAndPrefetch() )
         {
            break ;
         }
      } // while ( isEmpty() && 0 != _numToReturn )

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextParaData::_prepareData( pmdEDUCB * cb )
   {
      if ( !_isParalled )
      {
         return _rtnContextData::_prepareData( cb ) ;
      }
      else
      {
         return _getSubContextData( cb ) ;
      }
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextSort, RTN_CONTEXT_SORT, "SORT")

   _rtnContextSort::_rtnContextSort( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _rtnSubContextHolder(),
    _keyGen ( BSONObj() ),
    _dataSorted ( FALSE ),
    _numToSkip( 0 ),
    _numToReturn( -1 )
   {
      _enableMonContext = TRUE ;
      _enableQueryActivity = TRUE ;
   }

   _rtnContextSort::~_rtnContextSort()
   {
      setQueryActivity( _hitEnd ) ;
      _planRuntime.reset() ;
      _numToSkip = 0 ;
      _numToReturn = 0 ;
   }

   const CHAR* _rtnContextSort::name() const
   {
      return "SORT" ;
   }

   RTN_CONTEXT_TYPE _rtnContextSort::getType() const
   {
      return RTN_CONTEXT_SORT ;
   }

   INT32 _rtnContextSort::open( const BSONObj &orderby,
                                rtnContext *context,
                                pmdEDUCB *cb,
                                SINT64 numToSkip,
                                SINT64 numToReturn )
   {
      SDB_ASSERT( !orderby.isEmpty(), "impossible" ) ;
      SDB_ASSERT( NULL != cb, "possible" ) ;
      SDB_ASSERT( NULL != context, "impossible" ) ;
      INT32 rc = SDB_OK ;
      UINT64 sortBufSz = sdbGetRTNCB()->getAPM()->getSortBufferSizeMB() ;
      SINT64 limit = numToReturn ;

      if ( 0 < limit && 0 < numToSkip )
      {
         limit += numToSkip ;
      }

      rc = _sorting.init( sortBufSz, orderby,
                          contextID(), limit, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init sort:%d", rc ) ;
         goto error ;
      }

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      _returnOptions.setSkip( numToSkip ) ;
      _returnOptions.setLimit( numToReturn ) ;
      _numToSkip = numToSkip ;
      _numToReturn = numToReturn ;

      rc = _rebuildSrcContext( orderby, context ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rebuild src context:%d", rc ) ;
         goto error ;
      }

      if ( NULL != context->getPlanRuntime() )
      {
         _planRuntime.inheritRuntime( context->getPlanRuntime() ) ;
      }
      _setSubContext( context, cb ) ;
      _orderby = orderby.getOwned() ;
      _keyGen = _ixmIndexKeyGen( _orderby ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextSort::setQueryActivity ( BOOLEAN hitEnd )
   {
      if ( _planRuntime.canSetQueryActivity() &&
           enabledMonContext() &&
           enabledQueryActivity() )
      {
         _planRuntime.setQueryActivity( MON_SELECT, _monCtxCB, _returnOptions,
                                        hitEnd ) ;
      }
   }

   INT32 _rtnContextSort::_rebuildSrcContext( const BSONObj &orderBy,
                                              rtnContext *srcContext )
   {
      INT32 rc = SDB_OK ;
      const BSONObj &selector = srcContext->getSelector().getPattern() ;
      if ( selector.isEmpty() )
      {
         goto done ;
      }
      else
      {
         BOOLEAN needRebuild = FALSE ;
         if ( srcContext->getSelector().getStringOutput() )
         {
            needRebuild = TRUE ;
         }
         else
         {
            rtnNeedResetSelector( selector, orderBy, needRebuild ) ;
         }

         if ( needRebuild )
         {
            rc = srcContext->getSelector().move( _selector ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rebuild selector:%d", rc ) ;
               goto error ;
            }
            _returnOptions.setSelector( _selector.getPattern() ) ;
         }
         else
         {
            _returnOptions.setSelector( selector ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextSort::_sortData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf bufObj ;
      BSONObj obj ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         _getSubContext()->getMonCB()->recordStartTimestamp() ;
      }

      for ( ; ; )
      {
         rc = _getSubContext()->getMore( -1, bufObj, _getSubContextCB() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = _sorting.sort( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sort: %d", rc ) ;
               goto error ;
            }
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to getmore:%d", rc ) ;
            goto error ;
         }

         while ( SDB_OK == ( rc = bufObj.nextObj( obj ) ) )
         {
            BSONElement arrEle ;
            BSONObjSet keySet( _orderby ) ;
            rc = _keyGen.getKeys( obj, keySet, &arrEle ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed gen sort keys:%d", rc ) ;
               goto error ;
            }

            SDB_ASSERT( !keySet.empty(), "can not be empty" ) ;
            const BSONObj &keyObj = *(keySet.begin() ) ;

            rc = _sorting.push( keyObj,
                                obj.objdata(), obj.objsize(),
                                &arrEle, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to push obj: %d", rc ) ;
               goto error ;
            }
         }

         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "failed to get next obj from objBuf: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextSort::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      const INT32 maxNum = 1000000 ;
      const INT32 breakBufferSize = 2097152 ; /// 2MB
      const INT32 minRecordNum = 4 ;
      BSONObj key ;
      BSONObj obj ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_dataSorted )
      {
         rc = _sortData( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sort data:%d", rc ) ;
            goto error ;
         }
         _dataSorted = TRUE ;
      }

      for ( INT32 i = 0; i < maxNum; i++ )
      {
         const CHAR* objdata ;
         INT32 objlen ;
         rc = _sorting.fetch( key, &objdata, &objlen, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fetch from sorting:%d", rc ) ;
            goto error ;
         }
         else if ( 0 < _numToSkip )
         {
            -- _numToSkip ;
            /// wo do not want to break this loop when get nothing.
            -- i ;
            continue ;
         }
         else if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else
         {
            const BSONObj *record = NULL ;
            BSONObj selected ;
            obj = BSONObj( objdata ) ;
            if ( _selector.isInitialized() )
            {
               rc = _selector.select( obj, selected ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to select fields from obj:%d", rc ) ;
                  goto error ;
               }
               record = &selected ;
            }
            else
            {
               record = &obj ;
            }

            rc = append( *record ) ;
            PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                      obj.toString().c_str(), rc ) ;

            if ( 0 < _numToReturn )
            {
               -- _numToReturn ;
            }
         }

         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;

         if ( minRecordNum <= i && buffEndOffset() >= breakBufferSize )
         {
            break ;
         }

         if ( buffEndOffset() + DMS_RECORD_MAX_SZ > RTN_RESULTBUFFER_SIZE_MAX )
         {
            break ;
         }
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextSort::_toString( stringstream &ss )
   {
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
      if ( !_orderby.isEmpty() )
      {
         ss << ",Orderby:" << _orderby.toString().c_str() ;
      }
   }

   /*
      _rtnContextTemp implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextTemp, RTN_CONTEXT_TEMP, "TEMP")

   _rtnContextTemp::_rtnContextTemp( INT64 contextID, UINT64 eduID )
   :_rtnContextData( contextID, eduID )
   {
   }

   _rtnContextTemp::~_rtnContextTemp ()
   {
      // release temp collection
      if ( _dmsCB && _mbContext )
      {
         _dmsCB->getTempSUMgr()->release( _mbContext ) ;
      }
   }

   const CHAR* _rtnContextTemp::name() const
   {
      return "TEMP" ;
   }

   RTN_CONTEXT_TYPE _rtnContextTemp::getType () const
   {
      return RTN_CONTEXT_TEMP ;
   }

}
