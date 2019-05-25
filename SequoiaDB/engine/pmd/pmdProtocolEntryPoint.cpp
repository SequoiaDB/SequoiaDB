/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
      IPmdAccessProtocol *protocol = param->protocol ;
      SDB_OSS_DEL param ;
      param = NULL ;

      if ( SDB_OK != ( rc = eduMgr->activateEDU ( cb ) ) )
      {
         goto error ;
      }

      while ( ! cb->isDisconnected() )
      {
         SOCKET s ;
         rc = pListerner->accept ( &s, NULL, NULL ) ;
         if ( SDB_TIMEOUT == rc || SDB_TOO_MANY_OPEN_FD == rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         if ( rc && PMD_IS_DB_DOWN() )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( rc )
         {
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

         rc = eduMgr->startEDU ( EDU_TYPE_FAPAGENT, (void *)pParam,
                                 &agentEDU ) ;
         if ( rc )
         {
            PD_LOG( ( rc == SDB_QUIESCED ? PDWARNING : PDERROR ),
                      "Failed to start edu, rc: %d", rc ) ;

            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            SDB_OSS_DEL pParam ;
            pParam = NULL ;
            continue ;
         }
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
      SDB_OSS_DEL pParam ;
      pParam = NULL ;

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

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_FAPAGENT, FALSE,
                          pmdFapAgentEntryPoint,
                          "FAPAgent" ) ;


}
