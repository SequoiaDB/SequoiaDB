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

   Source File Name = rtnRecover.cpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/08/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/


#include "rtnRecover.hpp"
#include "pmdStartup.hpp"
#include "rtn.hpp"
#include "rtnContextData.hpp"
#include "dmsStorageDataCapped.hpp"
#include "dmsIndexBuilder.hpp"

using namespace bson ;

namespace engine
{

   #define RTN_REORG_FILE_SUBFIX       ".REORG"

   #define RTN_REBUILD_RESET_LSN       ( (UINT64)~0 )

   #define RTN_REBUILD_MAX_LSN_DIFF    ( 32 * 1024 * 1024 )

   /*
      _rtnRBDupKeyProcessor define and implement
    */
   #define RTN_RBDK_FILE_MAX     ( 500 * 1024 * 1024 )
   #define RTN_RBDK_FILE_PREFIX  ( 200 )

   class _rtnRBDupKeyProcessor : public dmsDupKeyProcessor
   {
   public:
      _rtnRBDupKeyProcessor( const CHAR *clFullName,
                             utilCLUniqueID clUniqueID ) ;
      virtual ~_rtnRBDupKeyProcessor() ;

   public:
      virtual INT32 processDupKeyRecord( dmsStorageData *suData,
                                         dmsMBContext *mbContext,
                                         const dmsRecordID &recordID,
                                         ossValuePtr recordDataPtr,
                                         pmdEDUCB *eduCB ) ;

   protected:
      INT32 _writeToFile( const CHAR *buffer,
                          UINT32 bufferSize ) ;

   protected:
      const CHAR *   _clFullName ;
      OSSFILE        _dkFile ;
      CHAR           _dkFilePath[ OSS_MAX_PATHSIZE + 1 ] ;
      UINT32         _dkFilePrefixLen ;
      UINT32         _curFileSize ;
      UINT32         _fileID ;
   } ;

   typedef class _rtnRBDupKeyProcessor rtnRBDupKeyProcessor ;

