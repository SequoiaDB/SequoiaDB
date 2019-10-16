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
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE, "_rtnExtContextBase::_rtnExtContextBase" )
   _rtnExtContextBase::_rtnExtContextBase( DMS_EXTOPR_TYPE type )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE ) ;
      _id = 0 ;
      _type = type ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE__RTNEXTCONTEXTBASE ) ;
   }

   _rtnExtContextBase::~_rtnExtContextBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSOR, "_rtnExtContextBase::appendProcessor" )
   void _rtnExtContextBase::appendProcessor( rtnExtDataProcessor *processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSOR ) ;
      _processors.push_back( processor ) ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS, "_rtnExtContextBase::appendProcessors" )
   void _rtnExtContextBase::appendProcessors( const vector< rtnExtDataProcessor * >& processorVec )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS ) ;
      _processors.insert( _processors.end(), processorVec.begin(),
                          processorVec.end() ) ;
      PD_TRACE_EXIT( SDB__RTNEXTCONTEXTBASE_APPENDPROCESSORS ) ;
   }

   _rtnExtDataOprCtx::_rtnExtDataOprCtx( DMS_EXTOPR_TYPE type )
   : _rtnExtContextBase( type ),
     _srcData1( NULL ),
     _srcData2( NULL )
   {
   }

   _rtnExtDataOprCtx::~_rtnExtDataOprCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAOPRCTX_SETSRCDATA, "_rtnExtDataOprCtx::setSrcData" )
   void _rtnExtDataOprCtx::setSrcData( const BSONObj *srcData1,
                                       const BSONObj *srcData2 )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAOPRCTX_SETSRCDATA ) ;
      if ( srcData1 )
      {
         _srcData1 = srcData1 ;
      }
      if ( srcData2 )
      {
         _srcData2 = srcData2 ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAOPRCTX_SETSRCDATA ) ;
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

   _rtnExtInsertCtx::_rtnExtInsertCtx()
   : _rtnExtDataOprCtx( DMS_EXTOPR_TYPE_INSERT )
   {
   }

   _rtnExtInsertCtx::~_rtnExtInsertCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTINSERTCTX_PROCESS, "_rtnExtInsertCtx::process" )
   INT32 _rtnExtInsertCtx::process( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTINSERTCTX_PROCESS ) ;
      BSONObj record ;

      rtnExtDataProcessor *processor = _processors.back() ;

      rc = processor->prepareInsert( *_srcData1, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare for insert failed[ %d ]",
                   rc ) ;
      _objects.push_back( record ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTINSERTCTX_PROCESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDeleteCtx::_rtnExtDeleteCtx()
   : _rtnExtDataOprCtx( DMS_EXTOPR_TYPE_DELETE )
   {
   }

   _rtnExtDeleteCtx::~_rtnExtDeleteCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDELETECTX_PROCESS, "_rtnExtDeleteCtx::process" )
   INT32 _rtnExtDeleteCtx::process( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDELETECTX_PROCESS ) ;
      BSONObj record ;

      rtnExtDataProcessor *processor = _processors.back() ;

      rc = processor->prepareDelete( *_srcData1, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare for delete failed[ %d ]",
                   rc ) ;
      _objects.push_back( record ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDELETECTX_PROCESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtUpdateCtx::_rtnExtUpdateCtx()
   : _rtnExtDataOprCtx( DMS_EXTOPR_TYPE_UPDATE )
   {
   }

   _rtnExtUpdateCtx::~_rtnExtUpdateCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTUPDATECTX_PROCESS, "_rtnExtUpdateCtx::process" )
   INT32 _rtnExtUpdateCtx::process( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTUPDATECTX_PROCESS ) ;
      BSONObj record ;

      rtnExtDataProcessor *processor = _processors.back() ;

      rc = processor->prepareUpdate( *_srcData1, *_srcData2, record ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare for delete failed[ %d ]",
                   rc ) ;
      _objects.push_back( record ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTUPDATECTX_PROCESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDropOprCtx::_rtnExtDropOprCtx( DMS_EXTOPR_TYPE type )
   : _rtnExtContextBase( type ),
     _removeFiles( FALSE )
   {
   }

   _rtnExtDropOprCtx::~_rtnExtDropOprCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPOPRCTX_PROCESS, "_rtnExtDropOprCtx::process" )
   INT32 _rtnExtDropOprCtx::process( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPOPRCTX_PROCESS ) ;
      vector <rtnExtDataProcessor *> processorVecP1 ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = _processors.begin();
            itr != _processors.end(); ++itr )
      {
         if ( _removeFiles )
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

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDROPOPRCTX_PROCESS, rc ) ;
      return rc ;
   error:
      if ( _removeFiles )
      {
         for ( vector<rtnExtDataProcessor *>::iterator itr = processorVecP1.begin();
               itr != processorVecP1.end(); ++itr )
         {
            (*itr)->doDropP1Cancel( cb, NULL ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDROPOPRCTX_DONE, "_rtnExtDropOprCtx::done" )
   INT32 _rtnExtDropOprCtx::done( rtnExtDataProcessorMgr *edpMgr, pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      INT32 ret = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDROPOPRCTX_DONE ) ;

      for ( EDP_VEC_ITR itr = _processors.begin(); itr != _processors.end();)
      {
         if ( _removeFiles )
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
         edpMgr->delProcessor( &(*itr ) ) ;
         _processors.erase( itr ) ;
      }

      if ( rc )
      {
         goto error ;
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
   : _rtnExtContextBase( DMS_EXTOPR_TYPE_TRUNCATE )
   {
   }

   _rtnExtTruncateCtx::~_rtnExtTruncateCtx()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTTRUNCATECTX_PROCESS, "_rtnExtTruncateCtx::process" )
   INT32 _rtnExtTruncateCtx::process( pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTTRUNCATECTX_PROCESS ) ;
      vector<rtnExtDataProcessor *> processorP1 ;

      for ( vector<rtnExtDataProcessor *>::iterator itr = _processors.begin();
            itr != _processors.end(); ++itr )
      {
         rc = (*itr)->doDropP1( cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop phase 1 failed[ %d ]", rc ) ;
         processorP1.push_back( *itr ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTTRUNCATECTX_PROCESS, rc ) ;
      return rc ;
   error:
      INT32 rcTmp = SDB_OK ;
      for ( vector<rtnExtDataProcessor *>::iterator itr = processorP1.begin();
            itr != processorP1.end(); ++itr )
      {
         rcTmp = (*itr)->doDropP1Cancel( cb, NULL ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Drop phase 1 cancel failed[ %d ]", rc ) ;
         }
      }
      goto done ;
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
         newCtx = SDB_OSS_NEW rtnExtInsertCtx ;
         break ;
      case DMS_EXTOPR_TYPE_DELETE:
         newCtx = SDB_OSS_NEW rtnExtDeleteCtx ;
         break ;
      case DMS_EXTOPR_TYPE_UPDATE:
         newCtx = SDB_OSS_NEW rtnExtUpdateCtx ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_GETEXTDATANAME, "_rtnExtDataHandler::getExtDataName" )
   INT32 _rtnExtDataHandler::getExtDataName( const CHAR *csName,
                                             const CHAR *clName,
                                             const CHAR *idxName,
                                             CHAR *buff, UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_GETEXTDATANAME ) ;
      SDB_ASSERT( csName && clName && idxName, "Names should not be NULL" ) ;
      if ( !buff )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer is not valid" ) ;
         goto error ;
      }
      else if ( buffSize < DMS_COLLECTION_FULL_NAME_SZ + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size too small" ) ;
         goto error ;
      }
      rtnExtDataProcessor::getExtDataNames( csName, clName, idxName, NULL, 0,
                                            buff, buffSize ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_GETEXTDATANAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX, "_rtnExtDataHandler::onOpenTextIdx" )
   INT32 _rtnExtDataHandler::onOpenTextIdx( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName,
                                            const BSONObj &idxKeyDef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX ) ;
      rtnExtDataProcessor *processor = NULL ;

      rc = _edpMgr->addProcessor( csName, clName, idxName,
                                  idxKeyDef, &processor ) ;
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

      context = (rtnExtDropOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPCS,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      _edpMgr->getProcessors( csName, processorVec ) ;
      if ( 0 == processorVec.size() )
      {
         goto done ;
      }
      context->setRemoveFiles( removeFiles ) ;
      context->appendProcessors( processorVec );
      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context process failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELCS, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELCL, "_rtnExtDataHandler::onDelCL" )
   INT32 _rtnExtDataHandler::onDelCL( const CHAR *csName, const CHAR *clName,
                                      pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELCL ) ;

      rtnExtDropOprCtx *context = NULL ;
      vector< rtnExtDataProcessor *> processorVec ;

      context = (rtnExtDropOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPCL,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      _edpMgr->getProcessors( csName, clName, processorVec ) ;
      if ( 0 == processorVec.size() )
      {
         goto done ;
      }
      context->setRemoveFiles( TRUE ) ;
      context->appendProcessors( processorVec ) ;
      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context process failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELCL, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
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
      rtnExtDropOprCtx *context = NULL ;

      if ( SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtDropOprCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPIDX,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      if ( DMS_EXTOPR_TYPE_DROPIDX == context->getType() )
      {
         rc = _edpMgr->getProcessor( csName, clName, idxName, &processor ) ;
         PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
         if ( !processor )
         {
            goto done ;
         }

         context->setRemoveFiles( TRUE ) ;
         context->appendProcessor( processor ) ;
         rc = context->process( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Context process failed[ %d ]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX, "_rtnExtDataHandler::onRebuildTextIdx" )
   INT32 _rtnExtDataHandler::onRebuildTextIdx( const CHAR *csName,
                                               const CHAR *clName,
                                               const CHAR *idxName,
                                               const BSONObj &idxKeyDef,
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
            rc = _edpMgr->addProcessor( csName, clName, idxName,
                                        idxKeyDef, &processor ) ;
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
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      rtnExtDataProcessor *processor = NULL ;
      rtnExtInsertCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtInsertCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_INSERT,
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

      context->appendProcessor( processor ) ;
      context->setSrcData( &object ) ;
      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context processing failed[ %d ]", rc ) ;

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
      rtnExtDeleteCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;
      BSONObj processData ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtDeleteCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DELETE,
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

      context->appendProcessor( processor ) ;
      context->setSrcData( &object ) ;
      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context processing failed[ %d ]", rc ) ;

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
      rtnExtUpdateCtx *context = NULL ;
      BOOLEAN newContext = FALSE ;

      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      context = (rtnExtUpdateCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_UPDATE,
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

      context->appendProcessor( processor ) ;
      context->setSrcData( &orignalObj, &newObj ) ;

      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context processing failed[ %d ]", rc ) ;

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
      BOOLEAN newContext = FALSE ;

      context = (rtnExtTruncateCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_TRUNCATE,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         newContext = TRUE ;
      }

      _edpMgr->getProcessors( csName, clName, processors ) ;
      PD_RC_CHECK( rc, PDERROR, "Get processors failed[ %d ]", rc ) ;
      if ( 0 == processors.size() )
      {
         goto done ;
      }

      context->appendProcessors( processors ) ;
      rc = context->process( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Context processing failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL, rc ) ;
      return rc ;
   error:
      if ( context && newContext )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_DONE, "_rtnExtDataHandler::done" )
   INT32 _rtnExtDataHandler::done( DMS_EXTOPR_TYPE type, pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_DONE ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      BOOLEAN ownContext = FALSE ;
      if ( !context )
      {
         goto done ;
      }

      ownContext = ( context->getType() == type ) ;
      if ( !ownContext )
      {
         goto done ;
      }

      rc = context->done( _edpMgr, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                      rc) ;

   done:
      if ( context && ownContext )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ABORTOPERATION, "_rtnExtDataHandler::abortOperation" )
   INT32 _rtnExtDataHandler::abortOperation( DMS_EXTOPR_TYPE type,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ABORTOPERATION ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      BOOLEAN ownContext = FALSE ;
      if ( !context )
      {
         goto done ;
      }

      ownContext = ( context->getType() == type ) ;
      if ( !ownContext )
      {
         goto done ;
      }

      rc = context->abort( _edpMgr, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                   rc) ;

   done:
      if ( context && ownContext )
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

