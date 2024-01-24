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

   Source File Name = pmdRestSvc.cpp

   Descriptive Name = Process MoDel HTTP Listener ( REST requests )

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for HTTP
   Listener.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossSocket.hpp"
#include "../omsvc/omRestSession.hpp"
#include "pmdRestSession.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdProcessor.hpp"

namespace engine
{

   /*
      rest service entry point
   */
   INT32 pmdRestSvcEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc                = SDB_OK ;
      pmdKRCB *krcb           = pmdGetKRCB() ;
      pmdOptionsCB *optionCB  = krcb->getOptionCB() ;
      monDBCB *mondbcb        = krcb->getMonDBCB () ;
      pmdEDUMgr *eduMgr       = cb->getEDUMgr() ;
      ossSocket *pListerner   = ( ossSocket* )pData ;
      EDUID agentEDU          = PMD_INVALID_EDUID ;

      if ( SDB_OK != ( rc = eduMgr->activateEDU ( cb )) )
      {
         goto error ;
      }

      while ( !cb->isDisconnected() && !pListerner->isClosed() )
      {
         SOCKET s ;
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
            PD_LOG ( PDERROR, "Failed to accept rest socket, rc: %d",
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

         mondbcb->connInc();
         if ( mondbcb->isConnLimited( optionCB->getMaxConn() ) )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            continue ;
         }

         // now we have a tcp socket for a new connection, let's get an agent
         // Note the new new socket sent passing to startEDU
         rc = eduMgr->startEDU ( EDU_TYPE_RESTAGENT, pData, &agentEDU ) ;

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
      } //while ( ! cb->isDisconnected() )

   done :
      return rc ;
   error :
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_RESTLISTENER, TRUE,
                          pmdRestSvcEntryPoint,
                          "RestListener" ) ;

   /*
      rest agent entry point
   */
   INT32 pmdRestAgentEntryPoint( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;

      SOCKET s = *(( SOCKET *) &pData ) ;

      // Add try-catch to ensure the processor can be detached successfully 
      if ( SDB_ROLE_OM == pmdGetDBRole() )
      {
         _omRestSession omRS( s ) ;
         omRS.attach( cb ) ;

         _pmdDataProcessor processor ;
         omRS.attachProcessor( &processor ) ;
         try
         {
            rc = omRS.run() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "rest session occured exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
         }
         omRS.detachProcessor() ;

         omRS.detach() ;
      }
      else if ( SDB_ROLE_COORD == pmdGetDBRole() )
      {
         pmdRestSession restSession( s ) ;
         restSession.attach( cb ) ;

         _pmdCoordProcessor processor ;
         restSession.attachProcessor( &processor ) ;
         try
         {
            rc = restSession.run() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "rest session occured exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
         }
         restSession.detachProcessor() ;

         restSession.detach() ;
      }
      else
      {
         pmdRestSession restSession( s ) ;
         restSession.attach( cb ) ;

         _pmdDataProcessor processor ;
         restSession.attachProcessor( &processor ) ;
         try
         {
            rc = restSession.run() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "rest session occured exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
         }
         restSession.detachProcessor() ;

         restSession.detach() ;
      }
      
      pmdGetKRCB()->getMonDBCB ()->connDec();

      return rc ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_RESTAGENT, FALSE,
                          pmdRestAgentEntryPoint,
                          "RestAgent" ) ;

}

