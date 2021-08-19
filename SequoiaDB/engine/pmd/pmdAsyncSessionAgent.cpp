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

   Source File Name = pmdAsyncSessionAgent.cpp

   Descriptive Name = Process MoDel Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdAsyncSession.hpp"
#include "pmd.hpp"
#include "pmdEntryPoint.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

#if defined ( SDB_ENGINE )
#include "rtn.hpp"
#endif // SDB_ENGINE

namespace engine
{

   /*
      pmdAsyncSessionScope define
   */
   class pmdAsyncSessionScope
   {
      public:
         pmdAsyncSessionScope( _pmdAsyncSession *pSession,
                               pmdEDUCB *cb )
         {
            _pSession = pSession ;
            _pSession->attachIn( cb ) ;
         }
         ~pmdAsyncSessionScope()
         {
            _pSession->attachOut() ;
         }
      private:
         _pmdAsyncSession     *_pSession ;
   } ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDSYNCSESSIONAGENTEP, "pmdAsyncSessionAgentEntryPoint" )
   INT32 pmdAsyncSessionAgentEntryPoint ( pmdEDUCB * cb, void * pData )
   {
      PD_TRACE_ENTRY ( SDB_PMDSYNCSESSIONAGENTEP );
      _pmdAsyncSession * pSession = (_pmdAsyncSession*)pData ;
      pmdEDUEvent event ;
      pmdBuffInfo *pBuffInfo = NULL ;
      MsgHeader *pMsg = NULL ;
      INT64 timeDiff = 0 ;
      pmdKRCB *krcb    = pmdGetKRCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      NET_HANDLE netHandle = 0 ;
      UINT32 poolType = 0 ;

      pmdAsyncSessionScope assitScope( pSession, cb ) ;

      while ( TRUE )
      {
         if ( !cb->isDisconnected() )
         {
            cb->resetInterrupt() ;
         }
         else
         {
            pSession->close() ;
         }
         cb->resetInfo( EDU_INFO_ERROR ) ;
         cb->resetLsn() ;

         if ( cb->waitEvent( event, OSS_ONE_SEC, TRUE ) )
         {
            /// update trans should here
            cb->updateTransConf() ;
            /// reset again to avoid set interrupt self
            if ( !cb->isDisconnected() )
            {
               cb->resetInterrupt() ;
            }
            if ( PMD_EDU_EVENT_TERM == event._eventType )
            {
               PD_LOG ( PDDEBUG, "EDU[%lld, %s] is terminated", cb->getID(),
                        getEDUName( cb->getType() ) ) ;
            }
            //Dispatch event msg to session
            else if ( PMD_EDU_EVENT_MSG == event._eventType )
            {
               mondbcb->addReceiveNum() ;

               PMD_UNMAKE_SESSION_USERDATA( event._userData,
                                            netHandle,
                                            poolType ) ;

               if ( PMD_SESSION_MSG_INPOOL == poolType )
               {
                  pBuffInfo = ( pmdBuffInfo* )( event._Data ) ;
                  pMsg = ( MsgHeader* )( pBuffInfo->pBuffer ) ;

                  timeDiff = (INT64)( ossGetCurrentMicroseconds() -
                                      pBuffInfo->addTime ) ;
               }
               else
               {
                  pBuffInfo = NULL ;
                  pMsg = ( MsgHeader* )event._Data ;
                  timeDiff = 0 ;
               }

               // if msg in the buff time over 2 seconds
               if ( timeDiff > 2000000 )
               {
                  PD_LOG( PDINFO, "Session[%s] msg[opCode:[%d]%d, requestID: "
                          "%lld, TID: %d, Len: %d] stay over %lld usecs",
                          pSession->sessionName(), IS_REPLY_TYPE(pMsg->opCode),
                          GET_REQUEST_TYPE(pMsg->opCode), pMsg->requestID,
                          pMsg->TID, pMsg->messageLength, timeDiff ) ;
               }

               pSession->onDispatchMsgBegin( netHandle, pMsg ) ;
               pSession->dispatchMsg ( netHandle, pMsg, &timeDiff ) ;
               pSession->onDispatchMsgEnd( timeDiff ) ;

               // if msg processed time over 20 seconds
               if ( timeDiff > 20000000 )
               {
                  PD_LOG( PDINFO, "Session[%s] msg[opCode:[%d]%d, requestID: "
                          "%lld, TID: %d, Len: %d] processed over %lld usecs",
                          pSession->sessionName(), IS_REPLY_TYPE(pMsg->opCode),
                          GET_REQUEST_TYPE(pMsg->opCode), pMsg->requestID,
                          pMsg->TID, pMsg->messageLength, timeDiff ) ;
               }

               if ( pBuffInfo )
               {
                  pBuffInfo->setFree () ;
               }
            }
            else
            {
               pSession->dispatch ( &event ) ;
            }

            //Relase memory
            pmdEduEventRelease( event, cb ) ;
            event.reset () ;
         }
         else if ( !cb->isDisconnected() )
         {
            pSession->onTimer( 0, OSS_ONE_SEC ) ;
         }
         else
         {
            break ;
         }
      }

      PD_TRACE_EXIT ( SDB_PMDSYNCSESSIONAGENTEP );
      return SDB_OK ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SHARDAGENT, FALSE,
                          pmdAsyncSessionAgentEntryPoint,
                          "ShardAgent" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_REPLAGENT, FALSE,
                          pmdAsyncSessionAgentEntryPoint,
                          "ReplAgent" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_OMAAGENT, FALSE,
                          pmdAsyncSessionAgentEntryPoint,
                          "OMAAgent" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_INDEX, FALSE,
                          pmdAsyncSessionAgentEntryPoint,
                          "SeIndexAgent" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_AGENT, FALSE,
                          pmdAsyncSessionAgentEntryPoint,
                          "SeAgent" ) ;

}

