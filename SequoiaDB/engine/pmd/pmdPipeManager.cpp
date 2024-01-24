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

   Source File Name = pmdPipeManager.hpp

   Descriptive Name = Process MoDel Pipe Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for pipe
   manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdPipeManager.hpp"
#include "pmd.hpp"
#include "pmdDef.hpp"
#include "pmdEnv.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   /*
      _pmdSystemPipeHandler implement
    */
   _pmdSystemPipeHandler::_pmdSystemPipeHandler()
   : _closedStdFds( FALSE )
   {
   }

   _pmdSystemPipeHandler::~_pmdSystemPipeHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDSYSTEMPIPEHANDLER_PROCESSMSG, "_pmdSystemPipeHandler::processMessage" )
   INT32 _pmdSystemPipeHandler::processMessage( CHAR *message,
                                                utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDSYSTEMPIPEHANDLER_PROCESSMSG ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_SHUTDOWN,
                            sizeof( ENGINE_NPIPE_MSG_SHUTDOWN ) ) )
      {
         PD_LOG ( PDEVENT, "Shutdown message is received" ) ;
         PMD_SHUTDOWN_DB( SDB_OK ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_ENDPIPE,
                                 sizeof( ENGINE_NPIPE_MSG_ENDPIPE ) ) )
      {
         INT32 result = SDB_OK ;
         if ( !_closedStdFds )
         {
#if defined( _LINUX )
            ossCloseStdFds() ;
#endif // _LINUX
            _closedStdFds = TRUE ;
         }
         rc = nodePipe.writePipe( (const CHAR *)( &result),
                                  sizeof( result ) ) ;
      }
      else
      {
         rc = SDB_UNKNOWN_MESSAGE ;
      }

      PD_TRACE_EXITRC( SDB_PMDSYSTEMPIPEHANDLER_PROCESSMSG, rc ) ;

      return rc ;
   }

   /*
      _pmdInfoPipeHandler implement
    */
   _pmdInfoPipeHandler::_pmdInfoPipeHandler()
   {
   }

   _pmdInfoPipeHandler::~_pmdInfoPipeHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDINFOPIPEHANDLER_PROCESSMSG, "_pmdInfoPipeHandler::processMessage" )
   INT32 _pmdInfoPipeHandler::processMessage( CHAR *message,
                                              utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDINFOPIPEHANDLER_PROCESSMSG ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_PID,
                            sizeof( ENGINE_NPIPE_MSG_PID ) ) )
      {
         OSSPID currentProcessPID = ossGetCurrentProcessID () ;
         rc = nodePipe.writePipe( (CHAR*)( &currentProcessPID ),
                                  sizeof( currentProcessPID ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_TYPE,
                                 sizeof( ENGINE_NPIPE_MSG_TYPE ) ) )
      {
         INT32 type = pmdGetDBType() ;
         rc = nodePipe.writePipe( (const CHAR*)( &type ), sizeof( type ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_ROLE,
                                 sizeof( ENGINE_NPIPE_MSG_ROLE ) ) )
      {
         INT32 role = pmdGetDBRole() ;
         rc = nodePipe.writePipe( (const CHAR*)( &role ), sizeof( role ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_GID,
                                 sizeof( ENGINE_NPIPE_MSG_GID ) ) )
      {
         INT32 gid = pmdGetNodeID().columns.groupID ;
         rc = nodePipe.writePipe( (const CHAR*)( &gid ), sizeof( gid ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_NID,
                                 sizeof( ENGINE_NPIPE_MSG_NID ) ) )
      {
         INT32 nid = pmdGetNodeID().columns.nodeID ;
         rc = nodePipe.writePipe( (const CHAR*)( &nid ), sizeof( nid ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_GNAME,
                                 sizeof( ENGINE_NPIPE_MSG_GNAME ) ) )
      {
         const CHAR *gname = pmdGetKRCB()->getGroupName() ;
         rc = nodePipe.writePipe( gname, ossStrlen( gname ) + 1 ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_PATH,
                                 sizeof( ENGINE_NPIPE_MSG_PATH ) ) )
      {
         const CHAR *path = pmdGetOptionCB()->getDbPath() ;
         rc = nodePipe.writePipe( path, ossStrlen( path ) + 1 ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_PRIMARY,
                                 sizeof( ENGINE_NPIPE_MSG_PRIMARY ) ) )
      {
         INT32 primary = pmdIsPrimary() ? 1 : 0 ;
         rc = nodePipe.writePipe( (const CHAR*)( &primary ),
                                  sizeof( primary ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_STARTTIME,
                                 sizeof( ENGINE_NPIPE_MSG_STARTTIME ) ) )
      {
         UINT64 startTime = pmdGetStartTime() ;
         rc = nodePipe.writePipe( (const CHAR *)( &startTime ),
                                  sizeof( startTime ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_DOING,
                                 sizeof( ENGINE_NPIPE_MSG_DOING ) ) )
      {
         CHAR doing[ PMD_DOING_STR_LEN + 1 ] = { 0 } ;
         pmdGetDoing( doing, PMD_DOING_STR_LEN ) ;
         rc = nodePipe.writePipe( doing, ossStrlen( doing ) + 1 ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_LOCATION,
                sizeof( ENGINE_NPIPE_MSG_LOCATION ) ) )
      {
         const CHAR *location = pmdGetLocation() ;
         rc = nodePipe.writePipe( location, ossStrlen( location ) + 1 ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_LOCPRIMARY,
                                 sizeof( ENGINE_NPIPE_MSG_LOCPRIMARY ) ) )
      {
         INT32 locPrimary = pmdIsLocationPrimary() ? 1 : 0 ;
         rc = nodePipe.writePipe( (const CHAR*) ( &locPrimary ),
                                  sizeof( locPrimary ) ) ;
      }
      else
      {
         rc = SDB_UNKNOWN_MESSAGE ;
      }

      PD_TRACE_EXITRC( SDB_PMDINFOPIPEHANDLER_PROCESSMSG, rc ) ;

      return rc ;
   }

   // static handlers
   static pmdSystemPipeHandler s_systemHandler ;
   static pmdInfoPipeHandler s_infoHandler ;

   /*
      _pmdPipeManager implement
    */
   _pmdPipeManager::_pmdPipeManager()
   : _initialized( FALSE ),
     _systemPipe( FALSE ),
     _eduID( PMD_INVALID_EDUID )
   {
   }

   _pmdPipeManager::~_pmdPipeManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_INIT, "_pmdPipeManager::init" )
   INT32 _pmdPipeManager::init( const CHAR *serviceName,
                                BOOLEAN systemPipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_INIT ) ;

      if ( _initialized )
      {
         goto done ;
      }

      PD_CHECK( NULL != serviceName, SDB_SYS, error, PDERROR,
                "Failed to initialize pipe manager, service name is invalid" ) ;

      _serviceName = serviceName ;
      _systemPipe = systemPipe ;
      _initialized = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_FINI, "_pmdPipeManager::fini" )
   INT32 _pmdPipeManager::fini()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_FINI ) ;

      unregisterAllHandlers() ;

      _eduID = PMD_INVALID_EDUID ;
      _systemPipe = FALSE ;
      _initialized = FALSE ;

      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_FINI, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_ACTIVE, "_pmdPipeManager::active" )
   INT32 _pmdPipeManager::active()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_ACTIVE ) ;

      // create a named pipe
      rc = _nodePipe.createPipe( _serviceName.c_str() ) ;
      // For critical pipe: if we are not able to create named pipe, then we
      // are not able to stop it using sdbstop.exe. So we should nicely
      // shutdown database in order to prevent killing process later
      PD_RC_CHECK( rc, ( _systemPipe ? PDSEVERE : PDERROR ),
                   "Failed to create named pipe: %s, rc: %d",
                   _nodePipe.getReadPipeName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_ACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_DEACTIVE, "_pmdPipeManager::deactive" )
   INT32 _pmdPipeManager::deactive()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_DEACTIVE ) ;

      _nodePipe.autoRelease() ;
      _eduID = PMD_INVALID_EDUID ;

      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_DEACTIVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_STARTEDU, "_pmdPipeManager::startEDU" )
   INT32 _pmdPipeManager::startEDU()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_STARTEDU ) ;

      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;

      if ( _systemPipe )
      {
         rc = registerHandler( &s_systemHandler ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register system handler, "
                      "rc: %d", rc ) ;

         rc = registerHandler( &s_infoHandler ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register info handler, "
                      "rc: %d", rc ) ;
      }

      PD_CHECK( _initialized, SDB_SYS, error, PDERROR,
                "Failed to start EDU for pipe manager, "
                "it is not initialized" ) ;

      rc = eduMgr->startEDU( EDU_TYPE_PIPESLISTENER, (void *)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start PIPELISTENER, rc: %d", rc ) ;

      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait pipe listener to be running, "
                   "rc: %d", rc ) ;

      _eduID = eduID ;

      PD_LOG( PDEVENT, "Start pipe manager [%s] in EDU [%llu]",
              _serviceName.c_str(), _eduID ) ;

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_STARTEDU, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_REGHANDLER, "_pmdPipeManager::registerHandler" )
   INT32 _pmdPipeManager::registerHandler( IPmdPipeHandler *handler )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_REGHANDLER ) ;

      SDB_ASSERT( NULL != handler, "handler is invalid" ) ;
      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must register in main thread" ) ;

      PD_CHECK( NULL != handler, SDB_INVALIDARG, error, PDERROR,
                "Failed to register handler, handler is invalid" ) ;
      PD_CHECK( NULL != pmdGetThreadEDUCB() &&
                EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                SDB_SYS, error, PDERROR, "Failed to register handler, "
                "should use main thread to register" ) ;
      PD_CHECK( PMD_INVALID_EDUID == _eduID, SDB_SYS, error, PDERROR,
                "Failed to register handler, EDU has started" ) ;

      try
      {
         PMD_PIPE_HANDLE_LIST::iterator iter = find( _handlers.begin(),
                                                     _handlers.end(),
                                                     handler ) ;
         if ( iter == _handlers.end() )
         {
            _handlers.push_back( handler ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "handler has been already registered" ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register handler, occurred unexpected "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_REGHANDLER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_UNREGALLHANDLERS, "_pmdPipeManager::unregisterAllHandlers" )
   void _pmdPipeManager::unregisterAllHandlers()
   {
      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_UNREGALLHANDLERS ) ;

      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must register in main thread" ) ;

      if ( PMD_INVALID_EDUID == _eduID )
      {
         _handlers.clear() ;
      }
      else
      {
         PD_LOG( PDWARNING, "Failed to unregister handlers, "
                 "EDU is still running" ) ;
      }

      PD_TRACE_EXIT( SDB_PMDPIPEMGR_UNREGALLHANDLERS ) ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_RUN, "_pmdPipeManager::run" )
   INT32 _pmdPipeManager::run( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_RUN ) ;

      CHAR messageBuffer[ PMD_NPIPE_BUFSZ + 1 ] = { 0 } ;

      // just sit here do nothing at the moment
      while ( !cb->isDisconnected() )
      {
         INT32 messageSize = 0 ;
         rc = _nodePipe.connectPipe() ;
         if ( SDB_OK != rc )
         {
            // we just loop if nothing returns in SDB_TIMEOUT
            if ( SDB_TIMEOUT == rc )
            {
               rc = SDB_OK ;
               continue ;
            }
            else if ( SDB_TOO_MANY_OPEN_FD == rc )
            {
               ossSleep( OSS_ONE_SEC ) ;
               rc = SDB_OK ;
               continue ;
            }
            // if we are not able to connect named pipe, then we are not able
            // to stop it using sdbstop.exe. So we should nicely shutdown
            // database in order to prevent killing process later
            PD_LOG ( ( _systemPipe ? PDSEVERE : PDERROR ),
                     "Failed to connect named pipe: %s, rc: %d",
                     _nodePipe.getReadPipeName(), rc ) ;
            goto error ;
         }

         while ( 0 == messageSize && !cb->isDisconnected() )
         {
            // then let's read from pipe. For this version let's just read
            rc = _nodePipe.readPipe( messageBuffer, PMD_NPIPE_BUFSZ, messageSize ) ;
            if ( SDB_OK != rc )
            {
               // if we simply timeout, maybe the sender is too slow. Let's continue
               if ( SDB_TIMEOUT == rc )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               // if we failed to read, let's dump error and break out the loop
               PD_LOG( PDERROR, "Failed to read packet, rc: %d", rc ) ;
               messageSize = 0 ;
               rc = SDB_OK ;
               break ;
            }
         }

         if ( messageSize > 0 )
         {
            processMessage( messageBuffer, messageSize, _nodePipe ) ;
         }

         _nodePipe.disconnectPipe() ;
      }

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_RUN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPEMGR_PROCESSMSG, "_pmdPipeManager::processMessage" )
   INT32 _pmdPipeManager::processMessage( CHAR *message,
                                          INT32 messageSize,
                                          utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPEMGR_PROCESSMSG ) ;

      PD_LOG ( PDINFO, "Received message from pipe listener [%s] message: %s, "
               "size: %d", _nodePipe.getReadPipeName(), message, messageSize ) ;

      for ( PMD_PIPE_HANDLE_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         rc = ( *iter )->processMessage( message, nodePipe ) ;
         if ( SDB_UNKNOWN_MESSAGE != rc )
         {
            break ;
         }
      }

      PD_RC_CHECK( rc, PDWARNING, "Failed to process message [%s], rc: %d",
                   message, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_PMDPIPEMGR_PROCESSMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   pmdPipeManager *sdbGetSystemPipeManager()
   {
      static pmdPipeManager s_sysPipeManager ;
      return ( &s_sysPipeManager ) ;
   }

}
