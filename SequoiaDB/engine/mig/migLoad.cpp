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

   Source File Name = migLoad.cpp

   Descriptive Name =

   When/how to use: load json file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2013  JW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pd.hpp"
#include "pmd.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "migLoad.hpp"
#include "pdTrace.hpp"
#include "migTrace.hpp"
#include "pmdEDUMgr.hpp"
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__SENDMSG,"migMaster::sendMsgToClient" )
   INT32 migMaster::sendMsgToClient ( const CHAR *format, ... )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__SENDMSG );
      SDB_ASSERT ( _sock, "_sock is NULL" ) ;
      va_list ap;
      pmdEDUCB *eduCB = pmdGetKRCB()->getEDUMgr()->getEDU() ;
      CHAR buffer[ PD_LOG_STRINGMAX ] ;
      boost::unique_lock<boost::mutex> lock ( _mutex ) ;

      ossMemset ( buffer, 0, PD_LOG_STRINGMAX ) ;
      va_start(ap, format) ;
      vsnprintf(buffer, PD_LOG_STRINGMAX, format, ap) ;
      va_end(ap) ;

      if ( _sendMsg )
      {
         ossMemset ( _sendMsg, 0, _sendSize ) ;
      }

      rc = msgBuildMsgMsg ( &_sendMsg, &_sendSize,
                            0, buffer ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to msgBuildMsgMsg" ) ;
         goto error ;
      }

      rc = pmdSend ( _sendMsg, *(INT32 *)_sendMsg,
                     _sock, eduCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send buff, rc=%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__SENDMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__GETBLOCK,"migMaster::getBlockFromPointer" )
   INT32 migMaster::getBlockFromPointer(UINT32 startOffset, UINT32 size,
                                        UINT32 &startBlock, UINT32 &endBlock)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__GETBLOCK );
      if ( startOffset + size > _pParameters->bufferSize )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed json" ) ;
         goto error ;
      }
      startBlock = startOffset / _blockSize ;
      endBlock = ( startOffset + size - 1 ) / _blockSize ;
      if ( endBlock < startBlock )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed json" ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__GETBLOCK, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__POPFROMQUEUE, "migMaster::popFromQueue" )
   void migMaster::popFromQueue( pmdEDUCB *eduCB,
                                 UINT32 &offset, UINT32 &size,
                                 UINT32 &line,   UINT32 &column )
   {
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__POPFROMQUEUE );
      BOOLEAN isPop = FALSE ;
      _dataQueue data ;
      do
      {
         isPop = _queue.timed_wait_and_pop ( data, 100 ) ;
      }while ( !isPop &&
               !eduCB->isInterrupted() &&
               !eduCB->isDisconnected() &&
               !eduCB->isForced() &&
               _sock->isConnected() ) ;
      if ( isPop )
      {
         offset = data.offset ;
         size   = data.size ;
         line   = data.line ;
         column = data.column ;
      }
      else
      {
         offset = 0 ;
         size   = 0 ;
         line   = 0 ;
         column = 0 ;
      }
      PD_TRACE_EXIT ( SDB__MIGLOADJSONPS__POPFROMQUEUE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__PUSHTOQUEUE, "migMaster::pushToQueue" )
   void migMaster::pushToQueue( UINT32 offset, UINT32 size,
                                UINT32 line,   UINT32 column )
   {
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__PUSHTOQUEUE ) ;
      _dataQueue data ;
      data.offset = offset ;
      data.size   = size ;
      data.line   = line ;
      data.column = column ;
      _queue.push ( data ) ;
      PD_TRACE_EXIT ( SDB__MIGLOADJSONPS__PUSHTOQUEUE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__INITIALIZE, "migMaster::initialize" )
   INT32 migMaster::initialize ( setParameters *pParameters )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__INITIALIZE );
      _utilParserParamet parserPara ;
      UINT32  startOffset = 0 ;
      UINT32  fieldsSize  = 0 ;

      _pParameters = pParameters ;
      _sock = _pParameters->clientSock ;
      _fileType = _pParameters->fileType ;
      _blockSize = _pParameters->bufferSize/_pParameters->bucketNum ;
      _exitSignal = FALSE ;

      if ( MIG_PARSER_JSON == _pParameters->fileType )
      {
         _parser = SDB_OSS_NEW _utilJSONParser() ;
      }
      else if ( MIG_PARSER_CSV == _pParameters->fileType )
      {
         _parser = SDB_OSS_NEW _utilCSVParser() ;
      }
      else
      {
         rc = SDB_MIG_UNKNOW_FILE_TYPE ;
         PD_LOG ( PDERROR, "Unknow file type" ) ;
         goto error ;
      }
      PD_CHECK ( _parser, SDB_OOM, error, PDERROR,
                 "Failed to new _parser" ) ;
      _ppBucket = (_bucket**)SDB_OSS_MALLOC ( sizeof(_bucket*) *
                                              _pParameters->bucketNum ) ;
      PD_CHECK ( _ppBucket, SDB_OOM, error, PDERROR,
                 "Failed to allocate bucket pointer array" ) ;
      ossMemset ( _ppBucket, 0, sizeof(_bucket*) * _pParameters->bucketNum ) ;
      for ( UINT32 i = 0; i < _pParameters->bucketNum; ++i )
      {
         _ppBucket[i] = SDB_OSS_NEW _bucket() ;
         PD_CHECK ( _ppBucket[i], SDB_OOM, error, PDERROR,
                    "Failed to allocate bucket pointer array" ) ;
      }

      _parser->setDel ( _pParameters->delCFR[0],
                        _pParameters->delCFR[1],
                        _pParameters->delCFR[2] ) ;

      parserPara.fileName = _pParameters->pFileName ;
      parserPara.bufferSize = _pParameters->bufferSize ;
      parserPara.blockNum = _pParameters->bucketNum ;

      rc = _parser->initialize( &parserPara ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init utilParseJSONs, rc = %d", rc ) ;
         if ( SDB_IO == rc || SDB_FNE == rc )
         {
            sendMsgToClient ( "Failed to open file %s, rc=%d",
                              _pParameters->pFileName, rc ) ;
         }
         goto error ;
      }
      _buffer = _parser->getBuffer() ;

      _delChar   = _pParameters->delCFR[0] ;
      _delField  = _pParameters->delCFR[1] ;
      _delRecord = _pParameters->delCFR[2] ;
      _autoAddField   = TRUE ;
      _autoCompletion = FALSE ;
      _isHeaderline   = _pParameters->headerline ;
      

      if ( _isHeaderline )
      {
         rc = _parser->getNextRecord ( startOffset, fieldsSize ) ;
         if ( rc )
         {
            if ( rc == SDB_EOF )
            {
               if ( 0 == fieldsSize )
               {
                  goto done ;
               }
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to _parser getNextRecord, rc=%d", rc ) ;
               goto error ;
            }
         }
      }
      
      if ( _pParameters->pFieldArray )
      {
         _pFields = _pParameters->pFieldArray ;
         _fieldsSize = ossStrlen( _pFields ) ;
      }
      else
      {
         _pFields = _buffer + startOffset ;
         _fieldsSize = fieldsSize ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__INITIALIZE, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__STOPWAIT, "migMaster::_stopAndWaitWorker" )
   INT32 migMaster::_stopAndWaitWorker ( pmdEDUCB *eduCB,
                                         UINT32 &success,
                                         UINT32 &failure )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__STOPWAIT );
      _workerReturn *workerRe = NULL ;
      pmdEDUEvent    event ;
      for ( UINT32 i = 0; i < _pParameters->workerNum; ++i )
      {
         pushToQueue ( 0, 0, 0, 0 ) ;
      }

      for ( UINT32 i = 0; i < _pParameters->workerNum; )
      {
         if ( eduCB->waitEvent ( event, 100 ) )
         {
            workerRe = (_workerReturn *)event._Data ;
            success += workerRe->success ;
            failure += workerRe->failure ;
            ++i ;
            pmdEduEventRelease( event, eduCB ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__STOPWAIT, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__CHECKWORKER, "migMaster::_checkErrAndRollback" )
   INT32 migMaster::_checkErrAndRollback  ( pmdEDUCB *eduCB,
                                            dmsStorageLoadOp* loadOp,
                                            dmsMBContext *mbContext,
                                            UINT32 &success,
                                            UINT32 &failure )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__CHECKWORKER );
      pmdEDUEvent    event ;

      // if we receive any post, that means worker got something wrong and we
      // should handle it respectively
      /*isGetEven = eduCB->waitEvent ( event, 0 ) ;
      if ( isGetEven )
      {
         // if we receive anything, let's count the success and failure and
         // goon, note it means something wrong happened at worker
         workerRe = (_workerReturn *)event._Data ;
         success += workerRe->success ;
         failure += workerRe->failure ;
         --_workerNum ;
      }*/
      // if something wrong happened at worker, or the connection is gone,
      // let's rollback
      //if ( isGetEven || !_sock->isConnected() )
      if ( !_exitSignal )
      {
         _exitSignal = !_sock->isConnected() ;
         if ( !_exitSignal )
         {
            _exitSignal = eduCB->isForced() ;
         }
      }
      if ( _exitSignal )
      {
         // print the error in log
         PD_LOG ( PDERROR, "rollback all data" ) ;
         // send error to user side, note we don't need to check rc since we
         // can't do anything if it's not success, anyway
         sendMsgToClient ( "Error: rollback all data" ) ;

         rc = _stopAndWaitWorker ( eduCB, success, failure ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to call _stopAndWaitWorker, rc=%d", rc ) ;

         //roll back
         rc = loadOp->loadRollbackPhase ( mbContext ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to rollback, rc=%d", rc ) ;
            sendMsgToClient ( "Error: Failed to rollback, rc = %d",
                              rc ) ;
            goto error ;
         }
         rc = SDB_LOAD_ROLLBACK ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__CHECKWORKER, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGLOADJSONPS__RUN, "migMaster::run" )
   INT32 migMaster::run( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGLOADJSONPS__RUN );
      UINT32      startOffset   = 0 ;
      UINT32      size          = 0 ;
      UINT32      startBlock    = 0 ;
      UINT32      endBlock      = 0 ;
      pmdKRCB    *krcb          = pmdGetKRCB () ;
      pmdEDUMgr  *eduMgr        = krcb->getEDUMgr () ;
      SDB_DMSCB  *dmsCB         = krcb->getDMSCB () ;
      EDUID       agentEDU      = PMD_INVALID_EDUID ;
      BOOLEAN     writable      = FALSE ;
      BOOLEAN     noClearFlag   = FALSE ;
      dmsMBContext *mbContext   = NULL ;
      UINT32      line          = 0 ;
      UINT32      column        = 0 ;
      UINT32      success       = 0 ;
      UINT32      failure       = 0 ;
      UINT16      clFlag        = 0 ;
      dmsStorageUnitID  suID     = DMS_INVALID_CS ;
      dmsStorageUnit   *su       = NULL ;
      initWorker  dataWorker ;
      pmdEDUEvent event ;
      dmsStorageLoadOp dmsLoadExtent ;

      sendMsgToClient ( "Load start" ) ;

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
         goto error;
      }
      writable = TRUE;

      rc = rtnCollectionSpaceLock ( _pParameters->pCollectionSpaceName,
                                    dmsCB,
                                    FALSE,
                                    &su,
                                    suID ) ;
      if ( rc )
      {
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            sendMsgToClient ( "Error: collection space not exist" ) ;
         }
         PD_LOG ( PDERROR, "Failed to lock collection space, rc=%d", rc ) ;
         goto error ;
      }

      dmsLoadExtent.init ( su ) ;

      rc = su->data()->getMBContext( &mbContext,
                                     _pParameters->pCollectionName,
                                     EXCLUSIVE ) ;
      if ( rc )
      { 
         if ( SDB_DMS_NOTEXIST == rc )
         {
            sendMsgToClient ( "Error: collection not exist" ) ;
         }
         PD_LOG ( PDERROR, "Failed to lock collection, rc=%d", rc ) ;
         goto error ;
      }

      clFlag = mbContext->mb()->_flag ;

      if ( DMS_IS_MB_DROPPED( clFlag ) )
      {
         PD_LOG( PDERROR, "Collection is droped" ) ;
         rc = SDB_COLLECTION_LOAD ;
         sendMsgToClient ( "Collection is droped" ) ;
         goto error ;
      }
      else if ( DMS_IS_MB_LOAD ( clFlag ) )
      {
         PD_LOG( PDERROR, "Collection is loading" ) ;
         rc = SDB_COLLECTION_LOAD ;
         sendMsgToClient ( "Collection is loading" ) ;
         // we set noClearFlag to true, so that we'll convert the collection
         // flag to NORMAL in done
         noClearFlag = TRUE ;
         goto error ;
      }

      dmsLoadExtent.setFlagLoad ( mbContext->mb() ) ;
      dmsLoadExtent.setFlagLoadLoad ( mbContext->mb() ) ;

      // unlock
      mbContext->mbUnlock() ;

      dataWorker.pMaster = this ;
      dataWorker.masterEDUID = cb->getID() ;
      dataWorker.pSu = su ;
      dataWorker.clLID = mbContext->clLID() ;
      dataWorker.collectionID = mbContext->mbID() ;

      for ( UINT32 i = 0; i < _pParameters->workerNum ; ++i )
      {
         eduMgr->startEDU ( EDU_TYPE_LOADWORKER, &dataWorker, &agentEDU ) ;
      }

      while ( TRUE )
      {
         rc = _checkErrAndRollback ( cb, &dmsLoadExtent,
                                     mbContext, success, failure ) ;
         if ( SDB_TIMEOUT != rc &&
              rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to call _checkErrAndRollback, rc=%d", rc ) ;
            goto error ;
         }

         // fetch one record
         rc = _parser->getNextRecord ( startOffset, size,
                                       &line, &column, _ppBucket ) ;
         if ( rc )
         {
            // special handle for end of file
            if ( rc == SDB_EOF )
            {
               // when we hit end of file, let's push 0 to all worker threads,
               // with num of workers ( so each worker will dispatch one 0, and
               // exit )
               rc = _stopAndWaitWorker ( cb, success, failure ) ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Failed to call _stopAndWaitWorker, rc=%d", rc ) ;
               break ;
            }
            sendMsgToClient ( "Error: Parse Json error in line: %u,"
                              " column: %u", line, column ) ;
            PD_LOG ( PDERROR, "Failed to parseJSONs getNextRecord,rc=%d", rc ) ;
            goto error1 ;
         }
         // calculate the blocks to be locked, based on the length of our record
         rc = getBlockFromPointer ( startOffset, size,
                                    startBlock, endBlock ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get block from pointer, rc=%d", rc ) ;
            goto error1 ;
         }
         // lock them
         for ( UINT32 i = startBlock; i <= endBlock; ++i )
         {
            _ppBucket[i]->inc() ;
         }
         // push the record to queue
         pushToQueue ( startOffset, size, line, column ) ;
      } // while ( !cb->isForced() )

      // when all workers are finish, let's start build phase to rebuild all
      // indexes
      sendMsgToClient ( "build index" ) ;
      rc = dmsLoadExtent.loadBuildPhase ( mbContext, cb,
                                          _pParameters->isAsynchronous,
                                          this, &success, &failure ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to load data, rc=%d", rc ) ;
         goto error ;
      }

   done:
      // we only lock and clear flag if we switched to load
      if ( su && mbContext && !noClearFlag )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         // we should log failure information
         if ( SDB_OK == rc )
         {
            if ( dmsLoadExtent.isFlagLoadLoad ( mbContext->mb() ) )
            {
               dmsLoadExtent.clearFlagLoadLoad ( mbContext->mb() ) ;
            }
            if ( dmsLoadExtent.isFlagLoadBuild ( mbContext->mb() ) )
            {
               dmsLoadExtent.clearFlagLoadBuild ( mbContext->mb() ) ;
            }
            if ( dmsLoadExtent.isFlagLoad ( mbContext->mb() ) )
            {
               dmsLoadExtent.clearFlagLoad ( mbContext->mb() ) ;
            }
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to lock collection, rc=%d", rc ) ;
         }
      }

      // send the success message to client
      sendMsgToClient ( "success json: %u, failure json: %u",
                        success, failure ) ;
      sendMsgToClient ( "Load end" ) ;

      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      // count down
      if ( writable )
      {
         dmsCB->writeDown( cb );
      }
      PD_TRACE_EXITRC ( SDB__MIGLOADJSONPS__RUN, rc );
      return rc ;
   error:
      goto done ;
   error1:
      _stopAndWaitWorker ( cb, success, failure ) ;
      sendMsgToClient ( "Error: rollback all data" ) ;
      failure += success ;
      success = 0 ;
      rc = dmsLoadExtent.loadRollbackPhase ( mbContext ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to rollback, rc=%d", rc ) ;
         sendMsgToClient ( "Error: Failed to rollback, rc = %d",
                           rc ) ;
         goto error ;
      }
      goto done ;
   }

   void migMaster::bucketDec ( INT32 blockID )
   {
      _ppBucket[ blockID ]->dec() ;
   }

   CHAR *migMaster::getBuffer()
   {
      return _buffer ;
   }

   migMaster::migMaster() : _buffer(NULL),
                            _pParameters(NULL),
                            _sendMsg(NULL),
                            _sendSize(0),
                            _ppBucket(NULL),
                            _blockSize(0)
   {
       _parser = NULL ;
   }

   migMaster::~migMaster()
   {
      if ( _ppBucket )
      {
         for ( UINT32 i = 0; i < _pParameters->bucketNum; ++i )
         {
            SAFE_OSS_DELETE ( _ppBucket[i] ) ;
         }
      }
      SAFE_OSS_FREE ( _ppBucket ) ;
      SAFE_OSS_DELETE ( _parser ) ;
      SAFE_OSS_FREE ( _sendMsg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGWORKER__GETBSON, "migWorker::_getBsonFromQueue" )
   INT32 migWorker::_getBsonFromQueue( pmdEDUCB *eduCB, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGWORKER__GETBSON );
      INT32 tempRc = SDB_OK ;
      UINT32 offset = 0 ;
      UINT32 size   = 0 ;
      UINT32 line   = 0 ;
      UINT32 column = 0 ;
      UINT32 startBlock = 0 ;
      UINT32 endBlock   = 0 ;

      _master->popFromQueue ( eduCB,
                              offset, size,
                              line, column ) ;
      if ( 0 == offset && 0 == size &&
           0 == line && 0 == column )
      {
         rc = SDB_MIG_END_OF_QUEUE ;
         goto done ;
      }

      if ( MIG_PARSER_JSON == _master->_fileType )
      {
         tempRc = fromjson ( _master->getBuffer() + offset, obj ) ;
      }
      else if ( MIG_PARSER_CSV == _master->_fileType )
      {
         rc = _csvParser.csv2bson( _master->getBuffer() + offset,
                                   size, &obj ) ;
         if ( rc )
         {
            rc = SDB_UTIL_PARSE_JSON_INVALID ;
            PD_LOG ( PDERROR, "Failed to convert Bson, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_MIG_UNKNOW_FILE_TYPE ;
         PD_LOG ( PDERROR, "unknow file type" ) ;
         goto error ;
      }
      if ( tempRc )
      {
         //PD_LOG ( PDERROR, "Failed to json convert bson, json: %s , rc=%d",
         //         _pJsonBuffer, tempRc ) ;
         _master->sendMsgToClient ( "Error: error "
                                    "in json format, line %u, column %u",
                                    line, column ) ;
      }
      rc = _master->getBlockFromPointer ( offset, size,
                                          startBlock, endBlock ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get block from pointer, rc=%d", rc ) ;
         goto error ;
      }
      for ( UINT32 i = startBlock; i <= endBlock; ++i )
      {
         _master->bucketDec( i ) ;
      }
   done:
      if ( tempRc )
      {
         rc = tempRc ;
      }
      PD_TRACE_EXITRC ( SDB__MIGWORKER__GETBSON, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MIGWORKER__IMPORT, "migWorker::importData" )
   INT32 migWorker::importData ( EDUID masterEDUID,
                                 dmsStorageUnit *su,
                                 UINT16 collectionID,
                                 UINT32 clLID,
                                 BOOLEAN isAsynchr,
                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MIGWORKER__IMPORT );
      pmdKRCB     *krcb           = pmdGetKRCB () ;
      pmdEDUMgr   *eduMgr         = krcb->getEDUMgr() ;
      BOOLEAN      isLast         = FALSE ;
      BOOLEAN      isFirst        = TRUE ;
      BSONObj      record ;
      dmsStorageLoadOp dmsLoadExtent ;
      _workerReturn *workRe = NULL ;
      _dmsMBContext *mbContext = NULL ;

      SDB_ASSERT ( su, "su is NULL" ) ;

      dmsLoadExtent.init ( su ) ;
      workRe = SDB_OSS_NEW _workerReturn() ;
      if ( !workRe )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "memory error" ) ;
         goto error ;
      }
      rc = su->data()->getMBContext( &mbContext, collectionID, clLID,
                                     DMS_INVALID_CLID, -1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;
         goto error ;
      }
      workRe->success = 0 ;
      workRe->failure = 0 ;
      workRe->rc      = 0 ;

      rc = _csvParser.init( _master->_autoAddField,
                            _master->_autoCompletion,
                            _master->_isHeaderline,
                            _master->_delChar,
                            _master->_delField,
                            _master->_delRecord ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to csv parser initialize, rc=%d", rc ) ;
         goto error ;
      }

      rc = _csvParser.parseHeader( _master->_pFields, _master->_fieldsSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to parse csv header, rc=%d", rc ) ;
         goto error ;
      }

      while ( !_master->_exitSignal &&
              !cb->isInterrupted() &&
              !cb->isDisconnected() )
      {
         rc = _getBsonFromQueue ( cb, record ) ;
         if ( rc )
         {
            if ( SDB_MIG_END_OF_QUEUE == rc )
            {
               isLast = TRUE ;
               rc = SDB_OK ;
               if ( isFirst )
               {
                  goto done ;
               }
            }
            else if ( SDB_INVALIDARG == rc )
            {
               ++workRe->failure ;
               continue ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get bson from queue, rc=%d", rc ) ;
               goto error ;
            }
         }
         rc = dmsLoadExtent.pushToTempDataBlock( mbContext,
                                                 cb,
                                                 record,
                                                 isLast,
                                                 isAsynchr ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to import to block, rc=%d", rc ) ;
            goto error ;
         }
         if ( isLast )
         {
            goto done ;
         }
         ++workRe->success ;
         isFirst = FALSE ;
      }
      goto error ;
   done:
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      workRe->rc = rc ;
      rc = eduMgr->postEDUPost ( masterEDUID, PMD_EDU_EVENT_MSG,
                                 PMD_EDU_MEM_ALLOC, workRe ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to postEDUPost, rc=%d", rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__MIGWORKER__IMPORT, rc );
      return rc ;
   error:
      _master->_exitSignal = TRUE ;
      if ( workRe )
      {
         workRe->failure += workRe->success ;
         workRe->success = 0 ;
      }
      goto done ;
   }

   migWorker::migWorker ( migMaster *master ) : _master(NULL)
   {
      SDB_ASSERT ( master, "master is NULL" ) ;
      _master = master ;
   }

   migWorker::~migWorker()
   {
   }
}
