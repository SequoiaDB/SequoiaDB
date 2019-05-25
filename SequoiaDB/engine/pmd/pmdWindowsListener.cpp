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

   Source File Name = pmdWindowsListener.cpp

   Descriptive Name = Process MoDel Windows Listener

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains entry point for local listener
   that only avaliable on Windows.

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
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossUtil.hpp"
#include "ossNPipe.hpp"
#include "utilNodeOpr.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   #define PMD_WL_NPIPE_BUFSZ          1024

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPELSTNNPNTPNT, "pmdPipeListenerEntryPoint" )
   INT32 pmdPipeListenerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDPIPELSTNNPNTPNT );
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      CHAR tempBuffer [ PMD_WL_NPIPE_BUFSZ + 1 ] = {0} ;
      const CHAR *pSvcName = ( const CHAR* )pData ;
      INT32 role = pmdGetDBRole() ;
      INT32 type = pmdGetDBType() ;
      utilNodePipe nodePipe ;
      BOOLEAN hasClosedStdFds = FALSE ;

      INT32 hasRead = 0 ;
      rc = eduMgr->activateEDU ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to activate EDU, rc: %d", rc ) ;
         goto error ;
      }

      rc = nodePipe.createPipe( pSvcName ) ;
      if ( rc )
      {
         PD_LOG ( PDSEVERE, "Failed to create named pipe: %s, rc = %d",
                  nodePipe.getReadPipeName(), rc ) ;
         goto error ;
      }

      while ( !cb->isDisconnected() )
      {
         rc = nodePipe.connectPipe() ;
         if ( rc )
         {
            if ( SDB_TIMEOUT == rc )
            {
               continue ;
            }
            else if ( SDB_TOO_MANY_OPEN_FD == rc )
            {
               ossSleep( OSS_ONE_SEC ) ;
               continue ;
            }
            PD_LOG ( PDSEVERE, "Failed to connect named pipe: %s, rc = %d",
                     nodePipe.getReadPipeName(), rc ) ;
            goto error ;
         }

         hasRead = 0 ;
         while ( 0 == hasRead && !cb->isDisconnected() )
         {
            rc = nodePipe.readPipe( tempBuffer, PMD_WL_NPIPE_BUFSZ, hasRead ) ;
            if ( rc )
            {
               if ( SDB_TIMEOUT == rc )
                  continue ;
               PD_LOG ( PDERROR, "Failed to read packet, rc = %d", rc ) ;
               hasRead = 0 ;
               rc = SDB_OK ;
               break ;
            }
         }

         if ( hasRead > 0 )
         {
            PD_LOG ( PDINFO, "Received message from windows listener: %s, "
                     "size: %d", tempBuffer, hasRead ) ;
            if ( ossStrncmp ( tempBuffer, ENGINE_NPIPE_MSG_SHUTDOWN,
                              sizeof(ENGINE_NPIPE_MSG_SHUTDOWN) ) == 0 )
            {
               PD_LOG ( PDEVENT, "Shutdown message is received" ) ;
               PMD_SHUTDOWN_DB( SDB_OK ) ;
            }
            else if ( ossStrncmp ( tempBuffer, ENGINE_NPIPE_MSG_PID,
                                   sizeof(ENGINE_NPIPE_MSG_PID) ) == 0 )
            {
               OSSPID currentProcessPID = ossGetCurrentProcessID () ;
               rc = nodePipe.writePipe( (CHAR*)&currentProcessPID,
                                        sizeof(currentProcessPID) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_TYPE,
                                       sizeof( ENGINE_NPIPE_MSG_TYPE ) ) )
            {
               rc = nodePipe.writePipe( (const CHAR*)&type, sizeof(type) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_ROLE,
                                       sizeof( ENGINE_NPIPE_MSG_ROLE ) ) )
            {
               rc = nodePipe.writePipe( (const CHAR*)&role, sizeof(role) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_GID,
                                       sizeof( ENGINE_NPIPE_MSG_GID ) ) )
            {
               INT32 gid = pmdGetNodeID().columns.groupID ;
               rc = nodePipe.writePipe( (const CHAR*)&gid, sizeof(gid) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_NID,
                                       sizeof( ENGINE_NPIPE_MSG_NID ) ) )
            {
               INT32 nid = pmdGetNodeID().columns.nodeID ;
               rc = nodePipe.writePipe( (const CHAR*)&nid, sizeof(nid) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_GNAME,
                                       sizeof( ENGINE_NPIPE_MSG_GNAME ) ) )
            {
               const CHAR *gname = pmdGetKRCB()->getGroupName() ;
               rc = nodePipe.writePipe( gname, ossStrlen( gname ) + 1 ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_PATH,
                                       sizeof( ENGINE_NPIPE_MSG_PATH ) ) )
            {
               const CHAR *path = pmdGetOptionCB()->getDbPath() ;
               rc = nodePipe.writePipe( path, ossStrlen( path ) + 1 ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_PRIMARY,
                                       sizeof( ENGINE_NPIPE_MSG_PRIMARY ) ) )
            {
               INT32 primary = pmdIsPrimary() ? 1 : 0 ;
               rc = nodePipe.writePipe( (const CHAR*)&primary,
                                        sizeof(primary) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_ENDPIPE,
                                       sizeof( ENGINE_NPIPE_MSG_ENDPIPE ) ) )
            {
               INT32 result = SDB_OK ;
               if ( !hasClosedStdFds )
               {
#if defined( _LINUX )
                  ossCloseStdFds() ;
#endif // _LINUX
                  hasClosedStdFds = TRUE ;
               }
               rc = nodePipe.writePipe( ( const CHAR * )&result,
                                        sizeof( result ) ) ;
            }
            else if ( 0 == ossStrncmp( tempBuffer, ENGINE_NPIPE_MSG_STARTTIME,
                                       sizeof( ENGINE_NPIPE_MSG_STARTTIME ) ) )
            {
               UINT64 startTime = pmdGetStartTime() ;
               rc = nodePipe.writePipe( (const CHAR*)&startTime,
                                        sizeof( UINT64 ) ) ;
            }

            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to write %s to named pipe, "
                        "rc = %d", tempBuffer, rc ) ;
            }
         }

         nodePipe.disconnectPipe() ;
      }

   done :
      nodePipe.autoRelease() ;
      PD_TRACE_EXITRC ( SDB_PMDPIPELSTNNPNTPNT, rc );
      return rc;
   error :
      switch ( rc )
      {
      case SDB_SYS :
         PD_LOG ( PDSEVERE, "System error occured" ) ;
         break ;
      default :
         PD_LOG ( PDSEVERE, "Internal error" ) ;
      }
      PD_LOG ( PDSEVERE, "Shutdown database" ) ;
      PMD_SHUTDOWN_DB( rc ) ;
      goto done ;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_PIPESLISTENER, TRUE,
                          pmdPipeListenerEntryPoint,
                          "PipeListener" ) ;
}


