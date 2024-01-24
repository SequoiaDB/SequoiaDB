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

   Source File Name = pmdTcpListener.cpp

   Descriptive Name = Process MoDel TCP Listener

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for TCP
   Listener.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <stdio.h>
#include "pd.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossSocket.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDTCPLSTNENTPNT, "pmdTcpListenerEntryPoint" )
   INT32 pmdTcpListenerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDTCPLSTNENTPNT ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdOptionsCB *optionCB = krcb->getOptionCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      EDUID agentEDU = PMD_INVALID_EDUID ;
      ossSocket *pListerner = ( ossSocket* )pData ;

      // let's set the state of EDU to RUNNING
      if ( SDB_OK != ( rc = eduMgr->activateEDU ( cb ) ) )
      {
         goto error ;
      }

      // master loop for tcp listener
      while ( !cb->isDisconnected() && !pListerner->isClosed() )
      {
         SOCKET s ;
         // timeout in 10ms, so we won't hold global bind latch for too long
         // and it's only held at first time into the loop
         rc = pListerner->accept ( &s, NULL, NULL ) ;
         // if we don't get anything for a period of time, let's loop
         if ( SDB_TIMEOUT == rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         else if ( SDB_TOO_MANY_OPEN_FD == rc )
         {
            pListerner->close() ;
            PD_LOG( PDERROR, "Can not accept more connections because of "
                    "open files upto limits, restart listening" ) ;
            pmdIncErrNum( rc ) ;

            while( !cb->isDisconnected() )
            {
               pListerner->close() ;
               ossSleep( 2 * OSS_ONE_SEC ) ;
               rc = pListerner->initSocket() ;
               if ( rc )
               {
                  continue ;
               }
               rc = pListerner->bind_listen() ;
               if ( rc )
               {
                  continue ;
               }
               PD_LOG( PDEVENT, "Restart listening on port[%d] succeed",
                       pListerner->getLocalPort() ) ;
               break ;
            }
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

         // assign the socket to the arg
         void *pData = NULL ;
         *((SOCKET *) &pData) = s ;

         if ( !krcb->isActive() )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;
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
         rc = eduMgr->startEDU ( EDU_TYPE_AGENT, pData, &agentEDU ) ;
         if ( rc )
         {
            PD_LOG( ( rc == SDB_QUIESCED ? PDWARNING : PDERROR ),
                    "Failed to start edu, rc: %d", rc ) ;

            // close remote connection if we can't create new thread
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
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
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_TCPLISTENER, TRUE,
                          pmdTcpListenerEntryPoint,
                          "TCPListener" ) ;

}

