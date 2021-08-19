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

   Source File Name = pmdProtocolEntryPoint.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include "pd.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossSocket.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdProcessor.hpp"
#include "pmdAccessProtocolBase.hpp"
#include "pmdModuleLoader.hpp"

namespace engine
{

   INT32 pmdFapListenerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDTCPLSTNENTPNT ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdOptionsCB *optionCB = krcb->getOptionCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      EDUID agentEDU = PMD_INVALID_EDUID ;

      pmdEDUParam *param = ( pmdEDUParam * )pData ;
      ossSocket *pListerner = (ossSocket *)(param->pSocket) ;
      // reserved protocol
      IPmdAccessProtocol *protocol = param->protocol ;
      // delete pmdEDUParam object
      SDB_OSS_DEL param ;
      param = NULL ;

      // let's set the state of EDU to RUNNING
      if ( SDB_OK != ( rc = eduMgr->activateEDU ( cb ) ) )
      {
         goto error ;
      }

      // master loop for tcp listener
      while ( ! cb->isDisconnected() )
      {
         SOCKET s ;
         // timeout in 10ms, so we won't hold global bind latch for too long
         // and it's only held at first time into the loop
         rc = pListerner->accept ( &s, NULL, NULL ) ;
         // if we don't get anything for a period of time, let's loop
         if ( SDB_TIMEOUT == rc || SDB_TOO_MANY_OPEN_FD == rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         // if we receive error due to database down, we finish
         if ( rc && PMD_IS_DB_DOWN() )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( rc )
         {
            // if we fail due to error, let's restart socket
            PD_LOG ( PDERROR, "Failed to accept socket in TcpListener(rc=%d)",
                     rc ) ;
            if ( pListerner->isClosed() )
            {
               break ;
            }
            else
            {
               continue ;
            }
         }

         cb->incEventCount() ;

         pmdEDUParam *pParam = SDB_OSS_NEW pmdEDUParam() ;
         // assign the socket to the pProtocolData
         *(( SOCKET *)&pParam->pSocket) = s ;
         pParam->protocol = protocol ;

         if ( !krcb->isActive() )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;

            SDB_OSS_DEL pParam ;
            pParam = NULL ;
            continue ;
         }

         mondbcb->connInc() ;
         if ( mondbcb->isConnLimited( optionCB->getMaxConn() ) )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            continue ;
         }

         // now we have a tcp socket for a new connection, let's get an 
         // agent, Note the new new socket sent passing to startEDU
         rc = eduMgr->startEDU ( EDU_TYPE_FAPAGENT, (void *)pParam,
                                 &agentEDU ) ;
         if ( rc )
         {
            PD_LOG( ( rc == SDB_QUIESCED ? PDWARNING : PDERROR ),
                      "Failed to start edu, rc: %d", rc ) ;

            // close remote connection if we can't create new thread
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            SDB_OSS_DEL pParam ;
            pParam = NULL ;
            continue ;
         }
         // Now EDU is started and posted with the new socket, let's
         // get back to wait for another request
      } //while ( ! cb->isDisconnected() )

      if ( SDB_OK != ( rc = eduMgr->waitEDU ( cb ) ) )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_PMDTCPLSTNENTPNT, rc );
      return rc;

   error :
      switch ( rc )
      {
      case SDB_SYS :
         PD_LOG ( PDSEVERE, "System error occured" ) ;
         break ;
      default :
         PD_LOG ( PDSEVERE, "Internal error" ) ;
         break ;
      }
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_FAPLISTENER, TRUE,
                          pmdFapListenerEntryPoint,
                          "FAPListener" ) ;

   INT32 pmdFapAgentEntryPoint( pmdEDUCB *cb, void *arg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOCALAGENTENTPNT ) ;
      pmdSession *session = NULL ;
      pmdEDUParam *pParam = ( pmdEDUParam * )arg ;
      SOCKET s = *((SOCKET *)&pParam->pSocket) ;
      IPmdAccessProtocol* protocol = pParam->protocol ;
      // delete pmdEDUParam object
      SDB_OSS_DEL pParam ;
      pParam = NULL ;

      /// get session
      session = protocol->getSession( s ) ;
      if ( NULL == session )
      {
         PD_LOG( PDERROR, "Failed to get Session of protocol" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      session->attach( cb ) ;

      if ( pmdGetDBRole() == SDB_ROLE_COORD )
      {
         pmdCoordProcessor coordProcessor ;
         session->attachProcessor( &coordProcessor ) ;
         rc = session->run() ;
         session->detachProcessor() ;
      }
      else
      {
         pmdDataProcessor dataProcessor ;
         session->attachProcessor( &dataProcessor ) ;
         rc = session->run() ;
         session->detachProcessor() ;
      }
      session->detach() ;

   done:
      if ( session )
      {
         protocol->releaseSession( session ) ;
         session = NULL ;
      }

      pmdGetKRCB()->getMonDBCB ()->connDec();
      
      PD_TRACE_EXITRC ( SDB_PMDLOCALAGENTENTPNT, rc );
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_FAPAGENT, FALSE,
                          pmdFapAgentEntryPoint,
                          "FAPAgent" ) ;


}
