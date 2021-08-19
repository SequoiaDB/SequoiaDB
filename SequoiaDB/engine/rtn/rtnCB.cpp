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
#include "rtnIxmKeySorter.hpp"
#include "rtnBackgroundJob.hpp"

#include "pmdController.hpp"

using namespace std;
namespace engine
{
   _SDB_RTNCB::_SDB_RTNCB()
      : _contextIdGenerator( 0 ),
        _remoteMessenger( NULL ),
        _textIdxVersion((INT64)RTN_INIT_TEXT_INDEX_VERSION)
   {
      _pLTMgr = NULL ;
   }

   _SDB_RTNCB::~_SDB_RTNCB()
   {
      FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
      {
         SDB_OSS_DEL ((*it).second) ;
      }
      FOR_EACH_CMAP_ELEMENT_END ;

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

      rtnIxmKeySorterCreator* creator = SDB_OSS_NEW _rtnIxmKeySorterCreator() ;
      if ( NULL == creator )
      {
         PD_LOG ( PDERROR, "failed to create _rtnIxmKeySorterCreator" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _pLTMgr = SDB_OSS_NEW rtnLocalTaskMgr() ;
      if ( !_pLTMgr )
      {
         PD_LOG( PDERROR, "Failed to create local task manager" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      sdbGetDMSCB()->setIxmKeySorterCreator( creator ) ;

      // The error of initialization of APM could be ignore
      // Only data and catalog nodes could initialize plan cache
      _accessPlanManager.init(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ||
              SDB_ROLE_OM == pmdGetDBRole() ) ?
             pmdGetOptionCB()->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL) pmdGetOptionCB()->getPlanCacheLevel(),
            pmdGetOptionCB()->getSortBufSize(),
            pmdGetOptionCB()->getOptCostThreshold(),
            pmdGetOptionCB()->isEnabledMixCmp() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_RTNCB::active ()
   {
      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::deactive ()
   {
      if ( _remoteMessenger )
      {
         _remoteMessenger->deactive() ;
      }
      return SDB_OK ;
   }

   INT32 _SDB_RTNCB::fini ()
   {
      _accessPlanManager.fini() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      dmsIxmKeySorterCreator* creator = sdbGetDMSCB()->getIxmKeySorterCreator() ;
      if ( NULL != creator )
      {
         SDB_OSS_DEL( creator ) ;
         sdbGetDMSCB()->setIxmKeySorterCreator( NULL ) ;
      }

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

      return SDB_OK ;
   }

   void _SDB_RTNCB::onConfigChange ()
   {
      _accessPlanManager.reinit(
            ( SDB_ROLE_DATA == pmdGetDBRole() ||
              SDB_ROLE_CATALOG == pmdGetDBRole() ||
              SDB_ROLE_STANDALONE == pmdGetDBRole() ) ?
            pmdGetOptionCB()->getPlanBuckets() : 0,
            (OPT_PLAN_CACHE_LEVEL) pmdGetOptionCB()->getPlanCacheLevel(),
            pmdGetOptionCB()->getSortBufSize(),
            pmdGetOptionCB()->getOptCostThreshold(),
            pmdGetOptionCB()->isEnabledMixCmp() ) ;
   }

   rtnContext* _SDB_RTNCB::contextFind ( SINT64 contextID, _pmdEDUCB *cb )
   {
      rtnContext *pContext = NULL ;
      std::pair<rtnContext*, bool> ret = _contextMap.find( contextID ) ;
      if ( ret.second )
      {
         if ( cb && !cb->contextFind( contextID ) )
         {
            PD_LOG ( PDWARNING, "Context %lld does not owned by "
                     "current session", contextID ) ;
         }
         else
         {
            pContext = ret.first ;
         }
      }

      // This assert is to ensure that the same edu resumes a previous context
#ifdef _DEBUG
      if ( pContext && pContext->getMonQueryCB() && cb)
      {
         SDB_ASSERT( cb->getMonQueryCB() == pContext->getMonQueryCB(), "Mismatch monQuery" ) ;
      }
#endif

      return pContext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_RTNCB_CONTEXTDEL, "_SDB_RTNCB::contextDelete" )
   void _SDB_RTNCB::contextDelete ( INT64 contextID, IExecutor *pExe )
   {
      PD_TRACE_ENTRY ( SDB__SDB_RTNCB_CONTEXTDEL ) ;

      rtnContext *pContext = NULL ;
      pmdEDUCB *cb = ( pmdEDUCB* )pExe ;

      if ( cb )
      {
         cb->contextDelete( contextID ) ;
      }

      {
         pair<rtnContext*, bool> ret = _contextMap.find( contextID ) ;
         if ( ret.second )
         {
            _contextMap.erase( contextID ) ;
            pContext = ret.first ;
         }
      }

      if ( pContext )
      {
         INT32 reference = pContext->getReference() ;
         pContext->waitForPrefetch() ;

         /// wait for sync
         if ( pContext->isWrite() && pContext->getDPSCB() &&
              pContext->getW() > 1 )
         {
            pContext->getDPSCB()->completeOpr( cb, pContext->getW() ) ;
         }

         sdbGetRTNContextBuilder()->release( pContext ) ;
         PD_LOG( PDDEBUG, "delete context(contextID=%lld, reference: %d)",
                 contextID, reference ) ;
      }

      PD_TRACE_EXIT ( SDB__SDB_RTNCB_CONTEXTDEL ) ;
      return ;
   }

   SINT32 _SDB_RTNCB::contextNew ( RTN_CONTEXT_TYPE type,
                                   rtnContext **context,
                                   SINT64 &contextID,
                                   _pmdEDUCB * pEDUCB )
   {
      SDB_ASSERT ( context, "context pointer can't be NULL" ) ;
      monSvcTaskInfo *pTaskInfo = NULL ;

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

      (*context) = sdbGetRTNContextBuilder()->create(
                     type, _contextId, pEDUCB->getID() ) ;

      if ( !(*context) )
      {
         return SDB_OOM ;
      }

      _contextMap.insert( _contextId, *context ) ;
      pEDUCB->contextInsert( _contextId ) ;
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
         (*context)->setMonQueryCB( monQuery ) ;
         monQuery->anchorToContext = TRUE ;
      }

      PD_LOG ( PDDEBUG, "Create new context(contextID=%lld, type: %d[%s])",
               contextID, type, getContextTypeDesp(type) ) ;

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
      }

      rc = _remoteMessenger->active() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active remote messenger, "
                   "rc: %d", rc) ;

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

   /*
      get global rtn cb
   */
   SDB_RTNCB* sdbGetRTNCB ()
   {
      static SDB_RTNCB s_rtnCB ;
      return &s_rtnCB ;
   }

}

