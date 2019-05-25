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

   Source File Name = rtnRemoteExec.cpp

   Descriptive Name = Remote Excuting Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for process op.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/27/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnRemoteExec.hpp"
#include "ossSocket.hpp"
#include "ossProc.hpp"
#include "msgMessage.hpp"
#include "omagentDef.hpp"
#include "utilParam.hpp"
#include "pmdEDU.hpp"
#include "msgDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_REMOTEEXEC, "rtnRemoteExec" )
   INT32 rtnRemoteExec ( SINT32 remoCode,
                         const CHAR * hostname,
                         SINT32 *retCode,
                         const BSONObj *arg1,
                         const BSONObj *arg2,
                         const BSONObj *arg3,
                         const BSONObj *arg4,
                         std::vector<BSONObj> *retObjs )
   {
      SDB_ASSERT ( retCode && hostname , "Invalid input" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_REMOTEEXEC ) ;
      PD_TRACE1 ( SDB_REMOTEEXEC,
                  PD_PACK_INT ( remoCode ) ) ;
      CHAR *pCMRequest = NULL ;
      CHAR *pReceiveBuffer = NULL ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      const CHAR *svcname = NULL ;
      UINT16 port = SDBCM_DFT_PORT ;
      INT32 reqSize = 0 ;
      SINT32 packetLength = 0 ;

      po::options_description desc ( "Config options" ) ;
      po::variables_map vm ;
      CHAR hostname2[OSS_MAX_HOSTNAME + 6] = { 0 } ;
      if ( ossStrlen(hostname) > OSS_MAX_HOSTNAME )
      {
         PD_LOG ( PDERROR, "Invalid host name: %s", hostname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossMemcpy( hostname2, hostname, ossStrlen( hostname ) );
      ossStrncat ( hostname2, SDBCM_CONF_PORT, ossStrlen(SDBCM_CONF_PORT) ) ;

      desc.add_options()
         (SDBCM_CONF_DFTPORT, po::value<string>(), "sdbcm default "
         "listening port")
         (hostname2, po::value<string>(), "sdbcm specified listening port")
      ;

      rc = ossGetEWD ( conf, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get excutable file's working "
                  "directory" ) ;
         goto error ;
      }
      if ( ( ossStrlen ( conf ) + ossStrlen ( SDBCM_CONF_PATH_FILE ) + 2 ) >
           OSS_MAX_PATHSIZE )
      {
         PD_LOG ( PDERROR, "Working directory too long" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncat( conf, OSS_FILE_SEP, 1 );
      ossStrncat( conf, SDBCM_CONF_PATH_FILE,
                  ossStrlen( SDBCM_CONF_PATH_FILE ) );
      rc = utilReadConfigureFile ( conf, desc, vm ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read configure file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( vm.count(hostname2) )
      {
         svcname = vm[hostname2].as<string>().c_str() ;
      }
      else if ( vm.count(SDBCM_CONF_DFTPORT) )
      {
         svcname = vm[SDBCM_CONF_DFTPORT].as<string>().c_str() ;
      }
      if ( svcname != NULL )
      {
         rc = ossGetPort( svcname, port ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Bad sdbcm listening service name: %s",
                     svcname ) ;
            goto error ;
         }
      }

      {
         ossSocket sock ( hostname, port, OSS_SOCKET_DFT_TIMEOUT ) ;
         rc = sock.initSocket () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed initialize socket, rc=%d", rc ) ;
            goto error ;
         }
         rc = sock.connect () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed connect remote server[%s:%d], rc=%d",
                     hostname, port, rc ) ;
            goto error ;
         }
         rc = sock.disableNagle() ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to disable nagle, rc: %d", rc ) ;
         }
         rc = sock.setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                                 OSS_SOCKET_KEEP_INTERVAL,
                                 OSS_SOCKET_KEEP_CONTER ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to set keep alive, rc=%d", rc ) ;
         }

         rc = msgBuildCMRequest ( &pCMRequest, &reqSize, remoCode,
                                   arg1, arg2, arg3, arg4 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to build cm request message, rc=%d",
                     rc ) ;
            goto error ;
         }

         rc = pmdSend ( pCMRequest, ((MsgHeader*)pCMRequest)->messageLength,
                        &sock, pmdGetThreadEDUCB() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to send cm request message, rc=%d", rc ) ;
            goto error ;
         }

         rc = pmdRecv ( (CHAR*)&packetLength, sizeof (SINT32), &sock,
                        pmdGetThreadEDUCB() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to receive cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         else if ( (UINT32)packetLength < sizeof(MsgHeader) ||
                   (UINT32)packetLength > SDB_MAX_MSG_LENGTH )
         {
            PD_LOG( PDERROR, "Recv msg size[%d] is less than "
                    "MsgHeader size[%d] or more than max msg size[%d]",
                    packetLength, sizeof(MsgHeader), SDB_MAX_MSG_LENGTH ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pReceiveBuffer = (CHAR*)SDB_OSS_MALLOC ( packetLength + 1 ) ;
         if ( !pReceiveBuffer )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "Failed to allocate %d bytes receive buffer",
                     packetLength ) ;
            goto error ;
         }
         *(SINT32*)(pReceiveBuffer) = packetLength ;
         rc = pmdRecv ( &pReceiveBuffer[sizeof (SINT32)],
                        packetLength-sizeof (SINT32), &sock,
                        pmdGetThreadEDUCB() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to receive cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         pReceiveBuffer[ packetLength ] = 0 ;
      }

      {
         SINT64 contextID  = 0 ;
         SINT32 startFrom = 0 ;
         SINT32 numReturned = 0 ;
         vector<BSONObj> objLst ;
        
         rc = msgExtractReply ( pReceiveBuffer, retCode, &contextID,
                                &startFrom, &numReturned, objLst ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to extract cm reply message, rc=%d",
                     rc ) ;
            goto error ;
         }
         SDB_ASSERT( contextID == -1, "Context id must be invalid" ) ;

         for ( UINT32 i = 0 ; i < objLst.size() ; ++i )
         {
            PD_LOG( PDEVENT, "RemoteExec recv obj: %s",
                    objLst[i].toString().c_str() ) ;
            if ( NULL != retObjs )
            {
               retObjs->push_back( objLst[i].copy() ) ;
            }
         }
      }

   done:
      if ( pCMRequest )
      {
         SDB_OSS_FREE ( pCMRequest ) ;
      }
      if ( pReceiveBuffer )
      {
         SDB_OSS_FREE ( pReceiveBuffer ) ;
      }
      PD_TRACE_EXITRC ( SDB_REMOTEEXEC, rc ) ;
      return rc ;
   error:
      if ( NULL != retObjs )
      {
         retObjs->clear() ;
      }
      goto done ;
   }

}


