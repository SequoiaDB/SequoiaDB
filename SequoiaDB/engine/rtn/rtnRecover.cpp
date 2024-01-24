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
#include "dmsCB.hpp"

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
      virtual INT32 processDupKeyRecord( dmsStorageDataCommon *suData,
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

   INT32 _rtnRBDupKeyProcessor::processDupKeyRecord( dmsStorageDataCommon *suData,
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
         dmsRecordData recordData ;
         BSONObj record ;
         ossPoolString recordStr ;

         // extract duplicated key record
         if ( 0 == recordDataPtr )
         {
            rc = suData->fetch( mbContext, recordID, recordData, eduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to fetch duplicated key record "
                         "for collection [%s] at ( extent %d, offset %d ), "
                         "rc: %d", _clFullName, recordID._extent,
                         recordID._offset, rc ) ;
            record = BSONObj( recordData.data() ) ;
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

   INT32 _rtnCLRebuilder::_rebuildData( pmdEDUCB *cb,
                                        dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      rc = mbContext->getCollPtr()->validateData( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to validate data of colleciton [%s], "
                   "rc: %d", _clFullName.c_str(), rc ) ;

      PD_LOG( PDEVENT, "Validated data on collection [%s]",
              _clFullName.c_str() ) ;

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

      // rebuild data without index, so set index file crashed
      _pSU->index()->setCrashed() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCLRebuilder::_rebuildIndex( pmdEDUCB *cb,
                                         dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      // need to lock mb
      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      for ( INT32 indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++ indexID )
      {
         // rebuild for unique indexes
         // kick records with duplicated keys caused by transaction rollback before crash
         rtnRBDupKeyProcessor dkProcessor( _clFullName.c_str(),
                                           mbContext->mb()->_clUniqueID ) ;
         dmsExtentID idxExtent = mbContext->mb()->_indexExtent[ indexID ] ;
         if ( DMS_INVALID_EXTENT == idxExtent )
         {
            break ;
         }
         ixmIndexCB indexCB ( idxExtent, _pSU->index(), mbContext ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX;
            goto error ;
         }

         if ( !indexCB.unique() || indexCB.isIDIndex() )
         {
            continue ;
         }

         ossPoolString indexName( indexCB.getName() ) ;
         BSONObj indexDef( indexCB.getDef().copy() ) ;

         rc = _pSU->index()->rebuildIndex( mbContext, idxExtent, indexCB, cb,
                                           SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                           &dkProcessor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rebuild index [%s] on "
                      "collection [%s], rc: %d", indexName.c_str(),
                      _clFullName.c_str(), rc ) ;

         PD_LOG( PDEVENT, "Rebuilded index [%s] define [%s] on collection [%s]",
                 indexName.c_str(), indexDef.toPoolString().c_str(),
                 _clFullName.c_str() ) ;
      }

      {
         /// Change status
         UINT16 flag = mbContext->mb()->_flag ;
         DMS_SET_MB_NORMAL( flag ) ;
         mbContext->mb()->_flag = flag ;

         mbContext->mb()->_idxCommitFlag = 1 ;
         mbContext->mb()->_idxCommitLSN = (UINT64)~0 ;
         mbContext->mbStat()->_idxCommitFlag.init( 1 ) ;
         mbContext->mbStat()->_idxLastLSN.init( RTN_REBUILD_RESET_LSN ) ;
         mbContext->mbStat()->_idxIsCrash = FALSE ;

         _indexNum = mbContext->mb()->_numIndexes ;
      }

      _pSU->data()->flushMeta( TRUE ) ;

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

      _totalLob = mbContext->mbStat()->_totalLobs.fetch() ;

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

      dmsCompactOptions options ;

      /// In query will create mbcontext, so in here, we need to unlock
      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], "
                   "rc: %d", _clFullName.c_str(), rc ) ;

      rc = mbContext->getCollPtr()->compact( options, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reorganize data of collection [%s], "
                   "rc: %d", _clFullName.c_str(), rc ) ;

   done:
      return rc ;
   error:
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
      INT32 rc = SDB_OK ;

      rc = context->getCollPtr()->validateData( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to validate data of colleciton [%s], "
                   "rc: %d", _clFullName.c_str(), rc ) ;

      /// data file is restored
      context->mbStat()->_commitFlag.init( 1 ) ;
      context->mbStat()->_isCrash = FALSE ;
      context->mbStat()->_lastLSN.init( RTN_REBUILD_RESET_LSN ) ;
      context->mb()->_commitFlag = 1 ;
      context->mb()->_commitLSN = (UINT64)~0 ;

      context->mbStat()->_idxCommitFlag.init( 0 ) ;
      context->mbStat()->_idxIsCrash = TRUE ;
      context->mb()->_idxCommitFlag = 0 ;

      /// flush meta
      _pSU->data()->flushMeta( TRUE ) ;

      _pSU->data()->flushMeta( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
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

      _totalLob = context->mbStat()->_totalLobs.fetch() ;

      _pSU->data()->flushMeta( TRUE ) ;

      return SDB_OK ;
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


