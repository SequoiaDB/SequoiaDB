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

   Source File Name = rtnDictCreatorJob.cpp

   Descriptive Name = Rtn Dictionary Creating Job.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "dms.hpp"
#include "dmsCB.hpp"
#include "dmsScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"
#include "rtnTrace.hpp"
#include "rtnDictCreatorJob.hpp"
#include "rtn.hpp"

namespace engine
{
   /*
    * Threshold of record number and data total size to create dictionary:
    * 100 records and 64MB data.
    */
   #define RTN_DICT_CREATE_REC_NUM_THRESHOLD    100
   #define RTN_DICT_CREATE_REC_DATA_SIZE        ( 64 << 20 )

   _rtnDictCreatorJob::_rtnDictCreatorJob( UINT32 scanInterval )
   : _scanInterval( scanInterval )
   {
   }

   _rtnDictCreatorJob::~_rtnDictCreatorJob()
   {
   }

   RTN_JOB_TYPE _rtnDictCreatorJob::type () const
   {
      return RTN_JOB_CREATE_DICT;
   }

   const CHAR* _rtnDictCreatorJob::name() const
   {
      return "DictionaryCreator" ;
   }

   BOOLEAN _rtnDictCreatorJob::muteXOn ( const _rtnBaseJob *pOther )
   {
      return FALSE;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTN_DICTCREATORJOB_DOIT, "_rtnDictCreatorJob::doit" )
   INT32 _rtnDictCreatorJob::doit()
   {
      /*
       * This thread will check all the collections in the list. If the
       * condition of creating dictionary is matched, the dictionary of the
       * collection will be created. And once this is done successfully, the
       * collection will be removed from the list, and will never be put into
       * it again. Otherwise it will be put back into the list and try to create
       * again.
       * Note:
       * This job thread should be started after the dictionary caches of all
       * storage units have been created. That is done during the control block
       * initialization phase. So it's fine.
       */
      PD_TRACE_ENTRY ( SDB__RTN_DICTCREATORJOB_DOIT ) ;

      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdEDUMgr *eduMgr = krcb->getEDUMgr() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      pmdEDUCB *cb = eduCB() ;
      pmdEDUEvent event ;
      BOOLEAN foundJob = FALSE ;
      UINT64 lastStartTime = pmdGetDBTick() ;
      dmsDictJob job ;
      vector<dmsDictJob> tmpJobQue ;

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         // If the database is being rebuilding, collections may be truncated.
         // Let's wait until the node is OK.
         if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
         {
#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Node is in rebuilding status. Dictionary creator "
                    "will wait until the node is normal" ) ;
#endif /* _DEBUG */
            ossSleepsecs( 1 ) ;
            continue ;
         }

         BOOLEAN retry = FALSE ;
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
          */
         eduMgr->waitEDU( cb ) ;
         /* Get the first item in the dictionary waiting list. */
         foundJob = dmsCB->dispatchDictJob( job ) ;
         if ( !foundJob )
         {
            /* If no colleciton is waitting for dictionary creating, wait... */
            while ( pmdGetTickSpanTime( lastStartTime ) < _scanInterval )
            {
               cb->waitEvent( event, OSS_ONE_SEC ) ;
            }

            /// push tmp job to que
            for ( UINT32 i = 0 ; i < tmpJobQue.size() ; ++i )
            {
               dmsCB->pushDictJob( tmpJobQue[i] ) ;
            }
            tmpJobQue.clear() ;

            lastStartTime = pmdGetDBTick() ;
            continue ;
         }

         eduMgr->activateEDU( cb ) ;

         /*
          * Check with the fetched storage unit id and mb id. Any arror happened
          * during the creation of the dictionary, it should be skipped this
          * time, and try again in the next round. If everything goes fine,
          * remove it from the list, and never check it again.
          */
         rc = _checkAndCreateDictForCL( job, retry ) ;
         if ( SDB_OK == rc && retry )
         {
            tmpJobQue.push_back( job ) ;
         }