   _rtnRBDupKeyProcessor::_rtnRBDupKeyProcessor( const CHAR *clFullName,
                                                 utilCLUniqueID clUniqueID )
   : _clFullName( clFullName ),
     _curFileSize( 0 ),
     _fileID( 0 )
   {
      const CHAR *dbPath = pmdGetOptionCB()->getDbPath() ;
      time_t timestamp = time( NULL ) ;
      CHAR timeBuffer[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { '\0' } ;
      CHAR filePrefix[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { '\0' } ;
      UINT32 prefix = 0 ;

      // file name: <cs.cl>_<unique ID>_<timestamp>.dup.<file ID>
      // e.g. foo.bar_12345_2020-01-20.17.18.19.dup.0

      // construct timestamp of file
      utilAscTime( timestamp, timeBuffer, OSS_TIMESTAMP_STRING_LEN ) ;

      // cut collection name when it is too long
      ossStrncpy( filePrefix, clFullName,
                  RTN_RBDK_FILE_PREFIX - ossStrlen( dbPath ) ) ;
      prefix = ossStrlen( filePrefix ) ;
      // construct file name prefix
      ossSnprintf( filePrefix + prefix, OSS_MAX_PATHSIZE - prefix + 1,
                   "_%llu_%s.dup", clUniqueID, timeBuffer ) ;

      // construct file name with DB path
      utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                         filePrefix, OSS_MAX_PATHSIZE, _dkFilePath ) ;

      _dkFilePrefixLen = ossStrlen( _dkFilePath ) ;
   }

   _rtnRBDupKeyProcessor::~_rtnRBDupKeyProcessor()
   {
      if ( _dkFile.isOpened() )
      {
         ossClose( _dkFile ) ;
      }
   }

   INT32 _rtnRBDupKeyProcessor::processDupKeyRecord( dmsStorageData *suData,
                                                     dmsMBContext *mbContext,
                                                     const dmsRecordID &recordID,
                                                     ossValuePtr recordDataPtr,
                                                     pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN lockHere = FALSE ;
      INT32 origLockType = mbContext->mbLockType() ;

      // should lock exclusive
      if ( EXCLUSIVE != origLockType )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s] for "
                      "exclusive, rc: %d", _clFullName, rc ) ;
         lockHere = TRUE ;
      }

      try
      {
         BSONObj record ;
         ossPoolString recordStr ;

         // extract duplicated key record
         if ( 0 == recordDataPtr )
         {
            rc = suData->fetch( mbContext, recordID, record, eduCB, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to fetch duplicated key record "
                         "for collection [%s] at ( extent %d, offset %d ), "
                         "rc: %d", _clFullName, recordID._extent,
                         recordID._offset, rc ) ;
         }
         else
         {
            record = BSONObj( (CHAR*)recordDataPtr ) ;
         }

         // need full text and no exception
         recordStr = record.toPoolString( false, true, false ) ;

         // write record to duplicated key file
         rc = _writeToFile( recordStr.c_str(), recordStr.size() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write duplicated key record "
                      "for collection [%s] at ( extent %d, offset %d ) "
                      "to file, rc: %d", _clFullName, recordID._extent,
                      recordID._offset, rc ) ;

         // remove duplicated key record
         rc = suData->deleteRecord( mbContext, recordID, 0, eduCB, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove duplicated key record "
                      "for collection [%s] at ( extent %u offset %u ), "
                      "rc: %d", _clFullName, recordID._extent,
                      recordID._offset, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to process duplicated key record for "
                 "collection [%s] at ( extent %u, offset %u ), "
                 "occur exception: %s", _clFullName, recordID._extent,
                 recordID._offset, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      // resume lock
      if ( lockHere )
      {
         if ( SHARED == origLockType )
         {
            INT32 tmpRC = mbContext->mbLock( origLockType ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDERROR, "Failed to lock collection [%s] for shared, "
                       "rc: %d", _clFullName, tmpRC ) ;
               if ( SDB_OK == rc )
               {
                  rc = tmpRC ;
               }
            }
         }
         else
         {
            mbContext->mbUnlock() ;
         }
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _rtnRBDupKeyProcessor::_writeToFile( const CHAR *buffer,
                                              UINT32 bufferSize )
   {
      INT32 rc = SDB_OK ;

      if ( bufferSize + _curFileSize > RTN_RBDK_FILE_MAX ||
           !_dkFile.isOpened() )
      {
         // close previous duplicated key file
         if ( _dkFile.isOpened() )
         {
            ossClose( _dkFile ) ;
         }

         // append file ID
         ossSnprintf( _dkFilePath + _dkFilePrefixLen,
                      OSS_MAX_PATHSIZE - _dkFilePrefixLen + 1,
                      ".%u", _fileID ++ ) ;

         // open new file
         rc = ossOpen( _dkFilePath, OSS_REPLACE | OSS_WRITEONLY,
                       OSS_RU | OSS_WU | OSS_RG, _dkFile ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open output file [%s], rc: %d",
                      _dkFilePath, rc ) ;

         PD_LOG( PDEVENT, "Open duplicated key file [%s] for collection [%s]",
                 _dkFilePath, _clFullName ) ;

         _curFileSize = 0 ;
      }

      SDB_ASSERT( _dkFile.isOpened(), "file should be opened" ) ;

      // write data
      rc = ossWriteN( &_dkFile, buffer, bufferSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write data to file [%s], rc: %d",
                   _dkFilePath, rc ) ;

      _curFileSize += bufferSize ;

      if ( _curFileSize < RTN_RBDK_FILE_MAX )
      {
         // write a enter into file
         rc = ossWriteN( &_dkFile, OSS_NEWLINE, OSS_NEWLINE_SIZE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write data to file [%s], rc: %d",
                      _dkFilePath, rc ) ;
         _curFileSize += OSS_NEWLINE_SIZE ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   _rtnCLRebuilderBase::_rtnCLRebuilderBase( dmsStorageUnit *pSU,
                                             const CHAR *pCLShortName )
   {
      _pSU = pSU ;
      _clName = pCLShortName ;
      _clFullName = _pSU->CSName() ;
      _clFullName += "." ;
      _clFullName += _clName ;

      _totalRecord = 0 ;
      _totalLob = 0 ;
      _indexNum = 0 ;
   }

   _rtnCLRebuilderBase::~_rtnCLRebuilderBase()
   {
      _release() ;
   }

   INT32 _rtnCLRebuilderBase::rebuild( pmdEDUCB *cb, rtnRUInfo *ruInfo )
   {
      INT32 rc = SDB_OK ;
      dmsMBContext *pContext = NULL ;

      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;

      /// first release
      _release() ;

      rc = _pSU->data()->getMBContext( &pContext,
                                       _clName.c_str(),
                                       EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Lock collection[%s] failed, rc: %d",
                 _clFullName.c_str(), rc ) ;
         goto error ;
      }

      beginTick.sample() ;
      PD_LOG( PDEVENT, "Begin to rebuild collection[%s]...",
              _clFullName.c_str() ) ;

      rc = _doRebuild( pContext, cb, ruInfo ) ;
      if ( rc )
      {
         goto error ;
      }

      _onRebuildDone() ;

   done:
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;
      /// release resource
      if ( pContext )
      {
         _pSU->data()->releaseMBContext( pContext ) ;
      }
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Rebuild collection[%s] succeed, cost: %u(s), "
                 "Total Record: %llu, Total Lob: %llu, Index Num: %u",
                 _clFullName.c_str(), seconds, _totalRecord,
                 _totalLob, _indexNum ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Rebuild collection[%s] failed, rc: %d, "
                 "cost: %u(s)", _clFullName.c_str(), rc, seconds ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilderBase::recover( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      dmsMBContext *pContext = NULL ;

      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;

      /// first release
      _release() ;

      rc = _pSU->data()->getMBContext( &pContext,
                                       _clName.c_str(),
                                       EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Lock collection[%s] failed, rc: %d",
                 _clFullName.c_str(), rc ) ;
         goto error ;
      }

      beginTick.sample() ;
      PD_LOG( PDEVENT, "Begin to recover collection[%s]...",
              _clFullName.c_str() ) ;

      rc = _doRecover( pContext, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      _onRecoverDone() ;

   done:
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;
      /// release resource
      if ( pContext )
      {
         _pSU->data()->releaseMBContext( pContext ) ;
      }
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Recover collection[%s] succeed, cost: %u(s)",
                 _clFullName.c_str(), seconds ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Recover collection[%s] failed, rc: %d, "
                 "cost: %u(s)", _clFullName.c_str(), rc, seconds ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilderBase::reorg( pmdEDUCB *cb, const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      dmsMBContext *pContext = NULL ;

      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;

      /// first release
      _release() ;

      rc = _pSU->data()->getMBContext( &pContext,
                                       _clName.c_str(),
                                       EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Lock collection[%s] failed, rc: %d",
                 _clFullName.c_str(), rc ) ;
         goto error ;
      }

      beginTick.sample() ;
      PD_LOG( PDEVENT, "Begin to reorg collection[%s]...",
              _clFullName.c_str() ) ;

      rc = _doReorg( pContext, cb, hint ) ;
      if ( rc )
      {
         goto error ;
      }

      _onRebuildDone() ;

   done:
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;
      /// release resource
      if ( pContext )
      {
         _pSU->data()->releaseMBContext( pContext ) ;
      }
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Reorg collection[%s] succeed, cost: %u(s), "
                 "Total Record: %llu, Total Lob: %llu, Index Num: %u",
                 _clFullName.c_str(), seconds, _totalRecord,
                 _totalLob, _indexNum ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Reorg collection[%s] failed, rc: %d, "
                 "cost: %u(s)", _clFullName.c_str(), rc, seconds ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _rtnCLRebuilderBase::_release()
   {
      _totalRecord = 0 ;
      _totalLob = 0 ;
      _indexNum = 0 ;
   }

   /*
      _rtnCLRebuilder implement
   */
   _rtnCLRebuilder::_rtnCLRebuilder( dmsStorageUnit *pSU,
                                     const CHAR *pCLShortName )
   : rtnCLRebuilderBase( pSU, pCLShortName )
   {
   }

   _rtnCLRebuilder::~_rtnCLRebuilder()
   {
   }

   INT32 _rtnCLRebuilder::_rebuild( pmdEDUCB *cb,
                                    dmsMBContext *mbContext,
                                    rtnRUInfo *ruInfo )
   {
      INT32 rc = SDB_OK ;

      if ( ruInfo->isAllValid() )
      {
         PD_LOG( PDEVENT, "Collection[%s] is valid, don't need to rebuild",
                 _clFullName.c_str() ) ;
         goto done ;
      }

      /// lock mb context
      rc = mbContext->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Lock collection mb context failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( 0 == ruInfo->_dataCommitFlag )
      {
         /// force to index rebuild
         ruInfo->_idxCommitFlag = 0 ;
         rc = _rebuildData( cb, mbContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild collection data record failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDEVENT, "Collection[%s]'s data file is valid, don't need "
                 "to rebuild", _clFullName.c_str() ) ;
         /// change to rebuild
         DMS_SET_MB_OFFLINE_REORG_REBUILD( mbContext->mb()->_flag ) ;
      }

      if ( 0 == ruInfo->_idxCommitFlag )
      {
         /// rebuild index
         rc = _rebuildIndex( cb, mbContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild collection indexes failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDEVENT, "Collection[%s]'s index file is valid, don't need "
                 "to rebuild", _clFullName.c_str() ) ;
         /// Change to normal
         DMS_SET_MB_NORMAL( mbContext->mb()->_flag ) ;
         _pSU->data()->flushMeta( TRUE ) ;
      }

      if ( 0 == ruInfo->_lobCommitFlag )
      {
         rc = _rebuildLob( cb, mbContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild collection lob failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      else if ( _pSU->data()->getHeader()->_createLobs )
      {
         PD_LOG( PDEVENT, "Collection[%s]'s lob file is valid, don't need "
                 "to rebuild", _clFullName.c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnCLRebuilder::_cleanRegSU()
   {
      string orgFileName ;

      /// build path
      CHAR  tmpName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                         _clFullName.c_str(), OSS_MAX_PATHSIZE,
                         tmpName ) ;
      orgFileName = tmpName ;
      orgFileName += RTN_REORG_FILE_SUBFIX ;

      if ( SDB_OK == ossAccess( orgFileName.c_str(), 0 ) )
      {
         INT32 rc = ossDelete( orgFileName.c_str() ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Remove the reorg file[%s] succeed",
                    orgFileName.c_str() ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Remove the reorg file[%s] failed, rc: %d",
                    orgFileName.c_str(), rc ) ;
         }
      }
   }

   INT32 _rtnCLRebuilder::_openRegSU( dmsReorgUnit *pSU,
                                      BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      string orgFileName ;

      /// build path
      CHAR  tmpName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                         _clFullName.c_str(), OSS_MAX_PATHSIZE,
                         tmpName ) ;
      orgFileName = tmpName ;
      orgFileName += RTN_REORG_FILE_SUBFIX ;

      if ( createNew )
      {
         /// first to delete the file
         if ( SDB_OK == ossAccess( orgFileName.c_str(), 0 ) )
         {
            rc = ossDelete( orgFileName.c_str() ) ;
            if ( SDB_OK == rc )
            {
               PD_LOG( PDEVENT, "Remove the old reorg file[%s] succeed",
                       orgFileName.c_str() ) ;
            }
         }
      }
      rc = pSU->open( orgFileName.c_str(),
                      _pSU->getPageSize(),
                      _pSU->data()->getSegmentSize(),
                      createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "File to create org file[%s], rc: %d",
                 orgFileName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_exportOneExtent( pmdEDUCB *cb,
                                            dmsMBContext *mbContext,
                                            dmsReorgUnit *pSU,
                                            dmsExtentID extID,
                                            BOOLEAN forward,
                                            UINT32 &remainPages,
                                            dmsExtentID &nextExtID,
                                            BOOLEAN &valid )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW extRW ;
      const dmsExtent *extent = NULL ;

      if ( extID < 0 ||
           (UINT32)extID >= _pSU->data()->getHeader()->_pageNum )
      {
         PD_LOG( PDERROR, "Extent id[%d] is invalid", extID) ;
         valid = FALSE ;
         goto error ;
      }

      extRW = _pSU->data()->extent2RW( extID, -1 ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.readPtr<dmsExtent>() ;
      if ( !extent )
      {
         PD_LOG( PDERROR, "Get extent[%d]'s address failed", extID ) ;
         valid = FALSE ;
         goto error ;
      }

      if ( forward )
      {
         nextExtID = extent->_nextExtent ;
      }
      else
      {
         nextExtID = extent->_prevExtent ;
      }

      if ( !extent->validate( mbContext->mbID() ) ||
           extent->_blockSize == 0 ||
           extent->_blockSize > _pSU->data()->segmentPages() ||
           extent->_blockSize > remainPages ||
           nextExtID == extID )
      {
         PD_LOG( PDERROR, "Extent[%d] is invalid", extID ) ;
         valid = FALSE ;
         goto error ;
      }

      remainPages -= extent->_blockSize ;

      rc = _exportByAExtent( cb, mbContext, pSU, extID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_exportByExtents( pmdEDUCB *cb,
                                            dmsMBContext *mbContext,
                                            dmsReorgUnit *pSU )
   {
      INT32 rc = SDB_OK ;
      UINT32 extentNum = 0 ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;
      dmsExtentID nextExtID = DMS_INVALID_EXTENT ;
      dmsExtentID forwardStopExt = DMS_INVALID_EXTENT ;
      dmsExtentID backwardStopExt = DMS_INVALID_EXTENT ;
      BOOLEAN valid = TRUE ;
      BOOLEAN damaged = FALSE ;
      BOOLEAN needBackward = TRUE ;
      UINT32 maxRemainPages = _pSU->data()->pageNum() ;
      dmsExtentID firstExtInMB = mbContext->mb()->_firstExtentID ;
      dmsExtentID lastExtInMB = mbContext->mb()->_lastExtentID ;

      nextExtID = firstExtInMB ;
      while( DMS_INVALID_EXTENT != nextExtID )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         extentID = nextExtID ;
         rc = _exportOneExtent( cb, mbContext, pSU, extentID, TRUE,
                                maxRemainPages, nextExtID, valid ) ;
         if ( rc )
         {
            goto error ;
         }

         // If the last extent is hit during the forward scanning, then no
         // backward scanning is needed.
         if ( needBackward && ( extentID == lastExtInMB ) )
         {
            needBackward = FALSE ;
         }

         // Once an invalid extent is found, we stop the traverse.
         if ( !valid )
         {
            damaged = TRUE ;
            break ;
         }

         ++extentNum ;
      }

      forwardStopExt = extentID ;

      // If we haven't reached the last extent, it means some extents at the
      // middle of the extent list are corrupted. In that case, continue to scan
      // the extents backwards in order to save as much data as possible.
      if ( needBackward )
      {
         if ( !damaged )
         {
            damaged = TRUE ;
         }

         valid = TRUE ;

         nextExtID = lastExtInMB ;
         while ( DMS_INVALID_EXTENT != nextExtID )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }

            extentID = nextExtID ;
            rc = _exportOneExtent( cb, mbContext, pSU, extentID, FALSE,
                                   maxRemainPages, nextExtID, valid ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( !valid )
            {
               break ;
            }

            ++extentNum ;

            if ( nextExtID == forwardStopExt )
            {
               break ;
            }
         }
         backwardStopExt = extentID ;
      }

      if ( damaged )
      {
         PD_LOG( PDWARNING, "Collection[%s]'s extent list is damaged. First "
                 "extent in mb:%d, last extent in mb:%d, forward processed "
                 "last extent:%d, backward processed last extent:%d. "
                 "Processed extent count:%u",
                 _clFullName.c_str(), firstExtInMB, lastExtInMB,
                 forwardStopExt, backwardStopExt, extentNum ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_exportByAExtent( pmdEDUCB *cb,
                                            dmsMBContext *mbContext,
                                            dmsReorgUnit *pSU,
                                            dmsExtentID extentID )
   {
      INT32 rc = SDB_OK ;

      UINT32 recordNum = 0 ;
      const dmsRecord *pRecord = NULL ;
      const dmsExtent *pExtent = NULL ;
      dmsOffset nextOffset = DMS_INVALID_OFFSET ;
      UINT32 extentSize = 0 ;
      dmsExtRW extRW ;
      dmsRecordID rid( extentID, DMS_INVALID_OFFSET ) ;
      dmsRecordRW recordRW ;
      dmsRecordData recordData ;
      dmsCompressorEntry *pEntry = NULL ;
      std::set<dmsOffset> offsetSet ;
      std::pair<std::set<dmsOffset>::iterator, bool> result ;

      pEntry = _pSU->data()->getCompressorEntry( mbContext->mbID() ) ;
      extRW = _pSU->data()->extent2RW( extentID, -1 ) ;
      extRW.setNothrow( TRUE ) ;
      pExtent = extRW.readPtr<dmsExtent>() ;
      if ( !pExtent )
      {
         PD_LOG( PDERROR, "Get extent[%d] address failed", extentID ) ;
         /// not report this error
         goto done ;
      }
      extentSize = pExtent->_blockSize * _pSU->getPageSize() ;

      /// scan backward
      nextOffset = pExtent->_firstRecordOffset ;
      while( DMS_INVALID_OFFSET != nextOffset )
      {
         if ( nextOffset < (INT32)sizeof(dmsExtent) ||
              nextOffset > (INT32)(extentSize - sizeof(dmsRecord) )  )
         {
            /// offset error
            PD_LOG( PDERROR, "Reocrd[%d.%d]'s next offset[%d] is error",
                    extentID, rid._offset, nextOffset ) ;
            break ;
         }
         rid._offset = nextOffset ;
         // We use a set to find the possible record link list circle in case
         // of data corruption. If found, ignore the remainning ones in the
         // list.
         try
         {
            result = offsetSet.insert( rid._offset ) ;
            if ( !result.second )
            {
               PD_LOG( PDERROR, "Record list circle found at record[%d.%d]. "
                       "Stop scanning the current extent", rid._extent,
                       rid._offset ) ;
               break ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         recordRW = _pSU->data()->record2RW( rid, mbContext->mbID() ) ;
         recordRW.setNothrow( TRUE ) ;
         pRecord = recordRW.readPtr() ;
         if ( !pRecord )
         {
            PD_LOG( PDERROR, "Get record[%d.%d] address failed",
                    rid._extent, rid._offset ) ;
            break ;
         }
         /// set next
         nextOffset = pRecord->getNextOffset() ;
         /// record is delete
         if ( pRecord->isDeleted() || pRecord->isDeleting() )
         {
            continue ;
         }
         /// State is wrong
         if ( !pRecord->isNormal() && !pRecord->isOvf() )
         {
            PD_LOG( PDERROR, "Record[%d.%d]'s state is wrong [%d]",
                    rid._extent, rid._offset, pRecord->getAttr() );
            continue ;
         }
         /// Wrong compressed flag
         if ( pRecord->isCompressed() &&
              !OSS_BIT_TEST( mbContext->mb()->_attributes,
                             DMS_MB_ATTR_COMPRESSED ) )
         {
            // Skip the corrupted record in below cases:
            // 1. old version compression which is not alterable
            // 2. wrong compression type
            if ( !OSS_BIT_TEST( mbContext->mb()->_compressFlags,
                                UTIL_COMPRESS_ALTERABLE_FLAG ) )
            {
               PD_LOG( PDERROR, "Record[%d.%d] should not be compressed" ) ;
               continue ;
            }
            else if ( NULL == getCompressorByType(
                           (UTIL_COMPRESSOR_TYPE)pRecord->getCompressType() ) )
            {
               PD_LOG( PDERROR, "Record[%d.%d] with wrong compression type" ) ;
               continue ;
            }
         }
         /// extract data
         rc = _pSU->data()->extractData( mbContext, recordRW,
                                         cb, recordData ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extract record[%d.%d] data failed, rc: %d",
                    rid._extent, rid._offset, rc ) ;
            rc = SDB_OK ;
            continue ;
         }

         /// write data to reorg file
         try
         {
            BSONObj obj( recordData.data() ) ;
            // In case of power failure, any kind of corruption may happen.
            // There was one time that one record was damaged. The BSONObj EOO
            // flag was not right. Then later in traversing of the object, when
            // reaching the expected end, it was treated as an element. Later
            // crash happened when try to get the size of the element.
            // So in the rebuilding phase, we scan all the record objects, to
            // make sure that they are valid BSON object( About 11% performance
            // lose in the test). If they are not, exception is expected to
            // happen and the current record will be ignored.
            BSONObjIterator itr( obj );
            while ( TRUE )
            {
               BSONElement e = itr.next() ;
               if ( true == e.eoo() )
               {
                  break ;
               }
            }

            rc = pSU->insertRecord( obj, cb, pEntry ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Insert rid[%d.%d] record data[%s] to "
                       "reorg file failed, rc: %d", rid._extent, rid._offset,
                       obj.toString().c_str(), rc ) ;
               goto error ;
            }
            ++recordNum ;
            ++_totalRecord ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Insert rid[%d.%d] record data to reorg file "
                    "occur exception: %s", rid._extent, rid._offset,
                    e.what() ) ;
            /// not goto error
         }
      }

      if ( rid._offset != pExtent->_lastRecordOffset ||
           recordNum != pExtent->_recCount )
      {
         PD_LOG( PDWARNING, "Extent[%d] is damaged, Last record offset:%d, "
                 "Record count:%u, Processed last offset:%d, Processed "
                 "record count:%u", extentID, pExtent->_lastRecordOffset,
                 pExtent->_recCount, rid._offset, recordNum ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_copyBack( pmdEDUCB *cb,
                                     dmsMBContext *mbContext,
                                     dmsReorgUnit *pRU )
   {
      INT32 rc             = SDB_OK ;

      CHAR *blockBuffer    = NULL ;
      UINT32 blockBuffSize = 0 ;
      INT32 blockSize      = 0 ;

      pRU->beginExport() ;

      // loop for each block
      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         // get the next block
         rc = pRU->getNextExtentSize( blockSize ) ;
         if ( rc )
         {
            // if we get end of file, that means we don't have "
            // any other blocks to copy, then we break the loop
            if ( SDB_EOF == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_LOG ( PDERROR, "Failed to get next extent size, rc: %d", rc ) ;
            goto error ;
         }

         if ( blockBuffSize < (UINT32)blockSize )
         {
            if ( blockBuffer )
            {
               cb->releaseBuff( blockBuffer ) ;
               blockBuffer = NULL ;
               blockBuffSize = 0 ;
            }
            rc = cb->allocBuff( (UINT32)blockSize, &blockBuffer,
                                &blockBuffSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate memory[%u], rc: %d",
                        blockSize, rc ) ;
               goto error ;
            }
         }

         // get the extent
         rc = pRU->exportExtent( blockBuffer ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to export extent, rc: %d", rc ) ;
            goto error ;
         }
         // load the extent into dms
         rc = _pSU->loadExtent ( mbContext, blockBuffer,
                                 (UINT16)( blockSize/_pSU->getPageSize() ) ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed load extent into DMS, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      if ( blockBuffer )
      {
         cb->releaseBuff( blockBuffer ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnCLRebuilder::_rebuildData( pmdEDUCB *cb,
                                        dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      dmsReorgUnit regSU ;
      UINT16 flag = mbContext->mb()->_flag ;
      BOOLEAN canClean = FALSE ;
      UINT16 phase = 0 ;

      if ( (flag & DMS_MB_OPR_TYPE_MASK) == DMS_MB_FLAG_OFFLINE_REORG )
      {
         phase = flag & DMS_MB_OPR_PHASE_MASK ;
      }
      else
      {
         phase = DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ;
      }

      if ( DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY == phase )
      {
         rc = _openRegSU( &regSU, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open reorg file failed, rc: %d", rc ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Start rebuild collection[%s]'s data with reorg "
                 "file[%s]", _clFullName.c_str(), regSU.getFileName() ) ;

         canClean = TRUE ;
         /// shadow copy
         DMS_SET_MB_OFFLINE_REORG_SHADOW_COPY( flag ) ;
         mbContext->mb()->_flag = flag ;
         PD_LOG( PDEVENT, "Begin shadow copy phase" ) ;

         rc = _exportByExtents( cb, mbContext, &regSU ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Shadow copy failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = regSU.flush() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to flush data to reorg file, rc: %d", rc ) ;
            goto error ;
         }

         /// truncate
         DMS_SET_MB_OFFLINE_REORG_TRUNCATE( flag ) ;
         mbContext->mb()->_flag = flag ;
         phase = DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ;
      }

      if ( DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE == phase ||
           DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK == phase )
      {
         PD_LOG( PDEVENT, "Begin truncate phase" ) ;

         /// when truncate failed, can't clean the reorg file
         canClean = FALSE ;
         rc = _pSU->data()->truncateCollection( _clName.c_str(), cb, NULL,
                                                TRUE, mbContext, FALSE,
                                                FALSE ) ;
         if ( ( SDB_OK != rc ) && ( SDB_DMS_CORRUPTED_EXTENT != rc ) )
         {
            PD_LOG( PDERROR, "Truncate collection[%s] failed, rc: %d",
                    _clFullName.c_str(), rc ) ;
            goto error ;
         }

         /// copyback
         DMS_SET_MB_OFFLINE_REORG_COPY_BACK( flag ) ;
         mbContext->mb()->_flag = flag ;
         phase = DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ;

         if ( !regSU.isOpened() )
         {
            rc = _openRegSU( &regSU, FALSE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Open reorg file failed, rc: %d", rc ) ;
               goto error ;
            }
         }
         PD_LOG( PDEVENT, "Begin copyback phase" ) ;

         rc = _copyBack( cb, mbContext, &regSU ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to copyback data to collection[%s], "
                    "rc: %d", _clFullName.c_str(), rc ) ;
            goto error ;
         }

         /// flush data
         _pSU->data()->flushAll( TRUE ) ;
         /// change to rebuild
         DMS_SET_MB_OFFLINE_REORG_REBUILD( flag ) ;
         mbContext->mb()->_flag = flag ;
         phase = DMS_MB_FLAG_OFFLINE_REORG_REBUILD ;

         /// when change to rebuild, can't clean reorg file
         canClean = TRUE ;

         /// data file is restored
         mbContext->mbStat()->_commitFlag.init( 1 ) ;
         mbContext->mbStat()->_isCrash = FALSE ;
         mbContext->mbStat()->_lastLSN.init( RTN_REBUILD_RESET_LSN ) ;
         mbContext->mb()->_commitFlag = 1 ;
         mbContext->mb()->_commitLSN = (UINT64)~0 ;

         mbContext->mbStat()->_idxCommitFlag.init( 0 ) ;
         mbContext->mbStat()->_idxIsCrash = TRUE ;
         mbContext->mb()->_idxCommitFlag = 0 ;

         /// flush meta
         _pSU->data()->flushMeta( TRUE ) ;
      }

   done:
      if ( canClean )
      {
         INT32 rcTmp = regSU.cleanup() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Failed to cleanup reorg file[%s], rc: %d",
                    regSU.getFileName(), rcTmp ) ;
         }
      }
      return rc ;
   error:
      /// Failed when copyback, need to reset the flag normal
      if ( DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY == phase )
      {
         DMS_SET_MB_NORMAL(flag) ;
         mbContext->mb()->_flag = flag ;
      }
      goto done ;
   }

   INT32 _rtnCLRebuilder::_rebuildIndex( pmdEDUCB *cb,
                                         dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      UINT16 flag = mbContext->mb()->_flag ;
      UINT16 phase = 0 ;

      if ( (flag & DMS_MB_OPR_TYPE_MASK) == DMS_MB_FLAG_OFFLINE_REORG )
      {
         phase = flag & DMS_MB_OPR_PHASE_MASK ;
      }

      if ( DMS_MB_FLAG_OFFLINE_REORG_REBUILD == phase )
      {
         PD_LOG( PDEVENT, "Start rebuild phase" ) ;

         rtnRBDupKeyProcessor dkProcessor( _clFullName.c_str(),
                                           mbContext->mb()->_clUniqueID ) ;

         rc = _pSU->index()->rebuildIndexes( mbContext, cb,
                                             SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                             &dkProcessor ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild indexes failed, rc: %d", rc ) ;
            goto error ;
         }

         /// Release the lock before flush.
         mbContext->mbUnlock() ;

         /// flush all
         _pSU->index()->flushAll( TRUE ) ;

         /// Take the lock again to modify meta data.
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Lock mb context failed, rc: %d", rc ) ;
            goto error ;
         }

         /// Change status
         DMS_SET_MB_NORMAL( flag ) ;
         mbContext->mb()->_flag = flag ;

         mbContext->mb()->_idxCommitFlag = 1 ;
         mbContext->mb()->_idxCommitLSN = (UINT64)~0 ;
         mbContext->mbStat()->_idxCommitFlag.init( 1 ) ;
         mbContext->mbStat()->_idxLastLSN.init( RTN_REBUILD_RESET_LSN ) ;
         mbContext->mbStat()->_idxIsCrash = FALSE ;

         _indexNum = mbContext->mb()->_numIndexes ;

         _pSU->data()->flushMeta( TRUE ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_rebuildLob( pmdEDUCB *cb,
                                       dmsMBContext *mbContext )
   {
      /// do nothing
      mbContext->mb()->_lobCommitFlag = 1 ;
      mbContext->mb()->_lobCommitLSN = (UINT64)~0 ;
      mbContext->mbStat()->_lobCommitFlag.init( 1 ) ;
      mbContext->mbStat()->_lobLastLSN.init( RTN_REBUILD_RESET_LSN ) ;
      mbContext->mbStat()->_lobIsCrash = FALSE ;

      _totalLob = mbContext->mbStat()->_totalLobs ;

      _pSU->data()->flushMeta( TRUE ) ;

      return SDB_OK ;
   }

   INT32 _rtnCLRebuilder::_recover( pmdEDUCB *cb,
                                    dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      UINT16 flag = 0 ;

      rc = mbContext->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Lock collection mb context failed, rc: %d", rc ) ;
         goto error ;
      }

      flag = mbContext->mb()->_flag ;

      if ( ( flag & DMS_MB_OPR_TYPE_MASK ) == DMS_MB_FLAG_OFFLINE_REORG )
      {
         if ( DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ==
              ( flag & DMS_MB_OPR_PHASE_MASK ) )
         {
            /// we can recover directly
            DMS_SET_MB_NORMAL( flag ) ;
            mbContext->mb()->_flag = flag ;
            goto done ;
         }

         _totalRecord = mbContext->mb()->_totalRecords ;
         /// rebuild data
         rc = _rebuildData( cb, mbContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Recover collection data record failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         /// rebuild index
         rc = _rebuildIndex( cb, mbContext ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Recover collection indexes failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         if ( 0 == mbContext->mb()->_lobCommitFlag &&
              _pSU->data()->getHeader()->_createLobs )
         {
            rc = _rebuildLob( cb, mbContext ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Recover collection lob failed, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
      }
      else if ( (flag & DMS_MB_OPR_TYPE_MASK) == DMS_MB_FLAG_ONLINE_REORG )
      {
         // online reorg
         PD_LOG ( PDERROR, "Online reorg recover is not supported yet" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }
      else
      {
         PD_LOG ( PDWARNING, "Collection is not in reorg status" ) ;
         rc = SDB_DMS_NOT_IN_REORG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_reorgData( pmdEDUCB *cb,
                                      dmsMBContext *mbContext,
                                      const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      UINT16 flag = 0 ;
      UINT16 phase = DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ;

      BSONObj dummyObj ;
      SINT64 contextID = -1 ;
      rtnContextData *context = NULL ;
      dmsReorgUnit regSU ;

      /// In query will create mbcontext, so in here, we need to unlock
      mbContext->mbUnlock() ;

      /// begin to query data
      rc = rtnQuery( _clFullName.c_str(), dummyObj, dummyObj, dummyObj,
                     hint, 0, cb, 0, -1, dmsCB, rtnCB, contextID,
                     (rtnContextBase**)&context ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            // if the collection is completely empty
            PD_LOG ( PDEVENT, "Empty collection is detected, "
                     "reorg is skipped" ) ;
            rc = SDB_OK ;
            contextID = -1 ;
         }
         PD_LOG ( PDERROR, "Failed to query, rc = %d", rc ) ;
         goto error ;
      }

      // let's lock the collection using exclusive mode
      rc = context->getMBContext()->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to lock collection %s, rc = %d",
                  _clFullName.c_str(), rc ) ;
         goto error ;
      }
      if ( context->getMBContext()->mbID() != mbContext->mbID() ||
           context->getMBContext()->clLID() != mbContext->clLID() )
      {
         /// collection has re-create or truncated, so not rebuild
         goto done ;
      }

      /// open reorg su
      rc = _openRegSU( &regSU, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open reorg file failed, rc: %d", rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Start offline reorg, use reorg file:%s",
              regSU.getFileName() ) ;

      flag = mbContext->mb()->_flag ;
      /// shadow copy
      DMS_SET_MB_OFFLINE_REORG_SHADOW_COPY( flag ) ;
      mbContext->mb()->_flag = flag ;
      PD_LOG( PDEVENT, "Begin shadow copy phase" ) ;

      if ( -1 != contextID )
      {
         /// export data
         rtnContextBuf buffObj ;
         dmsCompressorEntry *compEntry = NULL ;
         compEntry = _pSU->data()->getCompressorEntry( mbContext->mbID() ) ;

         while( TRUE )
         {
            rc = context->getMore( 1, buffObj, cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else if ( SDB_APP_INTERRUPT == rc )
            {
               goto error ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Query data from collection[%s] failed, "
                       "rc: %d", _clFullName.c_str(), rc ) ;
               goto error ;
            }

            try
            {
               BSONObj obj( buffObj.data() ) ;
               rc = regSU.insertRecord( obj, cb, compEntry ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to insert obj[%s] to reorg file, "
                          "rc: %d", obj.toString().c_str(), rc ) ;
                  goto error ;
               }
               ++_totalRecord ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Failed to build bson obj: %s", e.what() ) ;
               /// the bson is crashed, not goto error
            }
         }

         rc = regSU.flush() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to flush data to reorg file, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

      // let's lock the collection using exclusive mode
      rc = context->getMBContext()->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to lock collection %s, rc = %d",
                  _clFullName.c_str(), rc ) ;
         goto error ;
      }

      /// set to truncate
      /// shadow copy
      DMS_SET_MB_OFFLINE_REORG_TRUNCATE( flag ) ;
      mbContext->mb()->_flag = flag ;
      phase = DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ;

      regSU.close() ;
      rc = _recover( cb, context->getMBContext() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Recover collection[%s] failed, rc: %d",
                 _clFullName.c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      if ( DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY == phase )
      {
         regSU.cleanup() ;
         DMS_SET_MB_NORMAL( flag ) ;
         mbContext->mb()->_flag = flag ;
      }
      goto done ;
   }

   INT32 _rtnCLRebuilder::_doRebuild( dmsMBContext *context, pmdEDUCB *cb,
                                      rtnRUInfo *ruInfo )
   {
      INT32 rc = SDB_OK ;
      UINT16 flag = 0 ;

      SDB_ASSERT( context, "Context can not be NULL" ) ;
      SDB_ASSERT( cb, "cb can not be NULL" ) ;
      SDB_ASSERT( ruInfo, "ruInfo can not be NULL" ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold exclusive lock on mb" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      flag = context->mb()->_flag ;
      if ( ( DMS_IS_MB_OFFLINE_REORG( flag ) &&
             !DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY( flag ) ) ||
           ( DMS_IS_MB_ONLINE_REORG ( flag ) ) )
      {
         /// recover
         rc = _recover( cb, context ) ;
         PD_RC_CHECK( rc, PDERROR, "Recover collection[%s] failed, rc: %d",
                      _clFullName.c_str(), rc ) ;
      }
      else
      {
         /// rebuild
         rc = _rebuild( cb, context, ruInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Rebuild collection[%s] failed, rc: %d",
                      _clFullName.c_str(), rc ) ;

      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_onRebuildDone()
   {
      _cleanRegSU() ;
      return SDB_OK ;
   }

   INT32 _rtnCLRebuilder::_doRecover( dmsMBContext *context, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rc = _recover( cb, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Recover collection[%s] failed, rc: %d",
                   _clFullName.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_onRecoverDone()
   {
      _cleanRegSU() ;
      return SDB_OK ;
   }

   INT32 _rtnCLRebuilder::_doReorg( dmsMBContext *context, pmdEDUCB *cb,
                                    const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      UINT16 flag = 0 ;

      flag = context->mb()->_flag ;

      if ( ( DMS_IS_MB_OFFLINE_REORG( flag ) &&
             !DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY( flag ) ) ||
           ( DMS_IS_MB_ONLINE_REORG ( flag ) ) )
      {
         /// recover
         rc = _recover( cb, context ) ;
         PD_RC_CHECK( rc, PDERROR, "Recover collection[%s] failed, rc: %d",
                      _clFullName.c_str(), rc ) ;
      }
      else
      {
         /// reorg
         rc = _reorgData( cb, context, hint ) ;
         PD_RC_CHECK( rc, PDERROR, "Reorganize data of collection[%s] failed, "
                      "rc: %d", _clFullName.c_str(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_onReorgDone()
   {
      _cleanRegSU() ;
      return SDB_OK ;
   }

   _rtnCappedCLRebuilder::_rtnCappedCLRebuilder( dmsStorageUnit *pSU,
                                                 const CHAR *pCLShortName )
   : rtnCLRebuilderBase( pSU, pCLShortName )
   {
   }

   _rtnCappedCLRebuilder::~_rtnCappedCLRebuilder()
   {
   }

   INT32 _rtnCappedCLRebuilder::_doRebuild( dmsMBContext *context, pmdEDUCB *cb,
                                            rtnRUInfo *ruInfo )
   {
      return _doRecover( context, cb ) ;
   }

   INT32 _rtnCappedCLRebuilder::_onRebuildDone()
   {
      return SDB_OK ;
   }

   // For capped collection, the main target of restore is to restore the last
   // extent information, bacause we use a working extent buffer to store the
   // information when inserting, so the extent header is not always update
   // immediately. In this case, if problems like crash or power off happened,
   // the header of the last extent is expired. That's what needs to be
   // restored.
   INT32 _rtnCappedCLRebuilder::_doRecover( dmsMBContext *context,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rc = _rebuildData( context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Rebuild data for collection[%s] failed, "
                   "rc: %d", _clFullName.c_str(), rc ) ;
      rc = _rebuildLob( context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Rebuild lob for collection[%s] failed, rc: %d",
                   _clFullName.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCappedCLRebuilder::_onRecoverDone()
   {
      return SDB_OK ;
   }

   INT32 _rtnCappedCLRebuilder::_doReorg( dmsMBContext *context, pmdEDUCB *cb,
                                          const BSONObj &hint )
   {
      return _doRecover( context, cb ) ;
   }

   INT32 _rtnCappedCLRebuilder::_onReorgDone()
   {
      return SDB_OK ;
   }

   INT32 _rtnCappedCLRebuilder::_rebuildData( dmsMBContext *context,
                                              pmdEDUCB *cb )
   {
      dmsExtentID currentExt = DMS_INVALID_EXTENT ;
      dmsExtentID lastExtent = DMS_INVALID_EXTENT ;
      dmsExtentID lastValidExt = DMS_INVALID_EXTENT ;
      UINT32 totalExtNum = 0 ;
      UINT32 remainSpace = 0 ;

      currentExt = context->mb()->_firstExtentID ;
      lastExtent = context->mb()->_lastExtentID ;

      // Traverse the extent list to find out the last valid extent.
      while ( DMS_INVALID_EXTENT != currentExt )
      {
         dmsExtRW extRW = _pSU->data()->extent2RW( currentExt,
                                                   context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
         if ( !extent || !extent->validate(context->mbID()) )
         {
            break ;
         }
         else
         {
            // recover the extent
            _recoverOneExtent( currentExt, extent, context, remainSpace ) ;
            _totalRecord += extent->_recCount ;
         }
         totalExtNum++ ;
         lastValidExt = currentExt ;
         if ( lastValidExt == lastExtent )
         {
            break ;
         }
         currentExt = extent->_nextExtent ;
      }
      if ( lastValidExt != lastExtent )
      {
         context->mb()->_lastExtentID = lastValidExt ;
      }

      _pSU->data()->postDataRestored( context ) ;
      _pSU->data()->flushAll( TRUE ) ;

      context->mbStat()->_commitFlag.init( 1 ) ;
      context->mbStat()->_isCrash = FALSE ;
      context->mbStat()->_lastLSN.init( RTN_REBUILD_RESET_LSN ) ;
      context->mb()->_commitFlag = 1 ;
      context->mb()->_commitLSN = (UINT64)~0 ;
      context->mb()->_totalRecords = _totalRecord ;
      context->mb()->_totalDataPages =
         totalExtNum << _pSU->data()->pageSizeSquareRoot() ;
      // In capped collection, the free space is only the remainning free space
      // in the last extent.
      context->mb()->_totalDataFreeSpace = remainSpace ;
      context->mbStat()->_totalRecords = _totalRecord ;
      context->mbStat()->_rcTotalRecords.init( _totalRecord ) ;

      _pSU->data()->flushMeta( TRUE ) ;

      return SDB_OK ;
   }

   INT32 _rtnCappedCLRebuilder::_rebuildLob( dmsMBContext *context,
                                             pmdEDUCB *cb )
   {
      /// do nothing
      context->mb()->_lobCommitFlag = 1 ;
      context->mb()->_lobCommitLSN = (UINT64)~0 ;
      context->mbStat()->_lobCommitFlag.init( 1 ) ;
      context->mbStat()->_lobLastLSN.init( RTN_REBUILD_RESET_LSN ) ;
      context->mbStat()->_lobIsCrash = FALSE ;

      _totalLob = context->mbStat()->_totalLobs ;

      _pSU->data()->flushMeta( TRUE ) ;

      return SDB_OK ;
   }

   // Traverse all the records in one extent, validate each record, and update
   // the extent header if neccessary.
   void _rtnCappedCLRebuilder::_recoverOneExtent( dmsExtentID extentID,
                                                  const dmsExtent *extent,
                                                  dmsMBContext *mbContext,
                                                  UINT32 &remainSpace )
   {
      UINT32 recCount = 0 ;
      dmsOffset recordOffset = DMS_INVALID_OFFSET ;
      dmsOffset firstRecOffset = DMS_INVALID_OFFSET ;
      dmsOffset lastValidOffset = DMS_INVALID_OFFSET ;
      INT64 logicalID = -1 ;
      _extLidAndOffset2RecLid( extent->_logicID, DMS_EXTENT_METADATA_SZ,
                               logicalID ) ;

      remainSpace = DMS_CAP_EXTENT_SZ ;
      recordOffset = extent->_firstRecordOffset ;

      while ( DMS_INVALID_OFFSET != recordOffset &&
              recordOffset < DMS_CAP_EXTENT_SZ )
      {
         dmsRecordID recordID( extentID, recordOffset ) ;
         dmsRecordRW recRW = _pSU->data()->record2RW( recordID,
                                                      mbContext->mbID() ) ;
         recRW.setNothrow( TRUE ) ;
         const dmsCappedRecord *record = recRW.readPtr<dmsCappedRecord>() ;
         // Only when the record is normal, and the logical id in the record
         // header is as expected that the record is valid.
         // If invalid, stop, and treate it as the last record in this extent.
         if ( !record || !record->isNormal() ||
              ( logicalID != record->getLogicalID() ) )
         {
            break ;
         }
         // If the first record is valid, remember it for checking below.
         if ( recordOffset == extent->_firstRecordOffset )
         {
            firstRecOffset = recordOffset ;
         }
         recCount++ ;
         lastValidOffset = recordOffset ;
         recordOffset += record->getSize() ;
         remainSpace = DMS_CAP_EXTENT_SZ - recordOffset ;
         logicalID += record->getSize() ;
      }

      // If the diagnose information is not the same with the extent header,
      // update the extent header.
      if ( recCount != extent->_recCount ||
           firstRecOffset != extent->_firstRecordOffset ||
           lastValidOffset != extent->_lastRecordOffset )
      {
         dmsExtRW extRWTmp = _pSU->data()->extent2RW( extentID,
                                                      mbContext->mbID() ) ;
         extRWTmp.setNothrow( TRUE ) ;
         dmsExtent *extentTmp = extRWTmp.writePtr<dmsExtent>() ;
         SDB_ASSERT( extentTmp, "Extent pointer is not possible to be NULL" ) ;
         extentTmp->_recCount = recCount ;
         extentTmp->_firstRecordOffset = firstRecOffset ;
         extentTmp->_lastRecordOffset = lastValidOffset ;
         extentTmp->_freeSpace = remainSpace ;
         _pSU->data()->flushPages( extentID, extentTmp->_blockSize ) ;
      }
   }

   void _rtnCappedCLRebuilder::_extLidAndOffset2RecLid( dmsExtentID extLID,
                                                        dmsOffset offset,
                                                        INT64 &logicalID )
   {
      if ( DMS_INVALID_EXTENT == extLID ||
           offset < (INT32)DMS_EXTENT_METADATA_SZ )
      {
         logicalID = DMS_INVALID_REC_LOGICALID ;
      }
      else
      {
         logicalID = (INT64)extLID * DMS_CAP_EXTENT_BODY_SZ + offset
                     - DMS_EXTENT_METADATA_SZ ;
      }
   }

   _rtnCLRebuilderFactory::_rtnCLRebuilderFactory()
   {
   }

   _rtnCLRebuilderFactory::~_rtnCLRebuilderFactory()
   {
   }

   INT32 _rtnCLRebuilderFactory::create( dmsStorageUnit *pSU,
                                         const CHAR *pCLShortName,
                                         rtnCLRebuilderBase *&rebuilder )
   {
      INT32 rc = SDB_OK ;

      if ( DMS_STORAGE_NORMAL == pSU->type() )
      {
         rebuilder = SDB_OSS_NEW rtnCLRebuilder( pSU, pCLShortName ) ;
      }
      else if ( DMS_STORAGE_CAPPED == pSU->type() )
      {
         rebuilder = SDB_OSS_NEW rtnCappedCLRebuilder( pSU, pCLShortName ) ;
      }
      else
      {
         rc = SDB_DMS_INVALID_SU ;
         goto error ;
      }
      if ( !rebuilder )
      {
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnCLRebuilderFactory::release( rtnCLRebuilderBase *rebuilder )
   {
      if ( rebuilder )
      {
         SDB_OSS_DEL rebuilder ;
         rebuilder = NULL ;
      }
   }

   rtnCLRebuilderFactory* rtnGetCLRebuilderFactory()
   {
      static rtnCLRebuilderFactory s_factory ;
      return &s_factory ;
   }

   /*
      _rtnRecoverUnit implement
   */
   _rtnRecoverUnit::_rtnRecoverUnit()
   {
      _pSU = NULL ;
      _maxLsn = DPS_INVALID_LSN_OFFSET ;
      _maxValidLsn = DPS_INVALID_LSN_OFFSET ;
   }

   _rtnRecoverUnit::~_rtnRecoverUnit()
   {
   }

   INT32 _rtnRecoverUnit::init( dmsStorageUnit *pSu )
   {
      dmsStorageDataCommon    *pData   = NULL ;
      dmsStorageLob     *pLob    = NULL ;

      _pSU = pSu ;

      pData = _pSU->data() ;
      pLob = _pSU->lob() ;

      rtnRUInfo info ;
      string clFullName ;

      /// analyse the file
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         DPS_LSN_OFFSET tmpMaxlLsn = DPS_INVALID_LSN_OFFSET ;
         const dmsMB *mb = pData->getMBInfo( i ) ;
         if ( DMS_IS_MB_INUSE ( mb->_flag ) )
         {
            info._dataCommitFlag = mb->_commitFlag ;
            info._dataCommitLSN = mb->_commitLSN ;
            info._idxCommitFlag = mb->_idxCommitFlag ;
            info._idxCommitLSN = mb->_idxCommitLSN ;
            if ( !pLob->isOpened() )
            {
               info._lobCommitFlag = 1 ;
               info._lobCommitLSN = 0 ;
            }
            else
            {
               info._lobCommitFlag = mb->_lobCommitFlag ;
               info._lobCommitLSN = mb->_lobCommitLSN ;
            }
            ossStrncpy( info._clName, mb->_collectionName,
                        DMS_COLLECTION_NAME_SZ ) ;
            info._clName[ DMS_COLLECTION_NAME_SZ ] = 0 ;

            clFullName = _pSU->CSName() ;
            clFullName += "." ;
            clFullName += mb->_collectionName ;
            /// add to map
            _clStatus[ clFullName ] = info ;

            tmpMaxlLsn = info.maxLSN() ;
            if ( DPS_INVALID_LSN_OFFSET != tmpMaxlLsn )
            {
               if ( DPS_INVALID_LSN_OFFSET == _maxLsn ||
                    _maxLsn < tmpMaxlLsn )
               {
                  _maxLsn = tmpMaxlLsn ;
               }
            }

            tmpMaxlLsn = info.maxValidLSN() ;
            if ( DPS_INVALID_LSN_OFFSET != tmpMaxlLsn )
            {
               if ( DPS_INVALID_LSN_OFFSET == _maxValidLsn
                    || _maxValidLsn < tmpMaxlLsn )
               {
                  _maxValidLsn = tmpMaxlLsn ;
               }
            }

            PD_LOG( PDINFO, "Collection[%s] commit status[DataFlag:%u, "
                    "DataLSN:%llu, IdxFlag:%u, IdxLSN:%llu, LobFlag:%u, "
                    "LobLSN:%llu]", clFullName.c_str(), info._dataCommitFlag,
                    info._dataCommitLSN, info._idxCommitFlag,
                    info._idxCommitLSN, info._lobCommitFlag,
                    info._lobCommitLSN ) ;
         }
      }

      return SDB_OK ;
   }

   void _rtnRecoverUnit::release()
   {
      _pSU = NULL ;
      _maxLsn = DPS_INVALID_LSN_OFFSET ;
      _clStatus.clear() ;
   }

   INT32 _rtnRecoverUnit::restore( pmdEDUCB *cb )
   {
      /// reserved
      return SDB_OK ;
   }

   BOOLEAN _rtnRecoverUnit::isAllValid() const
   {
      MAP_SU_STATUS::const_iterator cit = _clStatus.begin() ;
      while( cit != _clStatus.end() )
      {
         if ( !( cit->second ).isAllValid() )
         {
            return FALSE ;
         }
         ++cit ;
      }
      return TRUE ;
   }

   BOOLEAN _rtnRecoverUnit::isAllInvalid() const
   {
      MAP_SU_STATUS::const_iterator cit = _clStatus.begin() ;
      while( cit != _clStatus.end() )
      {
         if ( ( cit->second ).isAllValid() )
         {
            return FALSE ;
         }
         ++cit ;
      }
      return TRUE ;
   }

   UINT32 _rtnRecoverUnit::getValidCLItem( MAP_SU_STATUS &items )
   {
      items.clear() ;
      MAP_SU_STATUS::iterator it = _clStatus.begin() ;
      while( it != _clStatus.end() )
      {
         rtnRUInfo &item = it->second ;
         if ( item.isAllValid() )
         {
            items[ it->first ] = it->second ;
         }
         ++it ;
      }
      return items.size() ;
   }

   UINT32 _rtnRecoverUnit::getInvalidCLItem( MAP_SU_STATUS &items )
   {
      items.clear() ;
      MAP_SU_STATUS::iterator it = _clStatus.begin() ;
      while( it != _clStatus.end() )
      {
         rtnRUInfo &item = it->second ;
         if ( !item.isAllValid() )
         {
            items[ it->first ] = it->second ;
         }
         ++it ;
      }
      return items.size() ;
   }

   UINT32 _rtnRecoverUnit::getCLItems( MAP_SU_STATUS &item )
   {
      item = _clStatus ;
      return item.size() ;
   }

   rtnRUInfo* _rtnRecoverUnit::getItem( const string &name )
   {
      MAP_SU_STATUS::iterator it = _clStatus.find( name ) ;
      if ( it != _clStatus.end() )
      {
         return &(it->second) ;
      }
      return NULL ;
   }

   void _rtnRecoverUnit::setAllInvalid()
   {
      MAP_SU_STATUS::iterator it = _clStatus.begin() ;
      while( it != _clStatus.end() )
      {
         it->second.setAllInvalid() ;
         ++it ;
      }
   }

   INT32 _rtnRecoverUnit::cleanup( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;
      UINT32 dropCount = 0 ;

      beginTick.sample() ;
      PD_LOG( PDEVENT, "Begin to cleanup collectionspace[%s]...",
              _pSU->CSName() ) ;

      MAP_SU_STATUS::iterator it ;

      /// if the lob is invalid, rebuild the bme
      if ( _pSU->lob()->isOpened() && _pSU->lob()->isCrashed() )
      {
         rc = _pSU->lob()->rebuildBME() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild lob[%s]'s BME failed, rc: %d",
                    _pSU->lob()->getSuFileName(), rc ) ;
            goto error ;
         }
      }

      /// drop invalid collection
      for ( it = _clStatus.begin() ; it != _clStatus.end() ; ++it )
      {
         rtnRUInfo &info = it->second ;

         if ( !info.isAllValid() )
         {
            rc = _pSU->data()->dropCollection( info._clName, cb, NULL,
                                               TRUE, NULL ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Drop collection[%s] failed, rc: %d",
                       info._clName, rc ) ;
               goto error ;
            }
            else
            {
               PD_LOG( PDEVENT, "Drop collection[%s] succeed",
                       info._clName ) ;
               ++dropCount ;
            }
         }
      }

   done:
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;

      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Cleanup collectionspace[%s] succeed, "
                 "cost: %u(s), Dropped Collection Num: %u",
                 _pSU->CSName(), seconds, dropCount ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Cleanup collectionspace[%s] failed, rc: %d, "
                 "cost: %u(s)", _pSU->CSName(), rc, seconds ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnRecoverUnit::rebuild( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;

      UINT32 totalNum = _clStatus.size() ;
      UINT32 sucNum = 0 ;
      MAP_SU_STATUS::iterator it ;
      rtnCLRebuilderFactory *factory = rtnGetCLRebuilderFactory() ;

      beginTick.sample() ;
      PD_LOG( PDEVENT, "Begin to rebuild collectionspace[%s]...",
              _pSU->CSName() ) ;

      /// disable sync
      _pSU->enableSync( FALSE ) ;

      /// if the lob is invalid, rebuild thd bme
      if ( _pSU->lob()->isOpened() && _pSU->lob()->isCrashed() )
      {
         rc = _pSU->lob()->rebuildBME() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild lob[%s]'s BME failed, rc: %d",
                    _pSU->lob()->getSuFileName(), rc ) ;
            goto error ;
         }
      }

      /// rebuild collections
      for ( it = _clStatus.begin() ; it != _clStatus.end() ; ++it )
      {
         rtnRUInfo &info = it->second ;
         rtnCLRebuilderBase *rebuilder = NULL ;
         rc = factory->create( _pSU, info._clName, rebuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Create collection rebuilder failed, rc: %d",
                      rc ) ;
         rc = rebuilder->rebuild( cb, &info ) ;
         if ( rc )
         {
            factory->release( rebuilder ) ;
            if ( SDB_APP_INTERRUPT != rc )
            {
               PD_LOG( PDERROR, "Rebuild collection[%s] failed, rc: %d",
                       info._clName, rc ) ;
            }
            goto error ;
         }
         factory->release( rebuilder ) ;
         ++sucNum ;
      }

      /// all the collectionspace rebuild ok
      _pSU->restoreForCrash() ;

   done:
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;

      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Rebuild collectionspace[%s] succeed, "
                 "cost: %u(s), Total Collection Num: %u, "
                 "Succeed Collection Num: %u", _pSU->CSName(),
                 seconds, totalNum, sucNum ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Rebuild collectionspace[%s] failed, rc: %d, "
                 "cost: %u(s), Total Collection Num: %u, "
                 "Succeed Collection Num: %u", _pSU->CSName(), rc,
                 seconds, totalNum, sucNum ) ;
      }
      /// enable sync
      _pSU->enableSync( TRUE ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnDBOprBase::_rewriteCLCommitLSN( _SDB_DMSCB *dmsCB,
                                             dmsStorageUnit *su,
                                             MAP_SU_STATUS &validCLs,
                                             DPS_LSN_OFFSET dpsMaxLSN )
   {
      INT32 rc = SDB_OK ;
      MAP_SU_STATUS::iterator statusIter ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      const CHAR *pCLShortName = NULL ;
      dmsMBContext *pContext = NULL ;

      for( statusIter = validCLs.begin(); statusIter != validCLs.end();
           ++statusIter )
      {
         BOOLEAN needFlush = FALSE ;
         rtnRUInfo &info = statusIter->second ;
         suID = DMS_INVALID_SUID ;
         pContext = NULL ;

         /// not all valid, continue
         if ( !info.isAllValid() )
         {
            continue ;
         }

         rc = rtnResolveCollectionNameAndLock( statusIter->first.c_str(), dmsCB,
                                               &su, &pCLShortName, suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection[%s], rc: %d",
                      statusIter->first.c_str(), rc ) ;

         /// get mb context
         rc = su->data()->getMBContext( &pContext, pCLShortName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection[%s]'s mblock, "
                      "rc: %d", statusIter->first.c_str(), rc ) ;

         if ( DPS_INVALID_LSN_OFFSET == pContext->mb()->_commitLSN
              || pContext->mb()->_commitLSN > dpsMaxLSN )
         {
            UINT64 oldCommitLSN = pContext->mb()->_commitLSN ;
            pContext->mbStat()->_lastLSN.init( dpsMaxLSN ) ;
            pContext->mb()->_commitLSN = dpsMaxLSN ;
            PD_LOG( PDEVENT, "Flush collection[%s]'s commitlsn "
                    "from[%lld] to [%lld]",
                    statusIter->first.c_str(), oldCommitLSN, dpsMaxLSN ) ;

            needFlush = TRUE ;
         }

         if ( DPS_INVALID_LSN_OFFSET == pContext->mb()->_idxCommitLSN
              || pContext->mb()->_idxCommitLSN > dpsMaxLSN )
         {
            UINT64 oldCommitLSN = pContext->mb()->_idxCommitLSN ;
            pContext->mbStat()->_idxLastLSN.init( dpsMaxLSN ) ;
            pContext->mb()->_idxCommitLSN = dpsMaxLSN ;
            PD_LOG( PDEVENT, "Flush collection[%s]'s idxCommitLSN "
                    "from[%lld] to [%lld]",
                    statusIter->first.c_str(), oldCommitLSN, dpsMaxLSN ) ;

            needFlush = TRUE ;
         }

         if ( 0 != su->data()->getHeader()->_createLobs
              && pContext->mb()->_totalLobPages > 0
              && ( DPS_INVALID_LSN_OFFSET == pContext->mb()->_lobCommitLSN
                   || pContext->mb()->_lobCommitLSN > dpsMaxLSN ) )
         {
            UINT64 oldCommitLSN = pContext->mb()->_lobCommitLSN ;
            pContext->mbStat()->_lobLastLSN.init( dpsMaxLSN ) ;
            pContext->mb()->_lobCommitLSN = dpsMaxLSN ;
            PD_LOG( PDEVENT, "Flush collection[%s]'s lobCommitLSN"
                    " from[%lld] to [%lld]",
                    statusIter->first.c_str(), oldCommitLSN, dpsMaxLSN ) ;

            needFlush = TRUE ;
         }

         if ( needFlush )
         {
            /// flush meta
            rc = su->data()->flushMeta( TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to flush meta, cl: %s, rc: %d",
                         statusIter->first.c_str(), rc ) ;

            PD_LOG( PDEVENT, "Flush all commit lsn success" ) ;
         }

         su->data()->releaseMBContext( pContext ) ;
         pContext = NULL ;

         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }

   done:
      if ( NULL != pContext && NULL != su )
      {
         su->data()->releaseMBContext( pContext ) ;
         pContext = NULL ;
      }

      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnDBOprBase::_rewriteCommitLSN( _SDB_DMSCB *dmsCB,
                                           MON_CS_SIM_LIST &csList,
                                           DPS_LSN_OFFSET dpsMaxLSN )
   {
      INT32 rc = SDB_OK ;
      MON_CS_SIM_LIST::iterator it ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      for ( it = csList.begin() ; it != csList.end() ; ++it )
      {
         const monCSSimple &csInfo = *it ;
         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }

         suID = DMS_INVALID_SUID ;
         dmsStorageUnit *su = NULL ;
         rc = dmsCB->nameToSUAndLock( csInfo._name, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                    csInfo._name, rc ) ;
            goto error ;
         }

         rtnRecoverUnit recoverUnit ;
         rc = recoverUnit.init( su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init recover unit, rc: %d", rc ) ;

         MAP_SU_STATUS validCLs ;
         recoverUnit.getValidCLItem( validCLs ) ;

         rc = _rewriteCLCommitLSN( dmsCB, su, validCLs, dpsMaxLSN ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rewrite cs[%s]'s commit lsn, "
                      "rc: %d", csInfo._name, rc ) ;

         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _rtnDBOprBase define
   */
   INT32 _rtnDBOprBase::doOpr( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      UINT64 maxLsn = DPS_INVALID_LSN_OFFSET ;

      if ( pmdGetStartup().isOK() )
      {
         /// don't need to rebuild
         return rc ;
      }

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      ossTick beginTick ;
      ossTick endTick ;
      ossTickDelta timeSpan ;
      ossTickConversionFactor factor ;
      UINT32 seconds = 0 ;
      UINT32 microSec = 0 ;

      UINT32 totalCount = 0 ;
      UINT32 sucNum = 0 ;

      MON_CS_SIM_LIST csList ;
      MON_CS_SIM_LIST::iterator it ;

      rc = _onBegin( cb ) ;
      if ( rc )
      {
         goto error ;
      }

      PD_LOG( PDEVENT, "Start %s database", oprName() ) ;
      beginTick.sample() ;

      if ( _lockDMS() )
      {
         rc = dmsCB->registerRebuild( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Register rebuild failed, rc: %d", rc ) ;
            goto error ;
         }
         hasLock = TRUE ;
      }

      /// dump all collectionspace
      rc = dmsCB->dumpInfo( csList, TRUE, FALSE, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }
      totalCount = csList.size() ;

      for ( it = csList.begin() ; it != csList.end() ; ++it )
      {
         const monCSSimple &csInfo = *it ;

         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            ++sucNum ;
            continue ;
         }

         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         dmsStorageUnit *su = NULL ;
         rc = dmsCB->nameToSUAndLock( csInfo._name, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                    csInfo._name, rc ) ;
            goto error ;
         }

         rtnRecoverUnit recoverUnit ;
         recoverUnit.init( su ) ;

         if ( DPS_INVALID_LSN_OFFSET == maxLsn ||
              maxLsn < recoverUnit.getMaxLsn() )
         {
            maxLsn = recoverUnit.getMaxLsn() ;
         }

         rc = _doOpr( cb, &recoverUnit, suID ) ;
         if ( DMS_INVALID_SUID != suID )
         {
            dmsCB->suUnlock( suID ) ;
            suID = DMS_INVALID_SUID ;
         }

         /// jduge the result
         if ( rc )
         {
            if ( SDB_APP_INTERRUPT != rc )
            {
               PD_LOG( PDERROR, "Do %s collectionspace[%s] failed, rc: %d",
                       oprName(), csInfo._name, rc ) ;
            }
            goto error ;
         }
         ++sucNum ;
      }

      /// move the dps
      if ( dpsCB && _cleanDPS() )
      {
         DPS_LSN expectLSN = dpsCB->expectLsn() ;

         if ( DPS_INVALID_LSN_OFFSET == expectLSN.offset )
         {
            expectLSN.offset = 0 ;
         }

         if ( DPS_INVALID_LSN_OFFSET != maxLsn &&
              expectLSN.offset < maxLsn )
         {
            if ( maxLsn - expectLSN.offset < RTN_REBUILD_MAX_LSN_DIFF )
            {
               expectLSN.offset = maxLsn ;
            }
            else
            {
               expectLSN.offset += RTN_REBUILD_MAX_LSN_DIFF ;
            }
         }

         if ( 0 == expectLSN.offset )
         {
            /// when rebuild, we can't move the dps to 0, because the new add
            /// node will sync from lsn 0
            expectLSN.offset = ossAlign4( (UINT32)sizeof( dpsLogRecordHeader ) ) ;
         }
         else
         {
            expectLSN.offset += ossAlign4( (UINT32)sizeof( dpsLogRecordHeader ) ) ;
         }

         if ( DPS_INVALID_LSN_VERSION == expectLSN.version )
         {
            expectLSN.version = DPS_INVALID_LSN_VERSION + 1 ;
         }

         /// clear transinfo
         sdbGetTransCB()->clearTransInfo() ;
         /// cut all dps
         dpsCB->move( 0, expectLSN.version ) ;
         /// then move to non-zero
         dpsCB->move( expectLSN.offset, expectLSN.version ) ;
         PD_LOG( PDEVENT, "Clean replica-logs succeed, move lsn to %lld.%lld",
                 expectLSN.version, expectLSN.offset ) ;

         // expectLSN.offset -1 is the impossible offset in dps, so this can
         // force other data-node to full-sync the collection that have be
         // rewritten.( SEQUOIADBMAINSTREAM-5738 )
         rc = _rewriteCommitLSN( dmsCB, csList, expectLSN.offset - 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rewrite commitLSN, rc: %d", rc ) ;
      }

      /// on end
      _onSucceed( cb ) ;

   done:
      if ( hasLock )
      {
         dmsCB->rebuildDown( cb ) ;
      }
      endTick.sample() ;
      timeSpan = endTick - beginTick ;
      timeSpan.convertToTime( factor, seconds, microSec ) ;

      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "%s database succeed, cost: %u(s), "
                 "Total CS Num: %u, Succeed CS Num: %u", oprName(),
                 seconds, totalCount, sucNum ) ;
      }
      else
      {
         PD_LOG( PDERROR, "%s database failed, rc: %d, "
                 "cost: %u(s), Total CS Num: %u, Succeed CS Num: %u",
                 oprName(), rc, seconds, totalCount, sucNum ) ;
      }
      return rc ;
   error:
      if ( SDB_IXM_DUP_KEY == rc )
      {
         // when found duplicated keys, data are in error status
         // move LSN to 0, so this node could not be PRIMARY again
         PD_LOG( PDERROR, "Clean replica-logs with duplicated key error" ) ;
         dpsCB->move( 0, 0 ) ;
      }
      goto done ;
   }

   /*
      _rtnDBRebuilder implement
   */
   INT32 _rtnDBRebuilder::_doOpr( pmdEDUCB *cb,
                                  rtnRecoverUnit *pUnit,
                                  dmsStorageUnitID &suID )
   {
      INT32 rc = SDB_OK ;

      if ( pUnit->isAllValid() )
      {
         pUnit->getSU()->restoreForCrash() ;
         PD_LOG( PDEVENT, "Collectionspace[%s] is valid, don't need to "
                 "rebuild", pUnit->getSU()->CSName() ) ;
      }
      else
      {
         rc = pUnit->rebuild( cb ) ;
      }
      return rc ;
   }

   void _rtnDBRebuilder::_onSucceed( pmdEDUCB *cb )
   {
      /// set ok
      pmdGetStartup().ok( TRUE ) ;
   }

   /*
      _rtnDBCleaner implement
   */
   INT32 _rtnDBCleaner::_doOpr( pmdEDUCB *cb,
                                rtnRecoverUnit *pUnit,
                                dmsStorageUnitID &suID )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      if ( _useUDF )
      {
#ifdef _DEBUG
         MAP_SU_STATUS validCLs ;
         pUnit->getValidCLItem( validCLs ) ;
#endif //_DEBUG

         pUnit->setAllInvalid() ;

         for ( UINT32 i = 0 ; i < _udfValidCLs.size() ; ++i )
         {
            rtnRUInfo *pInfo = pUnit->getItem( _udfValidCLs[i] ) ;
            if ( pInfo )
            {
               pInfo->setAllValid() ;
#ifdef _DEBUG
               SDB_ASSERT( validCLs.find( _udfValidCLs[i] ) !=
                           validCLs.end(), "Item must be Valid" ) ;
#endif //_DEBUG
            }
         }
      }

      if ( !pUnit->isAllValid() && !pUnit->isAllInvalid() )
      {
         rc = pUnit->cleanup( cb ) ;
         if ( rc )
         {
            pUnit->setAllInvalid() ;
            /// need to clean valid collection by collectionspace
            _removeCLsByCS( pUnit->getSU()->CSName() ) ;
         }
      }

      if ( pUnit->isAllInvalid() )
      {
         string csName = pUnit->getSU()->CSName() ;
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;

         rc = dmsCB->dropCollectionSpace( csName.c_str(), cb, NULL ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
      }

      return rc ;
   }

   void _rtnDBCleaner::_removeCLsByCS( const CHAR *csName )
   {
      UINT32 nameLen = ossStrlen( csName ) ;
      vector< string >::iterator it = _udfValidCLs.begin() ;
      while ( it != _udfValidCLs.end() )
      {
         if ( 0 == ossStrncmp( (*it).c_str(), csName, nameLen ) &&
              '.' == (*it).at( nameLen ) )
         {
            it = _udfValidCLs.erase( it ) ;
         }
         else
         {
            ++it ;
         }
      }
   }

   void _rtnDBCleaner::setUDFValidCLs( const vector< string > &vecValidCLs )
   {
      _useUDF = TRUE ;
      _udfValidCLs = vecValidCLs ;
   }

   /*
      _rtnDBFSPostCleaner implement
   */
   void _rtnDBFSPostCleaner::_onSucceed( pmdEDUCB *cb )
   {
      /// set ok
      pmdGetStartup().ok( TRUE ) ;
   }

   INT32 _rtnDBFSPostCleaner::_doOpr( pmdEDUCB *cb,
                                      rtnRecoverUnit *pUnit,
                                      dmsStorageUnitID &suID )
   {
      pUnit->getSU()->restoreForCrash() ;
      return SDB_OK ;
   }

}


