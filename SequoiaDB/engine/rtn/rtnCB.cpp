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

   Source File Name = rtnCB.cpp

   Descriptive Name = Runtime Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dmsCB.hpp"
#include "rtnBackgroundJob.hpp"
#include "pmdLightJobMgr.hpp"
#include "pmdController.hpp"

using namespace std;
namespace engine
{

   /*
      _rtnClearExpireContextJob define
    */
   class _rtnClearExpireContextJob : public utilLightJob
   {
   public:
      _rtnClearExpireContextJob( SDB_RTNCB *rtnCB )
      : _rtnCB( rtnCB )
      {
         SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" ) ;
      }

      virtual ~_rtnClearExpireContextJob()
      {
      }

      virtual const CHAR *name() const
      {
         return "ClearContextJob" ;
      }

      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime )
      {
         sleepTime = RTN_CTX_CHECK_INTERVAL ;

         if ( PMD_IS_DB_DOWN() )
         {
            result = UTIL_LJOB_DO_FINISH ;
         }
         else
         {
            _rtnCB->preDelExpiredContext() ;
            result = UTIL_LJOB_DO_CONT ;
         }

         return SDB_OK ;
      }

   protected:
      SDB_RTNCB * _rtnCB ;
   } ;
   typedef class _rtnClearExpireContextJob rtnClearExpireContextJob ;

   /*
      _rtnClearUserCacheJob define
   */
   class _rtnClearUserCacheJob : public utilLightJob
   {
   public:
      _rtnClearUserCacheJob( SDB_RTNCB *rtnCB ) : _rtnCB( rtnCB )
      {
         SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" );
      }

      virtual ~_rtnClearUserCacheJob() {}

      virtual const CHAR *name() const
      {
         return "ClearUserCacheJob";
      }

      virtual INT32 doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result, UINT64 &sleepTime )
      {
         // UINT64 microseconds
         sleepTime = pmdGetOptionCB()->getUserCacheInterval() * 1000ULL;

         if ( PMD_IS_DB_DOWN() )
         {
            result = UTIL_LJOB_DO_FINISH;
         }
         else
         {
            _rtnCB->getUserCacheMgr()->clear();
            result = UTIL_LJOB_DO_CONT;
         }

         return SDB_OK;
      }

   protected:
      SDB_RTNCB *_rtnCB;
   };
   typedef class _rtnClearUserCacheJob rtnClearUserCacheJob;

   _SDB_RTNCB::_SDB_RTNCB()
      : _contextIdGenerator( 0 ),
        _maxContextNum( RTN_MAX_CTX_NUM_DFT ),
        _maxSessionContextNum( RTN_MAX_SESS_CTX_NUM_DFT ),
        _contextTimeout( RTN_CTX_TIMEOUT_DFT ),
        _remoteMessenger( NULL ),
        _textIdxVersion((INT64)RTN_INIT_TEXT_INDEX_VERSION)
   {
      _pLTMgr = NULL ;
   }

   _SDB_RTNCB::~_SDB_RTNCB()
   {
      _contextMap.clear() ;
   }

   void* _SDB_RTNCB::queryInterface( SDB_INTERFACE_TYPE type )
   {
      if ( SDB_IF_CTXMGR == type )
      {
         return dynamic_cast<IContextMgr*>( this ) ;
      }
      return IControlBlock::queryInterface( type ) ;
   }

   void _SDB_RTNCB::onPrimaryChange( BOOLEAN primary,
                                     SDB_EVENT_OCCUR_TYPE occurType )
   {
      if ( !primary && SDB_EVT_OCCUR_AFTER == occurType )
      {
         if ( _pLTMgr )
         {
            _pLTMgr->clear() ;
         }
      }
   }

   INT32 _SDB_RTNCB::init ()
   {
      INT32 rc = SDB_OK ;

      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      _pLTMgr = SDB_OSS_NEW rtnLocalTaskMgr() ;
      if ( !_pLTMgr )
      {
         PD_LOG( PDERROR, "Failed to create local task manager" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( &_sorterCreator ) ;
      sdbGetDMSCB()->setScannerCheckerCreator( &_checkerCreator ) ;

      // The error of initialization of APM could be ignore
      // Only data and catalog nodes could initialize plan cache
      _accessPlanManager.init(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ||
              SDB_ROLE_OM == pmdGetDBRole() ) ?
                    optionCB->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL)( optionCB->getPlanCacheLevel() ),
            optionCB->getSortBufSize(),
            optionCB->getOptCostThreshold(),
            optionCB->isEnabledMixCmp(),
            optionCB->getPlanCacheMainCLThreshold() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;
      _contextTimeout = optionCB->contextTimeout() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::active ()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_CATALOG == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() ||
           SDB_ROLE_OM == pmdGetDBRole() )
      {
         rc = pmdGetKRCB()->getDMSCB()->regHandler( &_accessPlanManager ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register event handler of "
                      "access plan manager to DMS, rc: %d", rc ) ;
      }

      {
         UINT64 jobID = 0 ;
         rtnClearExpireContextJob *job = SDB_OSS_NEW rtnClearExpireContextJob( this ) ;
         PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate clear context job" ) ;

         rc = job->submit( TRUE, UTIL_LJOB_PRI_LOWEST, UTIL_LJOB_DFT_AVG_COST, &jobID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to submit clear context job, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "submit clear context job [%llu]", jobID ) ;
      }

      if ( pmdGetOptionCB()->getUserCacheInterval() > 0 )
      {
         UINT64 jobID = 0 ;
         rtnClearUserCacheJob *job = SDB_OSS_NEW rtnClearUserCacheJob( this ) ;
         PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate clear user cache job" ) ;

         rc = job->submit( TRUE, UTIL_LJOB_PRI_LOWEST, UTIL_LJOB_DFT_AVG_COST, &jobID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to submit clear user cache job, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "submit clear user cache job [%llu]", jobID ) ;
      }

      if ( SDB_ROLE_DATA       == pmdGetKRCB()->getDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetKRCB()->getDBRole() )
      {
         rc = rtnStartCleanupIdxStatusJob() ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to start clean up index status job" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::deactive ()
   {
      if ( _remoteMessenger )
      {
         _remoteMessenger->deactive() ;
      }
      if ( SDB_ROLE_DATA == pmdGetDBRole() ||
           SDB_ROLE_CATALOG == pmdGetDBRole() ||
           SDB_ROLE_STANDALONE == pmdGetDBRole() ||
           SDB_ROLE_OM == pmdGetDBRole() )
      {
         pmdGetKRCB()->getDMSCB()->unregHandler( &_accessPlanManager ) ;
      }
      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::fini ()
   {
      _accessPlanManager.fini() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( NULL ) ;
      sdbGetDMSCB()->setScannerCheckerCreator( NULL ) ;

      rtnJobMgr* jobMgr = rtnGetJobMgr() ;
      jobMgr->fini() ;

      rtnIndexJobHolder *idxJobHolder = rtnGetIndexJobHolder() ;
      idxJobHolder->fini() ;

      if ( _remoteMessenger )
      {
         SDB_OSS_DEL _remoteMessenger ;
      }

      if ( _pLTMgr )
      {
         _pLTMgr->fini() ;
         SDB_OSS_DEL _pLTMgr ;
         _pLTMgr = NULL ;
      }

      _unloadCSSet.clear() ;

      return SDB_OK ;
   }

   void _SDB_RTNCB::onConfigChange ()
   {
      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      _accessPlanManager.reinit(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ) ?
                    optionCB->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL)( optionCB->getPlanCacheLevel() ),
            optionCB->getSortBufSize(),
            optionCB->getOptCostThreshold(),
            optionCB->isEnabledMixCmp(),
            optionCB->getPlanCacheMainCLThreshold() ) ;

      _maxContextNum = optionCB->maxContextNum() ;
      _maxSessionContextNum = optionCB->maxSessionContextNum() ;
      _contextTimeout = optionCB->contextTimeout() ;
   }

   void _SDB_RTNCB::_setGlobalID( _pmdEDUCB *cb, rtnContextPtr &pContext )
   {
      if ( cb )
      {
         pmdOperator *pOperator = cb->getOperator() ;
         MsgGlobalID sessionOpGlobalID  = pOperator->getGlobalID() ;
         MsgGlobalID contextGlobalID    = pContext->getGlobalID() ;

         if ( sessionOpGlobalID.getQueryID() != contextGlobalID.getQueryID() )
         {
            // when getMore, the queryOpID should add 1
            contextGlobalID.incQueryOpID() ;
            pContext->_setGlobalID( contextGlobalID ) ;
            pOperator->updateGlobalID( contextGlobalID ) ;
         }
         else if ( sessionOpGlobalID.getQueryOpID() != contextGlobalID.getQueryOpID() )
         {
            pContext->_setGlobalID( sessionOpGlobalID ) ;
         }
      }
   }

   INT32 _SDB_RTNCB::contextFind( INT64 contextID,
                                  rtnContextPtr &context,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      std::pair<rtnContextPtr, bool> ret = _contextMap.find( contextID ) ;
      if ( ret.second )
      {
         if ( cb && !cb->contextFind( contextID ) )
         {
            PD_LOG ( PDWARNING, "Context %lld does not owned by "
                     "current session", contextID ) ;
            rc = SDB_RTN_CONTEXT_NOTEXIST ;
         }
         else
         {
            context = ret.first ;
            _setGlobalID( cb, context ) ;
         }
      }
      else
      {
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
      }

      return rc ;
   }

   INT32 _SDB_RTNCB::contextFind( INT64 contextID,
                                  RTN_CONTEXT_TYPE type,
                                  rtnContextPtr &context,
                                  _pmdEDUCB *cb,
                                  BOOLEAN closeOnUnexpectType )
   {
      INT32 rc = SDB_OK ;

      rtnContextPtr tempContext ;
      rc = contextFind( contextID, tempContext, cb ) ;
      if ( SDB_OK == rc )
      {
         if ( type == tempContext->getType() )
         {
            context = tempContext ;
            _setGlobalID( cb, context ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to find context [%llu] of type %d[%s], "
                    "current is %d[%s]", contextID, type,
                    getContextTypeDesp( type ), tempContext->getType(),
                    getContextTypeDesp( tempContext->getType() ) ) ;
            if ( closeOnUnexpectType )
            {
               contextDelete( contextID, cb ) ;
            }
            rc = SDB_SYS ;
         }
      }

      return rc ;
   }

   BOOLEAN _SDB_RTNCB::contextExist( INT64 contextID )
   {
      return _contextMap.find( contextID ).second ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_RTNCB_CONTEXTDEL, "_SDB_RTNCB::contextDelete" )
   void _SDB_RTNCB::contextDelete ( INT64 contextID, IExecutor *pExe )
   {
      PD_TRACE_ENTRY ( SDB__SDB_RTNCB_CONTEXTDEL ) ;

      rtnContextPtr pContext ;
      pmdEDUCB *cb = ( pmdEDUCB* )pExe ;

      if ( cb )
      {
         cb->contextDelete( contextID ) ;
      }

      {
         pair<rtnContextPtr, bool> ret = _contextMap.find( contextID ) ;
         if ( ret.second )
         {
            pContext = ret.first ;
            _contextMap.erase( contextID ) ;
         }
      }

      if ( pContext )
      {
         INT32 bufRef = pContext->getReference() ;
         INT64 ctxRef = pContext.refCount() ;

         // wait for pre-fetching
         pContext->waitForPrefetch() ;

         /// wait for sync
         if ( pContext->isWrite() && pContext->getDPSCB() &&
              pContext->getW() > 1 )
         {
            if ( NULL != cb )
            {
               cb->setOrgReplSize( pContext->getW() ) ;
            }
            pContext->getDPSCB()->completeOpr( cb, pContext->getW() ) ;
         }

         monClassQuery *monQueryCB = pContext->getMonQueryCB() ;
         if ( NULL != monQueryCB )
         {
            monQueryCB->anchorToContext = FALSE ;
            // Usuaully the monQuery will get removed/archived
            // at the point when pmd processMsg ends with data
            // collected at that time.
            // But if this context is cleaned and pmd currently
            // is not processing the query this context belongs to.
            // Which also means the original query this context
            // belongs to ends unexpectedly.
            // We need to clean the monQuery.
            if ( ( NULL == cb ) ||
                 ( cb->getMonQueryCB() != monQueryCB ) )
            {
               pmdGetKRCB()->getMonMgr()->removeMonitorObject( monQueryCB ) ;
            }
            pContext->setMonQueryCB( NULL ) ;
         }
         pContext.release() ;

         PD_LOG( PDDEBUG, "delete context(contextID=%lld, reference: %u, "
                 "buffer reference: %d)", contextID, ctxRef, bufRef ) ;
      }

      PD_TRACE_EXIT ( SDB__SDB_RTNCB_CONTEXTDEL ) ;
      return ;
   }

   UINT32 _SDB_RTNCB::preDelContext( const CHAR *csName,
                                     UINT32 suLogicalID )
   {
      _RTN_EDU_CTX_MAP contexts ;

      SDB_ASSERT ( NULL != csName,
                   "collection space name should be valid" ) ;
      SDB_ASSERT ( DMS_INVALID_LOGICCSID != suLogicalID,
                   "logical ID should be valid" ) ;

      if ( 0 == _contextMap.size( TRUE ) ||
           DMS_INVALID_LOGICCSID == suLogicalID )
      {
         goto done ;
      }

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

         if ( pContext &&
              pContext->isOpened() &&
              suLogicalID == pContext->getSULogicalID() )
         {
            try
            {
               EDUID eduID = pContext->eduID() ;
               INT64 contextID = pContext->contextID () ;
               contexts.insert( make_pair( eduID, contextID ) ) ;

               PD_LOG( PDDEBUG, "Pre-deleting context [%lld] of EDU [%llu] on "
                       "collection space [%s]", contextID, eduID, csName ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save delete context, "
                       "occur exception  %s", e.what() ) ;
               // can continue
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

      _notifyKillContexts( contexts ) ;

   done:
      return contexts.size() ;
   }

   UINT32 _SDB_RTNCB::preDelExpiredContext()
   {
      _RTN_EDU_CTX_MAP contexts ;

      // config is in minutes, convert to milliseconds
      UINT64 contextTimeoutMS = _contextTimeout * 60 * OSS_ONE_SEC ;

      if ( 0 >= contextTimeoutMS ||
           0 == _contextMap.size( TRUE ) )
      {
         goto done ;
      }

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

         if ( pContext &&
              pContext->needTimeout() &&
              pContext->isOpened() &&
              pmdGetTickSpanTime( pContext->getLastProcessTick() ) >
                                                         contextTimeoutMS )
         {
            try
            {
               contexts.insert( make_pair( pContext->eduID(),
                                           pContext->contextID() ) ) ;
               PD_LOG( PDEVENT, "Pre-deleting idle timeout context [%lld] "
                       "of EDU [%llu]", pContext->contextID(),
                       pContext->eduID() ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to save delete context, "
                       "occur exception  %s", e.what() ) ;
               // can continue
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

      _notifyKillContexts( contexts ) ;

   done:
      return contexts.size() ;
   }

   INT32 _SDB_RTNCB::dumpWritingContext( RTN_CTX_PROCESS_LIST &contextProcessList,
                                         EDUID filterEDUID,
                                         UINT64 blockID )
   {
      INT32 rc = SDB_OK ;

      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         rtnContext *pContext = it->second.get() ;

         if ( pContext &&
              pContext->isOpened() &&
              pContext->isWrite() )
         {
            if ( PMD_INVALID_EDUID != filterEDUID &&
                 pContext->eduID() == filterEDUID )
            {
               continue ;
            }
            else if ( blockID > 0 &&
                      pContext->getOpID() >= blockID )
            {
               continue ;
            }
            else
            {
               const CHAR *processName = pContext->getProcessName() ;
               if ( NULL == processName || 0 == processName[ 0 ] )
               {
                  continue ;
               }
               else
               {
                  try
                  {
                     rtnCtxProcessInfo info ;
                     info._opID = pContext->getOpID() ;
                     info._ctxID = pContext->contextID() ;
                     info._eduID = pContext->eduID() ;
                     info._processName.assign( processName ) ;
                     contextProcessList.push_back( info ) ;
                  }
                  catch ( exception &e )
                  {
                     PD_LOG( PDERROR, "Failed to save context, "
                             "occur exception %s", e.what() ) ;
                     rc = ossException2RC( &e ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      FOR_EACH_CMAP_ELEMENT_END

   done:
      return rc ;
   error:
      goto done ;
   }

   void _SDB_RTNCB::_notifyKillContexts( const _RTN_EDU_CTX_MAP &contexts )
   {
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;

      for ( _RTN_EDU_CTX_MAP::const_iterator iter = contexts.begin() ;
            iter != contexts.end() ;
            ++ iter )
      {
         try
         {
            pEDUMgr->postEDUPost( iter->first,
                                  PMD_EDU_EVENT_KILLCONTEXT,
                                  PMD_EDU_MEM_NONE,
                                  NULL,
                                  (UINT64)( iter->second ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to post event to EDU [%llu], "
                    "occur exception %s", iter->first, e.what() ) ;
            // can continue
         }

         PD_LOG( PDDEBUG, "post kill context [%lld] to EDU [%llu]",
                 iter->second, iter->first ) ;
      }
   }

   INT32 _SDB_RTNCB::contextNew( RTN_CONTEXT_TYPE type,
                                 rtnContextPtr &context,
                                 INT64 &contextID,
                                 _pmdEDUCB * pEDUCB )
   {
      monSvcTaskInfo *pTaskInfo = NULL ;

      if ( pEDUCB->isFromLocal() )
      {
         // WARNING: the check may fail when context flooding ( too many
         //          context are creating in the same time )
         if ( _maxContextNum > 0 &&
              _contextMap.size( FALSE ) >= (UINT32)( _maxContextNum ) )
         {
            PD_LOG_MSG( PDERROR, "the number of contexts exceeds the limit "
                        "[%s:%d]", PMD_OPTION_MAXCONTEXTNUM, _maxContextNum ) ;
            return SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT ;
         }

         if ( _maxSessionContextNum > 0 &&
              pEDUCB->contextNum() >= (UINT32)( _maxSessionContextNum ) )
         {
            PD_LOG_MSG( PDERROR, "the number of contexts in the session "
                        "exceeds the limit [%s:%d]",
                        PMD_OPTION_MAXSESSIONCONTEXTNUM, _maxSessionContextNum ) ;
            return SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT ;
         }
      }

      // if hit max signed 64 bit integer?
      if ( _contextIdGenerator.fetch() < 0 )
      {
         return SDB_SYS ;
      }

      INT64 _contextId = _contextIdGenerator.inc() ;
      if ( _contextId < 0 )
      {
         return SDB_SYS ;
      }

      context = sdbGetRTNContextBuilder()->create(
                     type, _contextId, pEDUCB->getID() ) ;

      if ( !context )
      {
         return SDB_OOM ;
      }

      if ( !( _contextMap.insert( _contextId, context ).second ) )
      {
         context.release() ;
         return SDB_OOM ;
      }

      if ( !pEDUCB->contextInsert( _contextId ) )
      {
         _contextMap.erase( _contextId ) ;
         context.release() ;
         return SDB_OOM ;
      }

      contextID = _contextId ;

      pTaskInfo = pEDUCB->getMonAppCB()->getSvcTaskInfo() ;
      if ( pTaskInfo )
      {
         pTaskInfo->monContextInc( 1 ) ;
      }

      // Anchor the monQuery on the first context that gets created in a query
      monClassQuery *monQuery = pEDUCB->getMonQueryCB() ;
      if ( NULL != monQuery &&
           !monQuery->anchorToContext )
      {
         context->setMonQueryCB( monQuery ) ;
         monQuery->anchorToContext = TRUE ;
      }

      if ( pEDUCB->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }
      context->setOpID( pEDUCB->getWritingID() ) ;

      // only check timeout for contexts from local service
      if ( !pEDUCB->isFromLocal() )
      {
         context->disableTimeout() ;
      }

      context->_setGlobalID( pEDUCB->getOperator()->getGlobalID() ) ;

      PD_LOG ( PDDEBUG, "Create new context(contextID=%lld, type: %d[%s], "
               "writing ID %llu)",
               contextID, type, getContextTypeDesp(type),
               context->getOpID() ) ;

      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::prepareRemoteMessenger()
   {
      INT32 rc = SDB_OK ;

      // Remote messenger should be enabled on data node to support text search.
      if ( SDB_ROLE_DATA == pmdGetDBRole() )
      {
         _remoteMessenger = SDB_OSS_NEW rtnRemoteMessenger() ;
         if ( !_remoteMessenger )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for remote messenger failed, "
                    "size[ %d ]", sizeof( rtnRemoteMessenger ) ) ;
            goto error ;
         }
         rc = _remoteMessenger->init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize remote messenger, "
                      "rc: %d", rc ) ;
         rc = _remoteMessenger->active() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to active remote messenger, "
                                   "rc: %d", rc) ;
      }

   done:
      return rc ;
   error:
      if ( _remoteMessenger )
      {
         SDB_OSS_DEL _remoteMessenger ;
         _remoteMessenger = NULL ;
      }
      goto done ;
   }

   INT32 _SDB_RTNCB::addUnloadCS( const CHAR* csName )
   {
      ossScopedLock lock( &_csLatch, EXCLUSIVE ) ;
      try
      {
         _unloadCSSet.insert( csName ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return ossException2RC( &e ) ;
      }
      return SDB_OK ;
   }

   void _SDB_RTNCB::delUnloadCS( const CHAR* csName )
   {
      ossScopedLock lock( &_csLatch, EXCLUSIVE ) ;
      _unloadCSSet.erase( csName ) ;
   }

   BOOLEAN _SDB_RTNCB::hasUnloadCS( const CHAR* csName )
   {
      BOOLEAN has = FALSE ;

      if ( _unloadCSSet.size() != 0 )
      {
         ossScopedLock lock( &_csLatch, SHARED ) ;
         if ( _unloadCSSet.count( csName ) != 0 )
         {
            has = TRUE ;
         }
      }

      return has ;
   }

   /*
      get global rtn cb
   */
   SDB_RTNCB* sdbGetRTNCB ()
   {
      static SDB_RTNCB s_rtnCB ;
      return &s_rtnCB ;
   }

}

