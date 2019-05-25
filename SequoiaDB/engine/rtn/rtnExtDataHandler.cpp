/*******************************************************************************

   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnExtDataHandler.cpp

   Descriptive Name = External data process handler for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "rtnExtDataHandler.hpp"
#include "rtnExtDataProcessor.hpp"

namespace engine
{
   _rtnExtContextBase::_rtnExtContextBase()
   {
      _id = 0 ;
   }
   _rtnExtContextBase::~_rtnExtContextBase()
   {
   }

   void _rtnExtContextBase::appendProcessor( rtnExtDataProcessor *processor )
   {
      _processors.push_back( processor ) ;
   }

   void _rtnExtContextBase::appendProcessor( rtnExtDataProcessor *processor,
                                             const BSONObj &processData )
   {
      _processors.push_back( processor ) ;
      _objects.push_back( processData ) ;
   }

   void _rtnExtContextBase::appendProcessors( const vector< rtnExtDataProcessor * >& processorVec )
   {
      _processors.insert( _processors.end(), processorVec.begin(),
                          processorVec.end() ) ;
   }

   _rtnExtDataOprCtx::_rtnExtDataOprCtx()
   {
   }

   _rtnExtDataOprCtx::~_rtnExtDataOprCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAOPRCTX_DONE, "_rtnExtDataOprCtx::done" )
   INT32 _rtnExtDataOprCtx::done( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAOPRCTX_DONE ) ;
      OBJ_VEC_ITR objItr = _objects.begin() ;
      for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end();
            ++itr )
      {
         SDB_ASSERT( objItr != _objects.end(), "objItr should not reach end" ) ;
         rc  = (*itr)->doWrite( cb,  *objItr, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Processor operation failed[ %d ]", rc ) ;
         objItr++ ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAOPRCTX_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAOPRCTX_ABORT, "_rtnExtDataOprCtx::abort" )
   INT32 _rtnExtDataOprCtx::abort( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAOPRCTX_ABORT ) ;
      _processors.clear() ;
      _objects.clear() ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAOPRCTX_ABORT ) ;
      return SDB_OK ;
   }

   _rtnExtDropOprCtx::_rtnExtDropOprCtx()
   : _removeFiles( FALSE )
   {
   }

   _rtnExtDropOprCtx::~_rtnExtDropOprCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPOPRCTX_DONE, "_rtnExtDropOprCtx::done" )
   INT32 _rtnExtDropOprCtx::done( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPOPRCTX_DONE ) ;
      if ( _removeFiles )
      {
         for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end();)
         {
            rc = (*itr)->doDropP2( cb, dpscb ) ;
            PD_RC_CHECK( rc, PDERROR, "Do drop phase 2 failed[ %d ]", rc ) ;
            edpMgr->delProcessor( &(*itr ) ) ;
            _processors.erase( itr ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDROPOPRCTX_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPOPRCTX_ABORT, "_rtnExtDropOprCtx::abort" )
   INT32 _rtnExtDropOprCtx::abort( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDROPOPRCTX_ABORT ) ;
      for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end(); )
      {
         (*itr)->doDropP1Cancel( cb, dpscb ) ;
         _processors.erase( itr ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDROPOPRCTX_ABORT ) ;
      return SDB_OK ;
   }

   _rtnExtTruncateCtx::_rtnExtTruncateCtx()
   : _oldCLLID( 0 ),
     _newCLLID( 0 )
   {
   }

   _rtnExtTruncateCtx::~_rtnExtTruncateCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTTRUNCATECTX_DONE, "_rtnExtTruncateCtx::done" )
   INT32 _rtnExtTruncateCtx::done( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTTRUNCATECTX_DONE ) ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = _processors.begin();
            itr != _processors.end(); ++itr )
      {
         rc = (*itr)->doDropP2( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop phase 2 failed[ %d ]", rc ) ;
         rc = (*itr)->doRebuild( cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "External data rebuild failed[ %d ]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTTRUNCATECTX_DONE, rc ) ;
      return rc ;
   error:
      for ( vector<rtnExtDataProcessor *>::iterator itr = _processors.begin();
            itr != _processors.end();)
      {
         (*itr)->doDropP1Cancel( cb, NULL ) ;
         _processors.erase( itr ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTTRUNCATECTX_ABORT, "_rtnExtTruncateCtx::abort" )
   INT32 _rtnExtTruncateCtx::abort( rtnExtDataProcessorMgr *edpMgr,
                                    pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTTRUNCATECTX_ABORT ) ;
      for ( vector<rtnExtDataProcessor *>::iterator itr = _processors.begin();
            itr != _processors.end(); )
      {
         (*itr)->doDropP1Cancel( cb, dpscb ) ;
         _processors.erase( itr ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTTRUNCATECTX_ABORT ) ;
      return SDB_OK ;
   }

   void _rtnExtTruncateCtx::setCLLIDs( UINT32 oldLID, UINT32 newLID )
   {
      _oldCLLID = oldLID ;
      _newCLLID = newLID ;
   }

   _rtnExtContextMgr::_rtnExtContextMgr()
   {
   }

   _rtnExtContextMgr::~_rtnExtContextMgr()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT, "_rtnExtContextMgr::findContext" )
   rtnExtContextBase* _rtnExtContextMgr::findContext( UINT32 contextID )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT ) ;
      rtnExtContextBase *context = NULL ;
      ossScopedRWLock lock( &_mutex, SHARED ) ;
      std::pair< rtnExtContextBase*, bool > ret = _contextMap.find( contextID ) ;
      if ( ret.second )
      {
         context = ret.first ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTMGR_FINDCONTEXT ) ;
      return context ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTMGR_CREATECONTEXT, "_rtnExtContextMgr::createContext" )
   INT32 _rtnExtContextMgr::createContext( RTN_EXTCTX_TYPE type,
                                           pmdEDUCB *cb,
                                           rtnExtContextBase** context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTMGR_CREATECONTEXT ) ;
      rtnExtContextBase *newCtx = NULL ;
      UINT32 ctxID = 0 ;

      switch ( type )
      {
      case RTN_EXTCTX_TYPE_INSERT:
      case RTN_EXTCTX_TYPE_DELETE:
      case RTN_EXTCTX_TYPE_UPDATE:

         newCtx = SDB_OSS_NEW rtnExtDataOprCtx ;
         break ;
      case RTN_EXTCTX_TYPE_DROPCS:
      case RTN_EXTCTX_TYPE_DROPCL:
      case RTN_EXTCTX_TYPE_DROPIDX:
         newCtx = SDB_OSS_NEW rtnExtDropOprCtx ;
         break ;
      case RTN_EXTCTX_TYPE_TRUNCATE:
         newCtx = SDB_OSS_NEW rtnExtTruncateCtx ;
         break ;
      default:
         SDB_ASSERT( FALSE, "Invalid context type" ) ;
         break ;
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
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _contextMap.insert( ctxID, newCtx ) ;
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
      std::pair< rtnExtContextBase*, bool > ret ;

      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         ret = _contextMap.find( contextID ) ;
      }
      if ( ret.second )
      {
         context = ret.first ;
         ossScopedRWLock wLock( &_mutex, EXCLUSIVE ) ;
         _contextMap.erase( contextID ) ;
         SDB_OSS_DEL context ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTCONTEXTMGR_DELCONTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDataHandler::_rtnExtDataHandler( rtnExtDataProcessorMgr *edpMgr )
   {
      _edpMgr = edpMgr ;
   }

   _rtnExtDataHandler::~_rtnExtDataHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX, "_rtnExtDataHandler::onOpenTextIdx" )
   INT32 _rtnExtDataHandler::onOpenTextIdx( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX ) ;
      rtnExtDataProcessor *processor = NULL ;

      rc = _edpMgr->addProcessor( csName, clName, idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Add external processor failed[ %d ]", rc ) ;
      SDB_ASSERT( processor, "processor is NULL" ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELCS, "_rtnExtDataHandler::onDelCS" )
   INT32 _rtnExtDataHandler::onDelCS( const CHAR *csName, pmdEDUCB *cb,
                                      BOOLEAN removeFiles, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELCS ) ;
      rtnExtDropOprCtx *context = NULL ;
      vector <rtnExtDataProcessor *> processorVec ;
      vector <rtnExtDataProcessor *> processorVecP1 ;

      context = (rtnExtDropOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( RTN_EXTCTX_TYPE_DROPCS,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }
      context->setRemoveFiles( removeFiles ) ;

      _edpMgr->getProcessors( csName, processorVec ) ;
      if ( 0 == processorVec.size() )
      {
         goto done ;
      }

      for ( vector<rtnExtDataProcessor *>::iterator itr = processorVec.begin();
            itr != processorVec.end(); ++itr )
      {
         if ( removeFiles )
         {
            rc = (*itr)->doDropP1( cb, NULL ) ;
         }
         else
         {
            rc = (*itr)->doUnload( cb, NULL ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Processor drop operation failed[ %d ]",
                      rc ) ;
         processorVecP1.push_back( *itr ) ;
      }
      context->appendProcessors( processorVec );

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELCS, rc ) ;
      return rc ;
   error:
      if ( removeFiles )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = processorVecP1.begin();
               itr != processorVecP1.end(); ++itr )
         {
            (*itr)->doDropP1Cancel( cb, NULL ) ;
         }
      }

      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDROPALLINDEXES, "_rtnExtDataHandler::onDropAllIndexes" )
   INT32 _rtnExtDataHandler::onDropAllIndexes( const CHAR *csName,
                                               const CHAR *clName,
                                               pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDROPALLINDEXES ) ;
      vector<rtnExtDataProcessor *> processorVec ;
      vector<rtnExtDataProcessor *> processorP1 ;

      _edpMgr->getProcessors( csName, clName, processorVec ) ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = processorVec.begin();
            itr != processorVec.end(); ++itr )
      {
         rc = (*itr)->doDropP1( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop phase 1 failed[ %d ]", rc ) ;
         processorP1.push_back( *itr ) ;
      }

      for ( vector<rtnExtDataProcessor *>::iterator itr = processorP1.begin();
            itr != processorP1.end(); )
      {
         rc = (*itr)->doDropP2( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop phase 2 failed[ %d ]", rc ) ;
         _edpMgr->delProcessor( &(*itr ) ) ;
         processorP1.erase( itr ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDROPALLINDEXES, rc ) ;
      return rc ;
   error:
      for ( vector<rtnExtDataProcessor *>::iterator itr = processorP1.begin();
            itr != processorP1.end(); ++itr )
      {
         (*itr)->doDropP1Cancel( cb, NULL ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX, "_rtnExtDataHandler::onDropTextIdx" )
   INT32 _rtnExtDataHandler::onDropTextIdx( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName,
                                            pmdEDUCB *cb,
                                            SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX ) ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;

      if ( SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      rc = _edpMgr->getProcessor( csName, clName, idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
      if ( !processor )
      {
         goto done ;
      }
      rc = processor->doDropP1( cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor drop operation failed[ %d ]", rc ) ;
      rc = processor->doDropP2( cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor drop operation failed[ %d ]", rc ) ;
      _edpMgr->delProcessor( &processor ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX, "_rtnExtDataHandler::onRebuildTextIdx" )
   INT32 _rtnExtDataHandler::onRebuildTextIdx( const CHAR *csName,
                                               const CHAR *clName,
                                               const CHAR *idxName,
                                               pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX ) ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;

      BOOLEAN create = ( SDB_DB_NORMAL == dbStatus
                         || SDB_DB_FULLSYNC == dbStatus ) ? TRUE : FALSE ;

      SDB_ASSERT( cb, "eduCB is NULL" ) ;

      rc = _edpMgr->getProcessor( csName, clName, idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor for index "
                   "failed[ %d ]", rc ) ;
      if ( !processor )
      {
         if ( create )
         {
            rc = _edpMgr->addProcessor( csName, clName, idxName, &processor ) ;
            PD_RC_CHECK( rc, PDERROR, "Add processor for collection[ %s.%s ] "
                         "and index[ %s ] failed[ %d ]", csName, clName,
                         idxName, rc ) ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get external data processor failed[ %d ]", rc ) ;
            goto error ;
         }
      }

      rc = processor->doRebuild( cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Rebuild of index failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONINSERT, "_rtnExtDataHandler::onInsert" )
   INT32 _rtnExtDataHandler::onInsert( const CHAR *csName, const CHAR *clName,
                                       const CHAR *idxName,
                                       const ixmIndexCB &indexCB,
                                       const BSONObj &object, _pmdEDUCB* cb,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONINSERT ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DB_STATUS dbStatus = krcb->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;
      rtnExtDataOprCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;
      BSONObj processData ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtDataOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( RTN_EXTCTX_TYPE_INSERT,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         newContext = TRUE ;
      }

      rc = _edpMgr->getProcessor( csName, clName, idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
      SDB_ASSERT( processor, "Processor is NULL" ) ;
      if ( !processor )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get external processor failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = processor->prepareInsert( indexCB, object, processData ) ;
      PD_RC_CHECK( rc, PDERROR, "External processor handle insert failed[ %d ]",
                   rc ) ;

      context->appendProcessor( processor, processData ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONINSERT, rc ) ;
      return rc ;
   error:
      if ( context && newContext )
      {
         INT32 rcTmp = _contextMgr.delContext( context->getID(), cb ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Delete context failed[ %d ]", rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELETE, "_rtnExtDataHandler::onDelete" )
   INT32 _rtnExtDataHandler::onDelete( const CHAR *csName, const CHAR *clName,
                                       const CHAR *idxName,
                                       const ixmIndexCB &indexCB,
                                       const BSONObj &object, _pmdEDUCB* cb,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELETE ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DB_STATUS dbStatus = krcb->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;
      rtnExtDataOprCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;
      BSONObj processData ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtDataOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( RTN_EXTCTX_TYPE_DELETE,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         newContext = TRUE ;
      }

      rc = _edpMgr->getProcessor( csName, clName,
                                  idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
      SDB_ASSERT( processor, "Processor is NULL" ) ;
      if ( !processor )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get external processor failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = processor->prepareDelete( indexCB, object, processData ) ;
      PD_RC_CHECK( rc, PDERROR, "External processor handle delete failed[ %d ]",
                   rc ) ;
      context->appendProcessor( processor, processData ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELETE, rc ) ;
      return rc ;
   error:
      if ( context && newContext )
      {
         INT32 rcTmp = _contextMgr.delContext( context->getID(), cb ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Delete context failed[ %d ]", rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONUPDATE, "_rtnExtDataHandler::onUpdate" )
   INT32 _rtnExtDataHandler::onUpdate( const CHAR *csName, const CHAR *clName,
                                       const CHAR *idxName,
                                       const ixmIndexCB &indexCB,
                                       const BSONObj &orignalObj,
                                       const BSONObj &newObj,
                                       pmdEDUCB* cb,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONUPDATE ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DB_STATUS dbStatus = krcb->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;
      rtnExtDataOprCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;
      BSONObj processData ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtDataOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( RTN_EXTCTX_TYPE_UPDATE,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         newContext = TRUE ;
      }

      rc = _edpMgr->getProcessor( csName, clName, idxName, &processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
      SDB_ASSERT( processor, "Processor is NULL" ) ;
      if ( !processor )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get external processor failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = processor->prepareUpdate( indexCB, orignalObj, newObj, processData ) ;
      PD_RC_CHECK( rc, PDERROR, "External processor handle update failed[ %d ]",
                   rc ) ;

      context->appendProcessor( processor, processData ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONUPDATE, rc ) ;
      return rc ;
   error:
      if ( context && newContext )
      {
         INT32 rcTmp = _contextMgr.delContext( context->getID(), cb ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Delete context failed[ %d ]", rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL, "_rtnExtDataHandler::onTruncateCL" )
   INT32 _rtnExtDataHandler::onTruncateCL( const CHAR *csName,
                                           const CHAR *clName,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL ) ;
      rtnExtTruncateCtx *context = NULL ;
      vector<rtnExtDataProcessor *> processors ;
      vector<rtnExtDataProcessor *> processorP1 ;

      context = (rtnExtTruncateCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( RTN_EXTCTX_TYPE_TRUNCATE,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      _edpMgr->getProcessors( csName, clName, processors ) ;
      PD_RC_CHECK( rc, PDERROR, "Get processors failed[ %d ]", rc ) ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = processors.begin();
            itr != processors.end(); ++itr )
      {
         rc = (*itr)->doDropP1( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop phase 1 failed[ %d ]", rc ) ;
         processorP1.push_back( *itr ) ;
      }
      context->appendProcessors( processors ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL, rc ) ;
      return rc ;
   error:
      INT32 rcTmp = SDB_OK ;
      for ( vector<rtnExtDataProcessor *>::iterator itr = processorP1.begin();
            itr != processorP1.end(); ++itr )
      {
         rcTmp = (*itr)->doDropP1Cancel( cb, dpsCB ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Drop phase 1 cancel failed[ %d ]", rc ) ;
         }
      }
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_DONE, "_rtnExtDataHandler::done" )
   INT32 _rtnExtDataHandler::done( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_DONE ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         goto done ;
      }

      rc = context->done( _edpMgr, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                   rc) ;

   done:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ABORTOPERATION, "_rtnExtDataHandler::abortOperation" )
   INT32 _rtnExtDataHandler::abortOperation( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ABORTOPERATION ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         goto done ;
      }

      rc = context->abort( _edpMgr, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                   rc) ;

   done:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ABORTOPERATION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   rtnExtDataHandler* rtnGetExtDataHandler()
   {
      static rtnExtDataHandler s_edh( rtnGetExtDataProcessorMgr() ) ;
      return &s_edh ;
   }
}

