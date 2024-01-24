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

   Source File Name = rtnExtContext.cpp

   Descriptive Name = External data process context

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/08/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnExtContext.hpp"
#include "rtnTrace.hpp"
#include "rtnCB.hpp"
#include "ossMem.hpp"

#define RTN_EXT_CACHE_CTX_MAX                (1000)

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE, "_rtnExtContextBase::_rtnExtContextBase" )
   _rtnExtContextBase::_rtnExtContextBase( DMS_EXTOPR_TYPE type )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE ) ;
      _type = type ;
      _id = 0 ;
      _stat = EXT_CTX_STAT_NORMAL ;
      _processorMgr = NULL ;
      _lockType = -1 ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE ) ;
   }

   _rtnExtContextBase::~_rtnExtContextBase()
   {
      _cleanup() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_RESET, "_rtnExtContextBase::reset" )
   void _rtnExtContextBase::reset( DMS_EXTOPR_TYPE type )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_RESET ) ;
      _type = type ;
      _id = 0 ;
      _stat = EXT_CTX_STAT_NORMAL ;
      _processorMgr = NULL ;
      _lockType = -1 ;
      _cleanup() ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_DONE, "_rtnExtContextBase::done" )
   INT32 _rtnExtContextBase::done( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_DONE ) ;

      if ( EXT_CTX_STAT_NORMAL == _stat )
      {
         rc = _onDone( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Operation _onDone failed[ %d ]", rc ) ;
      }
      else if ( EXT_CTX_STAT_ABORTING == _stat )
      {
         rc = _onAbort( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Operation _onAbort failed[ %d ]", rc ) ;
      }

   done:
      _cleanup() ;
      PD_TRACE_EXITRC( SDB__RTNEXTCONTEXTBASE_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_ABORT, "_rtnExtContextBase::abort" )
   INT32 _rtnExtContextBase::abort( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_ABORT ) ;

      rc = _onAbort( cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Operation _onAbort failed[ %d ]", rc ) ;

   done:
      _cleanup() ;
      PD_TRACE_EXITRC( SDB__RTNEXTCONTEXTBASE_ABORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE__CLEANUP, "_rtnExtContextBase::_cleanup" )
   void _rtnExtContextBase::_cleanup()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE__CLEANUP ) ;
      if ( _processorLocked() )
      {
         _processorMgr->unlockProcessors( _processors, _lockType ) ;
         _lockType = -1 ;
      }
      _processors.clear();
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE__CLEANUP ) ;
   }

   void _rtnExtContextBase::_appendProcessor( rtnExtDataProcessor *processor )
   {
      _processors.push_back( processor ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS, "_rtnExtContextBase::_appendProcessors" )
   void _rtnExtContextBase::_appendProcessors( const vector< rtnExtDataProcessor * >& processorVec )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS ) ;
      _processors.insert( _processors.end(), processorVec.begin(),
                          processorVec.end() ) ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS ) ;
   }

   _rtnExtCrtIdxCtx::_rtnExtCrtIdxCtx()
   : _rtnExtContextBase( DMS_EXTOPR_TYPE_CRTIDX ),
     _rebuildDone( FALSE )
   {
   }

   _rtnExtCrtIdxCtx::~_rtnExtCrtIdxCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCRTIDXCTX_OPEN, "_rtnExtCrtIdxCtx::open" )
   INT32 _rtnExtCrtIdxCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                 dmsMBContext *mbContext,
                                 const CHAR *csName,
                                 ixmIndexCB &indexCB,
                                 pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCRTIDXCTX_OPEN ) ;
      rtnExtDataProcessor *processor = NULL ;

      SDB_ASSERT( mbContext, "mb context is NULL" ) ;

      _processorMgr = processorMgr ;

      if ( !mbContext->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller should hold mb exclusive lock" ) ;
         goto error ;
      }

      {
         indexCB.setFlag( IXM_INDEX_FLAG_CREATING );
         rc = _processorMgr->createProcessor( csName,
                                              mbContext->mb()->_collectionName,
                                              indexCB.getName(),
                                              indexCB.getExtDataName(),
                                              indexCB.keyPattern(),
                                              TRUE, cb, dpscb, processor );
         PD_RC_CHECK( rc, PDERROR, "Create external processor failed[%d]",
                      rc ) ;

         _rebuildDone = TRUE ;

         indexCB.setFlag( IXM_INDEX_FLAG_NORMAL ) ;
         _appendProcessor( processor );
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTCRTIDXCTX_OPEN, rc ) ;
      return rc ;
   error:
      if ( processor )
      {
         _processorMgr->destroyProcessor( processor ) ;
         processor = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCRTIDXCTX__ONABORT, "_rtnExtCrtIdxCtx::_onAbort" )
   INT32 _rtnExtCrtIdxCtx::_onAbort( _pmdEDUCB * cb, SDB_DPSCB * dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCRTIDXCTX__ONABORT ) ;

      if ( _rebuildDone )
      {
         SDB_ASSERT( 1 == _processors.size(), "Processor is NULL" ) ;
         rc = _processors.front()->doDropP1( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Processor drop operation phase 1 "
                                   "failed[ %d ]", rc ) ;
         rc = _processors.front()->doDropP2( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Processor drop operation phase 2 "
                                   "failed[ %d ]", rc ) ;
      }

      if ( _processors.size() > 0 )
      {
         _processors.front()->setStat( RTN_EXT_PROCESSOR_DROPPING ) ;
         _processorMgr->destroyProcessors( _processors ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTCRTIDXCTX__ONABORT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtRebuildIdxCtx::_rtnExtRebuildIdxCtx()
   : _rtnExtContextBase( DMS_EXTOPR_TYPE_REBUILDIDX )
   {
   }

   _rtnExtRebuildIdxCtx::~_rtnExtRebuildIdxCtx()
   {
      if ( _processorLocked() )
      {
         _processorMgr->unlockProcessors( _processors, _lockType ) ;
         _lockType = -1 ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTREBUILDIDXCTX_OPEN, "_rtnExtRebuildIdxCtx::open" )
   INT32 _rtnExtRebuildIdxCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                     const CHAR *csName, const CHAR *clName,
                                     const CHAR *idxName, const CHAR *extName,
                                     const BSONObj &idxKeyDef, pmdEDUCB *cb,
                                     SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTREBUILDIDXCTX_OPEN ) ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;
      BOOLEAN newProcessor = FALSE ;

      // Index will be rebuilt in the following cases:
      // 1. One new index is created.
      // 2. In case of full sync.
      // 3. Rebuilding after crash.
      // If rebuild index is called when db is in normal status, it means one
      // new index is being created. In that case, processor and new capped
      // collection should be created.
      // In case of full sync, leave all the capped collections to sync by
      // themselves, because we want them to be exactly the same with the ones
      // on primary node. The processors will be deleted if the index is
      // dropped. So we need to add them again here.
      // In any other cases( rebuilding, for example ), the processors are added
      // during opening of storage. So no need to add.
      BOOLEAN create = ( SDB_DB_NORMAL == dbStatus
                         || SDB_DB_FULLSYNC == dbStatus ) ? TRUE : FALSE ;

      SDB_ASSERT( cb, "eduCB is NULL" ) ;

      _processorMgr = processorMgr ;

      rc = processorMgr->getProcessorByExtName( extName, EXCLUSIVE,
                                                processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor for index "
                                "failed[ %d ]", rc ) ;
      _processorMgr = processorMgr ;

      if ( !processor )
      {
         if ( create )
         {
            // The processor can only be used after the rebuild is done. So do
            // not activate the processor when create it.
            rc = processorMgr->createProcessor( csName, clName, idxName,
                                                extName, idxKeyDef, TRUE,
                                                cb, dpscb, processor ) ;
            PD_RC_CHECK( rc, PDERROR, "Create external processor failed[ %d ]",
                         rc ) ;
            newProcessor = TRUE ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get external data processor failed[ %d ]", rc ) ;
            goto error ;
         }
      }
      else
      {
         _lockType = EXCLUSIVE ;
         _appendProcessor( processor ) ;
         rc = processor->doRebuild( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Rebuild of index failed[ %d ]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTREBUILDIDXCTX_OPEN, rc ) ;
      return rc ;
   error:
      if ( newProcessor )
      {
         processor->setStat( RTN_EXT_PROCESSOR_DROPPING ) ;
         _processorMgr->destroyProcessor( processor ) ;
         _lockType = -1 ;
      }
      goto done ;
   }

   _rtnExtDataOprCtx::_rtnExtDataOprCtx( DMS_EXTOPR_TYPE type )
   : _rtnExtContextBase( type )
   {
   }

   _rtnExtDataOprCtx::~_rtnExtDataOprCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAOPRCTX_OPEN, "_rtnExtDataOprCtx::open" )
   INT32 _rtnExtDataOprCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                  const CHAR *extName, pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAOPRCTX_OPEN ) ;
      INT32 lockType = -1 ;

      rtnExtDataProcessor *processor = NULL ;
      SDB_ASSERT( processorMgr && extName, "Invalid argument" ) ;

      _processorMgr = processorMgr ;
      // Why add EXCLUSIVE lock here? To make sure of the right order of records
      // insertted into capped collection.
      // This lock cooperates with mb lock of original collection. It's taken
      // inside the protection of the mb lock, and released after the mb lock
      // released(after writting dps log). So we need this lock to ensure the
      // write order.
      lockType = cb->isDoRollback() ? -1 : EXCLUSIVE ;
      rc = processorMgr->getProcessorByExtName( extName, lockType, processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor failed[%d]", rc ) ;
      _lockType = lockType ;
      if ( !processor )
      {
         goto done ;
      }

      _appendProcessor( processor ) ;

      try
      {
         BSONObj record = getOprRecord( (void *)processor ) ;
         if ( record.isEmpty() )
         {
            // No operation data to insert into capped collection. The record
            // will be skipped.
            goto done ;
         }
         rc = processor->processDML( record, cb, cb->isDoRollback(), dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Process data operation failed[%d]", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAOPRCTX_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAOPRCTX__ONDONE, "_rtnExtDataOprCtx::_onDone" )
   INT32 _rtnExtDataOprCtx::_onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAOPRCTX__ONDONE ) ;
      for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end();
            ++itr )
      {
         rc = (*itr)->done( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Processor done operation failed[ %d ]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAOPRCTX__ONDONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDropCSCtx::~_rtnExtDropCSCtx()
   {
      // Cancle all pending operations.
      if ( _processorP1.size() > 0 )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = _processorP1.begin();
               itr != _processorP1.end(); ++itr )
         {
            (*itr)->doDropP1Cancel( _eduCB, _dpsCB ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCSCTX_OPEN, "_rtnExtDropCSCtx::open")
   INT32 _rtnExtDropCSCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                 const CHAR *csName, pmdEDUCB *cb,
                                 BOOLEAN removeFiles, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCSCTX_OPEN ) ;
      BOOLEAN metaLocked = FALSE ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      vector<rtnExtDataProcessor *> processors ;

      _processorMgr = processorMgr ;

   retry:
      // First try to lock the processor in EXCLUSIVE mode, to make sure that no
      // one else is currently using the processor. Then take the meta data lock
      // in exclusive mode, to block further request for any processor. Then
      // release the processor lock. In this way, we can be sure no one can use
      // the processor before we release the metadata lock.
      rc = processorMgr->getProcessorsByCS( csName, EXCLUSIVE, processors ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare processors failed[ %d ]", rc ) ;
      // May be there is no text indices in this cs any more.
      if ( 0 == processors.size() )
      {
         goto done ;
      }
      _lockType = EXCLUSIVE ;

      if ( !processorMgr->tryAquireMetaLock( EXCLUSIVE ) )
      {
         processorMgr->unlockProcessors( processors, EXCLUSIVE ) ;
         processors.clear() ;
         _lockType = -1 ;
         ossSleep( 50 ) ;
         goto retry ;
      }
      metaLocked = TRUE ;

      processorMgr->unlockProcessors( processors, EXCLUSIVE ) ;
      _lockType = -1 ;

      _removeFiles = removeFiles ;
      _eduCB = cb ;
      _dpsCB = dpscb ;

      if ( SDB_DB_FULLSYNC != dbStatus )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = processors.begin();
               itr != processors.end(); ++itr )
         {
            if ( _removeFiles )
            {
               rc = (*itr)->doDropP1( cb, dpscb ) ;
            }
            else
            {
               rc = (*itr)->doUnload( cb ) ;
            }
            if ( rc )
            {
               PD_LOG( PDERROR, "Processor drop operation failed[%d]", rc ) ;
               if ( SDB_DMS_CS_NOTEXIST != rc )
               {
                  goto error ;
               }
            }
            else
            {
               // Set the tatus of the processor to dropping to make it invisible to
               // other threads.
               (*itr)->setStat( RTN_EXT_PROCESSOR_DROPPING ) ;
               _processorP1.push_back( *itr ) ;
            }
         }
      }

   done:
      if ( metaLocked )
      {
         processorMgr->releaseMetaLock( EXCLUSIVE ) ;
      }

      if ( processors.size() > 0 )
      {
         _appendProcessors( processors ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDROPCSCTX_OPEN, rc ) ;
      return rc ;
   error:
      _onAbort( cb, dpscb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCSCTX__ONDONE, "_rtnExtDropCSCtx::_onDone")
   INT32 _rtnExtDropCSCtx::_onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCSCTX__ONDONE ) ;

      // For drop operation, the processors need to be removed.
      for ( EDP_VEC_ITR itr = _processorP1.begin(); itr != _processorP1.end();
            ++itr )
      {
         if ( _removeFiles )
         {
            rc = (*itr)->doDropP2( cb, dpscb ) ;
            if ( rc )
            {
               // Write error log, but continue.
               PD_LOG( PDERROR, "Do drop phase 2 failed[ %d ]", rc ) ;
            }
         }
      }

      // Take the meta lock and delete the processors.
      _processorMgr->destroyProcessors( _processors ) ;
      // _processors and _processorP1 are the same.
      _processorP1.clear() ;
      _processors.clear() ;
      _lockType = -1 ;

      PD_TRACE_EXITRC( SDB__RTNEXTDROPCSCTX__ONDONE, rc ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCSCTX__ONABORT, "_rtnExtDropCSCtx::_onAbort")
   INT32 _rtnExtDropCSCtx::_onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCSCTX__ONABORT ) ;
      for ( EDP_VEC_ITR itr = _processorP1.begin(); itr != _processorP1.end(); )
      {
         (*itr)->doDropP1Cancel( cb, dpscb ) ;
         (*itr)->setStat( RTN_EXT_PROCESSOR_NORMAL ) ;
         itr = _processorP1.erase( itr ) ;
      }

      PD_TRACE_EXIT( SDB__RTNEXTDROPCSCTX__ONABORT ) ;
      return SDB_OK ;
   }

   _rtnExtDropCLCtx::~_rtnExtDropCLCtx()
   {
      // Cancle all pending operations.
      if ( _processorP1.size() > 0 )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = _processorP1.begin();
               itr != _processorP1.end(); ++itr )
         {
            (*itr)->doDropP1Cancel( _eduCB, _dpsCB ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCLCTX_OPEN, "_rtnExtDropCLCtx::open")
   INT32 _rtnExtDropCLCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                 const CHAR *csName, const CHAR *clName,
                                 pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCLCTX_OPEN ) ;
      BOOLEAN metaLocked = FALSE ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      vector<rtnExtDataProcessor *> processors ;

      _processorMgr = processorMgr ;

   retry:
      // First try to lock the processor in EXCLUSIVE mode, to make sure that no
      // one else is currently using the processor. Then take the meta data lock
      // in exclusive mode, to block further request for any processor. Then
      // release the processor lock. In this way, we can be sure no one can use
      // the processor before we release the metadata lock.
      rc = processorMgr->getProcessorsByCL( csName, clName,
                                            EXCLUSIVE, processors ) ;
      PD_RC_CHECK( rc, PDERROR, "Get processors failed[ %d ]", rc ) ;
      if ( 0 == processors.size() )
      {
         goto done ;
      }
      _lockType = EXCLUSIVE ;

      if ( !processorMgr->tryAquireMetaLock( EXCLUSIVE ) )
      {
         processorMgr->unlockProcessors( processors, EXCLUSIVE ) ;
         processors.clear() ;
         _lockType = -1 ;
         ossSleep( 50 ) ;
         goto retry ;
      }
      metaLocked = TRUE ;

      processorMgr->unlockProcessors( processors, EXCLUSIVE ) ;
      _lockType = -1 ;

      _eduCB = cb ;
      _dpsCB = dpscb ;

      if ( SDB_DB_FULLSYNC != dbStatus )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = processors.begin();
               itr != processors.end(); ++itr )
         {
            rc = (*itr)->doDropP1( cb, NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Processor drop operation failed[ %d ]",
                         rc ) ;
            // Set the status of the processor to dropping to make it invisible
            // to other threads.
            (*itr)->setStat( RTN_EXT_PROCESSOR_DROPPING ) ;
            _processorP1.push_back( *itr ) ;
         }
      }

   done:
      if ( metaLocked )
      {
         processorMgr->releaseMetaLock( EXCLUSIVE ) ;
      }

      if ( processors.size() > 0 )
      {
         _appendProcessors( processors ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDROPCLCTX_OPEN, rc ) ;
      return rc ;
   error:
      _onAbort( cb, dpscb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCLCTX__ONDONE, "_rtnExtDropCLCtx::_onDone")
   INT32 _rtnExtDropCLCtx::_onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCLCTX__ONDONE ) ;

      // For drop operation, the processors need to be removed.
      for ( EDP_VEC_ITR itr = _processorP1.begin(); itr != _processorP1.end();
            ++itr )
      {
         rc = (*itr)->doDropP2( cb, dpscb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Do drop phase 2 failed[ %d ]", rc ) ;
         }
      }
      _processorMgr->destroyProcessors( _processors ) ;
      _processorP1.clear() ;
      _processors.clear() ;
      _lockType = -1 ;

      PD_TRACE_EXITRC( SDB__RTNEXTDROPCLCTX__ONDONE, rc ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPCLCTX__ONABORT, "_rtnExtDropCLCtx::_onAbort")
   INT32 _rtnExtDropCLCtx::_onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDROPCLCTX__ONABORT ) ;
      for ( EDP_VEC_ITR itr = _processorP1.begin(); itr != _processorP1.end(); )
      {
         (*itr)->doDropP1Cancel( cb, dpscb ) ;
         (*itr)->setStat( RTN_EXT_PROCESSOR_NORMAL ) ;
         itr = _processorP1.erase( itr ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDROPCLCTX__ONABORT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPIDXCTX_OPEN, "_rtnExtDropIdxCtx::open")
   INT32 _rtnExtDropIdxCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                  const CHAR *extName, pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPIDXCTX_OPEN ) ;
      BOOLEAN metaLocked = FALSE ;
      BOOLEAN dropP1Done = FALSE ;
      rtnExtDataProcessor *processor = NULL ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;

      _processorMgr = processorMgr ;

   retry:
      // First try to lock the processor in EXCLUSIVE mode, to make sure that no
      // one else is currently using the processor. Then take the meta data lock
      // in exclusive mode, to block further request for any processor. Then
      // release the processor lock. In this way, we can be sure no one can use
      // the processor before we release the metadata lock.
      rc = processorMgr->getProcessorByExtName( extName, EXCLUSIVE,
                                                processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get processors failed[ %d ]", rc ) ;
      if ( !processor )
      {
         goto done ;
      }
      _lockType = EXCLUSIVE ;

      if ( !processorMgr->tryAquireMetaLock( EXCLUSIVE ) )
      {
         processorMgr->unlockProcessor( processor, EXCLUSIVE ) ;
         processor = NULL ;
         _lockType = -1 ;
         ossSleep( 50 ) ;
         goto retry ;
      }
      metaLocked = TRUE ;

      processorMgr->unlockProcessor( processor, EXCLUSIVE ) ;
      _lockType = -1 ;

      // Set the status of the processor to dropping to make it invisible to
      // other threads.
      processor->setStat( RTN_EXT_PROCESSOR_DROPPING ) ;

      // If it's full sync, just remove the processors, but not remove the
      // capped CS.
      if ( SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      rc = processor->doDropP1( cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor drop operation failed[ %d ]", rc ) ;
      dropP1Done = TRUE ;

   done:
      if ( metaLocked )
      {
         processorMgr->releaseMetaLock( EXCLUSIVE ) ;
      }

      if ( processor )
      {
         _appendProcessor( processor ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDROPIDXCTX_OPEN, rc ) ;
      return rc ;
   error:
      // In case of error, recover the index.
      if ( dropP1Done )
      {
         processor->doDropP1Cancel( cb, dpscb ) ;
      }
      processor->setStat( RTN_EXT_PROCESSOR_NORMAL ) ;
      processor = NULL ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPIDXCTX__ONDONE, "_rtnExtDropIdxCtx::_onDone")
   INT32 _rtnExtDropIdxCtx::_onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      INT32 ret = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPIDXCTX__ONDONE ) ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;

      if ( SDB_DB_FULLSYNC != dbStatus )
      {
         // For drop operation, the processors need to be removed.
         for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end();
               ++itr )
         {
            ret = (*itr)->doDropP2( cb, dpscb ) ;
            if ( ret )
            {
               PD_LOG( PDERROR, "Do drop phase 2 failed[ %d ]", rc ) ;
               if ( SDB_OK == rc )
               {
                  rc = ret ;
               }
            }
         }
      }

      // Unlock and delete all processors.
      _processorMgr->destroyProcessors( _processors ) ;
      _processors.clear() ;
      _lockType = -1 ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDROPIDXCTX__ONDONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPIDXCTX__ONABORT, "_rtnExtDropIdxCtx::_onAbort")
   INT32 _rtnExtDropIdxCtx::_onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDROPIDXCTX__ONABORT ) ;
      for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end(); )
      {
         (*itr)->doDropP1Cancel( cb, dpscb ) ;
         itr = _processors.erase( itr ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDROPIDXCTX__ONABORT ) ;
      return SDB_OK ;
   }

   _rtnExtTruncateCtx::_rtnExtTruncateCtx()
   : _rtnExtContextBase( DMS_EXTOPR_TYPE_TRUNCATE )
   {
   }

   _rtnExtTruncateCtx::~_rtnExtTruncateCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTTRUNCATECTX_OPEN, "_rtnExtTruncateCtx::open" )
   INT32 _rtnExtTruncateCtx::open( rtnExtDataProcessorMgr *processorMgr,
                                   const CHAR *csName, const CHAR *clName,
                                   pmdEDUCB *cb, BOOLEAN needChangeCLID,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTTRUNCATECTX_OPEN ) ;
      std::vector<rtnExtDataProcessor *> processors ;

      _processorMgr = processorMgr ;
      rc = processorMgr->getProcessorsByCL( csName, clName, EXCLUSIVE,
                                            processors ) ;
      PD_RC_CHECK( rc, PDERROR, "Get processors failed[ %d ]", rc ) ;
      if ( 0 == processors.size() )
      {
         goto done ;
      }
      _lockType = EXCLUSIVE ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = processors.begin();
            itr != processors.end(); ++itr )
      {
         rc = (*itr)->processTruncate( cb, needChangeCLID, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Process truncate collection failed[%d]",
                      rc ) ;
      }

   done:
      if ( _processorLocked() )
      {
         _appendProcessors( processors ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTTRUNCATECTX_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTTRUNCATECTX_DONE, "_rtnExtTruncateCtx::_onDone" )
   INT32 _rtnExtTruncateCtx::_onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTTRUNCATECTX_DONE ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rtnCB->incTextIdxVersion() ;

      PD_TRACE_EXIT( SDB__RTNEXTTRUNCATECTX_DONE ) ;
      return SDB_OK ;
   }

   _rtnExtContextMgr::_rtnExtContextMgr()
   {
   }

   _rtnExtContextMgr::~_rtnExtContextMgr()
   {
      RTN_CTX_MAP::bucket_iterator itr = _contextMap.begin() ;
      // Release all contexts in the context map, if any.
      while ( itr != _contextMap.end() )
      {
         RTN_CTX_MAP::map_iterator subItr = (*itr).begin() ;
         while ( subItr != (*itr).end() )
         {
            SAFE_OSS_DELETE( subItr->second ) ;
            ++subItr ;
         }
         itr++ ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT, "_rtnExtContextMgr::findContext" )
   rtnExtContextBase* _rtnExtContextMgr::findContext( UINT32 contextID )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT ) ;
      rtnExtContextBase *context = NULL ;
      RTN_CTX_MAP::Bucket& bucket = _contextMap.getBucket( contextID ) ;
      BUCKET_SLOCK( bucket ) ;
      RTN_CTX_MAP::map_const_iterator itr = bucket.find( contextID ) ;
      if ( itr != bucket.end() )
      {
         context = itr->second ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT ) ;
      return context ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTMGR_CREATECONTEXT, "_rtnExtContextMgr::createContext" )
   INT32 _rtnExtContextMgr::createContext( DMS_EXTOPR_TYPE type,
                                           pmdEDUCB *cb,
                                           rtnExtContextBase** context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTMGR_CREATECONTEXT ) ;
      rtnExtContextBase *newCtx = NULL ;
      UINT32 ctxID = 0 ;

      switch ( type )
      {
         case DMS_EXTOPR_TYPE_INSERT:
         case DMS_EXTOPR_TYPE_DELETE:
         case DMS_EXTOPR_TYPE_UPDATE:
            newCtx = SDB_OSS_NEW rtnExtDataOprCtx( type ) ;
            break ;
         case DMS_EXTOPR_TYPE_DROPCS:
            newCtx = SDB_OSS_NEW rtnExtDropCSCtx ;
            break ;
         case DMS_EXTOPR_TYPE_DROPCL:
            newCtx = SDB_OSS_NEW rtnExtDropCLCtx ;
            break ;
         case DMS_EXTOPR_TYPE_DROPIDX:
            newCtx = SDB_OSS_NEW rtnExtDropIdxCtx ;
            break ;
         case DMS_EXTOPR_TYPE_TRUNCATE:
            newCtx = SDB_OSS_NEW rtnExtTruncateCtx ;
            break ;
         case DMS_EXTOPR_TYPE_REBUILDIDX:
            newCtx = SDB_OSS_NEW rtnExtRebuildIdxCtx ;
            break ;
         case DMS_EXTOPR_TYPE_CRTIDX:
            newCtx = SDB_OSS_NEW rtnExtCrtIdxCtx ;
            break ;
         default:
            SDB_ASSERT( FALSE, "Invalid context type" ) ;
            rc = SDB_SYS ;
            goto error ;
      }

      if ( !newCtx )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alocate memory for context failed" ) ;
         goto error ;
      }

      ctxID = cb->getTID() ;
      newCtx->setID( ctxID ) ;
      {
         RTN_CTX_MAP::Bucket& bucket = _contextMap.getBucket( ctxID ) ;
         BUCKET_XLOCK( bucket ) ;
         bucket.insert( RTN_CTX_MAP::value_type( ctxID, newCtx ) ) ;
         if ( context )
         {
            *context = newCtx ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTCONTEXTMGR_CREATECONTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTMGR_DELCONTEXT, "_rtnExtContextMgr::delContext" )
   INT32 _rtnExtContextMgr::delContext( UINT32 contextID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTMGR_DELCONTEXT ) ;
      rtnExtContextBase *context = NULL ;

      {
         RTN_CTX_MAP::Bucket& bucket = _contextMap.getBucket( contextID ) ;
         BUCKET_XLOCK( bucket ) ;
         RTN_CTX_MAP::map_const_iterator itr = bucket.find( contextID ) ;
         if ( itr != bucket.end() )
         {
            context = itr->second ;
            bucket.erase( contextID ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Context of id[%u] not found", contextID ) ;
            goto error ;
         }
      }

      if ( context )
      {
         SDB_OSS_DEL context ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTCONTEXTMGR_DELCONTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