         cb->incEventCount() ;
      }

      PD_TRACE_EXITRC( SDB__RTN_DICTCREATORJOB_DOIT, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTDICTCREATORJOB, "startDictCreatorJob" )
   INT32 startDictCreatorJob ( EDUID *pEDUID, UINT32 scanInterval )
   {
      INT32 rc = SDB_OK ;
      rtnDictCreatorJob *pJob = NULL;
      PD_TRACE_ENTRY ( SDB_STARTDICTCREATORJOB ) ;

      pJob = SDB_OSS_NEW rtnDictCreatorJob( scanInterval ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to allocate memory for dictionary creator" ) ;
      }

      rc = rtnGetJobMgr()->startJob ( pJob, RTN_JOB_MUTEX_RET, pEDUID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to start dictionary creator job, rc = %d", rc ) ;
   done :
      PD_TRACE_EXITRC ( SDB_STARTDICTCREATORJOB, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTN_DICTCREATORJOB__CONDITIONMATCH, "_rtnDictCreatorJob::_conditionMatch" )
   BOOLEAN _rtnDictCreatorJob::_conditionMatch( dmsStorageUnit *su,
                                                dmsMBContext *context,
                                                UINT16 mbID )
   {
      PD_TRACE_ENTRY( SDB__RTN_DICTCREATORJOB__CONDITIONMATCH ) ;
      const dmsMBStatInfo *mbStatInfo = NULL ;
      BOOLEAN rc = FALSE ;

      if ( DMS_INVALID_EXTENT == context->mb()->_firstExtentID ||
           DMS_INVALID_EXTENT == context->mb()->_lastExtentID )
      {
         goto done ;
      }
      else
      {
         mbStatInfo = su->data()->getMBStatInfo( mbID ) ;
         SDB_ASSERT( mbStatInfo, "mbStatInfo should never be null" ) ;
         rc = (mbStatInfo->_totalRecords >= RTN_DICT_CREATE_REC_NUM_THRESHOLD &&
               mbStatInfo->_totalOrgDataLen >= RTN_DICT_CREATE_REC_DATA_SIZE) ;
      }

   done:
      PD_TRACE_EXIT( SDB__RTN_DICTCREATORJOB__CONDITIONMATCH ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTN_DICTCREATORJOB__CREATEDICT, "_rtnDictCreatorJob::_createDict" )
   INT32 _rtnDictCreatorJob::_createDict( dmsStorageDataCommon *sd,
                                          dmsMBContext *context,
                                          utilLZWDictCreator* creator )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTN_DICTCREATORJOB__CREATEDICT ) ;
      _mthRecordGenerator generator ;
      dmsRecordID recordID ;
      ossValuePtr recordDataPtr = 0 ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      UINT64 fetchNum = 0 ;
      UINT64 fetchSize = 0 ;
      BOOLEAN dictFull = FALSE ;

      SDB_ASSERT( sd && context && creator, "Invalid argument value" ) ;

      dmsTBScanner tbScanner( sd, context, NULL ) ;
      /*
       * The loop will end either all records have been fetched, or the
       * dictionary is full.
       */
      while ( SDB_OK == ( rc = tbScanner.advance( recordID, generator,
                                                  cb ) ) )
      {
         try
         {
            generator.getDataPtr( recordDataPtr ) ;
            BSONObj bs( (const CHAR*)recordDataPtr ) ;
            creator->build( bs.objdata(), bs.objsize(), dictFull ) ;
            if ( dictFull )
            {
               break ;
            }
            ++fetchNum ;
            fetchSize += bs.objsize() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      // The table scanner will release the mb latch when swithing to next
      // extent. During that time, the data or event the collection may change.
      // So after the scanner quit, we should check again.
      if ( SDB_DMS_EOC == rc )
      {
         // If break the loop because of data end hit, the dictionary is not
         // full. If not data is fetched, need to retry later. Otherwise, the
         // dictionary is OK.
         if ( fetchNum < RTN_DICT_CREATE_REC_NUM_THRESHOLD ||
              fetchSize < RTN_DICT_CREATE_REC_DATA_SIZE )
         {
            goto error ;
         }
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Fetching data and creating the dictionary "
                          "failed: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTN_DICTCREATORJOB__CREATEDICT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTN_DICTCREATORJOB__TRANSFERDICT, "_rtnDictCreatorJob::_transferDict" )
   INT32 _rtnDictCreatorJob::_transferDict( dmsStorageDataCommon *sd,
                                            dmsMBContext *context,
                                            CHAR *dictStream,
                                            UINT32 dictSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTN_DICTCREATORJOB__TRANSFERDICT ) ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      SDB_ASSERT( FALSE == context->isMBLock(),
                  "mb should not have been locked" ) ;

      ossSnprintf( fullName, sizeof( fullName ), "%s.%s",
                   sd->getSuName(), context->mb()->_collectionName ) ;
      rc = rtnLoadCollectionDict( fullName, dictStream, dictSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Load compression dictionary for collection[%s]"
                   " failed: %d", context->mb()->_collectionName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTN_DICTCREATORJOB__TRANSFERDICT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTN_DICTCREATORJOB__CHECKANDCREATEDICTFORCL, "_rtnDictCreatorJob::_checkAndCreateDictForCL" )
   INT32 _rtnDictCreatorJob::_checkAndCreateDictForCL( const dmsDictJob &job,
                                                       BOOLEAN &retry )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTN_DICTCREATORJOB__CHECKANDCREATEDICTFORCL ) ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      pmdKRCB *krCB = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krCB->getDMSCB() ;
      UINT32 dictBufLen = UTIL_MAX_DICT_TOTAL_SIZE ;
      CHAR *dictBuf = NULL ;
      BOOLEAN writable = FALSE ;
      pmdEDUCB *cb = pmdGetKRCB()->getEDUMgr()->getEDU() ;
      ossTimestamp begin ;
      ossTimestamp end ;
      utilLZWDictCreator creator ;

      retry = FALSE ;

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      // As no lock is taken after pushing the dictionary creation job into the
      // queue, the original collection, or even the collection space may have
      // been dropped. So need to check that before creating the dictionary.
      su = dmsCB->suLock( job._suID ) ;
      if ( ( NULL == su ) || ( su->LogicalCSID() != job._suLID ) )
      {
         PD_LOG( PDDEBUG, "Original collection space with logical ID %u dose "
                          "not exist any more. Skip the task", job._suLID ) ;
         goto done ;
      }

      // Lock the collection, and check if its data match the condition. If yes,
      // start to create the dictionary.
      rc = su->data()->getMBContext( &mbContext, job._clID,
                                     job._clLID, DMS_INVALID_CLID, SHARED ) ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_TRUNCATED == rc )
         {
            PD_LOG( PDINFO, "Original collection with logical ID %u dose not "
                            "exist any more. Skip the task", job._clID ) ;
            goto done ;
         }
         PD_LOG( PDERROR, "Get mb context failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT != mbContext->mb()->_dictExtentID ||
           UTIL_COMPRESSOR_LZW != mbContext->mb()->_compressorType )
      {
         goto done ;
      }

      if ( !_conditionMatch( su, mbContext, job._clID ) )
      {
         mbContext->mbUnlock() ;
         retry = TRUE ;
         goto done ;
      }

      // As the preparation may take a while, release the mb latch first.
      mbContext->pause() ;
      rc = creator.prepare() ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to prepare dictionary creator, rc: %d", rc ) ;
      rc = mbContext->resume() ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_TRUNCATED == rc )
         {
            PD_LOG( PDINFO, "Original collection with logical ID %u dose not "
                            "exist any more. Skip the task", job._clID ) ;
            goto done ;
         }
         PD_LOG( PDERROR, "Get mb context failed[%d]", rc ) ;
         goto error ;
      }

      // Double check of the condition after resuming the mb latch.
      if ( !_conditionMatch( su, mbContext, job._clID ) )
      {
         mbContext->mbUnlock() ;
         retry = TRUE ;
         goto done ;
      }

      ossGetCurrentTime( begin ) ;
      rc = _createDict( su->data(), mbContext, &creator ) ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_TRUNCATED == rc )
         {
            PD_LOG( PDINFO, "Original collection with logical ID %u dose not "
                            "exist any more. Skip the task", job._clID ) ;
            goto done ;
         }
         else if ( SDB_DMS_EOC != rc )
         {
            // Data in the collection is not enough. Should not print any error.
            PD_LOG( PDERROR, "Failed to create dictionary, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( mbContext->isMBLock() )
      {
         mbContext->mbUnlock() ;
      }

      dictBuf = (CHAR*)SDB_OSS_MALLOC( dictBufLen ) ;
      PD_CHECK( dictBuf, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for dictionary, rc: %d", rc ) ;

      rc = creator.finalize( dictBuf, dictBufLen ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to finalize dictionary, rc: %d", rc ) ;

      rc = _transferDict( su->data(), mbContext, dictBuf, dictBufLen ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to pass dictionary to dms, rc: %d", rc ) ;
      ossGetCurrentTime( end ) ;

      PD_LOG( PDEVENT, "Compression dictionary created succesfully for "
              "collection[%s.%s]. Time: %llums",
              su->CSName(), mbContext->mb()->_collectionName,
              end.time * 1000 + end.microtm / 1000 -
              (begin.time * 1000 + begin.microtm / 1000) ) ;

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }

      if ( su )
      {
         dmsCB->suUnlock( job._suID ) ;
      }

      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }

      if ( dictBuf )
      {
         SDB_OSS_FREE( dictBuf ) ;
      }

      PD_TRACE_EXITRC( SDB__RTN_DICTCREATORJOB__CHECKANDCREATEDICTFORCL, rc ) ;
      return rc ;
   error:
      // For other errors, let's try again later.
      if ( SDB_DMS_CS_NOTEXIST != rc && SDB_DMS_NOTEXIST != rc &&
           SDB_DMS_TRUNCATED != rc )
      {
         PD_LOG( PDWARNING, "Create compression dictionary failed[%d]. "
                            "Will try again later", rc ) ;
         rc = SDB_OK ;
         retry = TRUE ;
      }
      goto done ;
   }
}

