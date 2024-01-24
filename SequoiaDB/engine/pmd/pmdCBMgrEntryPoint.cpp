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

   Source File Name = pmdCBMgrEntryPoint.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDUMgr.hpp"
#include "pmdObjBase.hpp"
#include "dpsTransDef.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDCBMGREP, "pmdCBMgrEntryPoint" )
   INT32 pmdCBMgrEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDCBMGREP );

      _pmdObjBase *pObj = ( _pmdObjBase* )pData ;
      pmdEDUMgr *pEDUMgr = cb->getEDUMgr() ;
      pmdEDUEvent eventData;
      INT64 timeSpan = 0 ; /// usec
      INT64 maxMsgTime = pObj->getMaxProcMsgTime() * 1000000 ;
      INT64 maxEventTime = pObj->getMaxProcEventTime() * 1000000 ;
      INT64 *pMsgTimeSpan = maxMsgTime >= 0 ? &timeSpan : NULL ;
      INT64 *pEventTimeSpan = maxEventTime >= 0 ? &timeSpan : NULL ;
      BOOLEAN attachedDummySession = FALSE ;
      pmdDummySession session ;
      IOperator *pSdbOp = NULL ;

      if ( NULL == cb->getSession() )
      {
         session.attachCB( cb ) ;
         attachedDummySession = TRUE ;
      }

      pSdbOp = session.getOperator() ;

      pObj->attachCB( cb ) ;

      rc = pEDUMgr->activateEDU( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

#if defined ( SDB_ENGINE )
      // not use transaction
      cb->getTransExecutor()->setUseTransLock( FALSE ) ;
      cb->getTransExecutor()->setTransIsolation( TRANS_ISOLATION_RU, TRUE ) ;
#endif //SDB_ENGINE

      //Wait event msg and dispatch msg
      while ( !cb->isDisconnected() )
      {
         if ( cb->waitEvent( eventData, OSS_ONE_SEC, TRUE ) )
         {
            cb->resetInterrupt() ;
            cb->resetInfo( EDU_INFO_ERROR ) ;
            cb->resetLsn() ;
            cb->updateConf() ;
            pdClearLastError() ;

            if ( PMD_EDU_EVENT_TERM == eventData._eventType )
            {
               PD_LOG ( PDDEBUG, "EDU[%lld, %s] is terminated", cb->getID(),
                        getEDUName( cb->getType() ) ) ;
            }
            //Dispatch event msg to cb manager
            else if ( PMD_EDU_EVENT_MSG == eventData._eventType )
            {
               MsgHeader *pMsg = ((MsgHeader*)(eventData._Data)) ;

               ((pmdOperator*)pSdbOp)->setMsg( pMsg ) ;

               //restore handle
               pObj->dispatchMsg( (NET_HANDLE)eventData._userData,
                                  (MsgHeader*)(eventData._Data),
                                  pMsgTimeSpan ) ;
               if ( pMsgTimeSpan && timeSpan > maxMsgTime )
               {
                  MsgHeader *pMsg = (MsgHeader*)(eventData._Data) ;
                  PD_LOG( PDWARNING, "[%s] Process msg[opCode:[%d]%d, "
                          "requestID: %lld, TID: %d, Len: %d] over %d microsecs",
                          pObj->name(), IS_REPLY_TYPE(pMsg->opCode),
                          GET_REQUEST_TYPE(pMsg->opCode), pMsg->requestID,
                          pMsg->TID, pMsg->messageLength, timeSpan ) ;
               }

               ((pmdOperator*)pSdbOp)->clearMsg() ;
            }
            else
            {
               pObj->dispatchEvent ( &eventData, pEventTimeSpan ) ;
               if ( pEventTimeSpan && timeSpan > maxEventTime )
               {
                  PD_LOG( PDWARNING, "[%s] Process event[type:%d] over %d "
                          "us", pObj->name(), eventData._eventType,
                          timeSpan ) ;
               }
            }

            //Relase memory
            pmdEduEventRelease( eventData, cb ) ;
            eventData.reset () ;
         }
         else
         {
            cb->shrink() ;
         }
      }

   done:
      pObj->detachCB( cb ) ;
      if ( attachedDummySession )
      {
         session.detachCB() ;
      }
      PD_TRACE_EXITRC ( SDB_PMDCBMGREP, rc );
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CLUSTER, TRUE,
                          pmdCBMgrEntryPoint,
                          "Cluster" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CLUSTERSHARD, TRUE,
                          pmdCBMgrEntryPoint,
                          "ClusterShard" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_COORDMGR, TRUE,
                          pmdCBMgrEntryPoint,
                          "CoordMgr" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CATMGR, TRUE,
                          pmdCBMgrEntryPoint,
                          "CatalogMgr" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_OMMGR, TRUE,
                          pmdCBMgrEntryPoint,
                          "OMManager" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SEADPTMGR, TRUE,
                          pmdCBMgrEntryPoint,
                          "SeAdapterMgr" ) ;

}

